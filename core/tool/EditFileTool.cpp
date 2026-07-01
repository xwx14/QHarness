#include "tool/EditFileTool.h"
#include "tool/ContentCodec.h"
#include <nlohmann/json.hpp>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace qh {
namespace tool {

namespace fs = std::filesystem;

namespace {

// ===== 字符串工具（对应 Go strings 包语义，仅本文件用）=====

// 子串出现次数（空 needle 返回 0，避免无限循环；正常场景 needle 非空）
size_t countOccurrences(const std::string& s, const std::string& sub) {
    if (sub.empty()) return 0;
    size_t count = 0, pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) {
        ++count;
        pos += sub.size();
    }
    return count;
}

// 替换首次出现（找不到则原样返回）
std::string replaceFirst(const std::string& s, const std::string& from, const std::string& to) {
    const size_t pos = s.find(from);
    if (pos == std::string::npos) return s;
    std::string out = s;
    out.replace(pos, from.size(), to);
    return out;
}

// 全量替换（from 空 → 原样返回）
std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

// 去首尾空白（对应 Go strings.TrimSpace；按字节判 isspace，UTF-8 多字节字符的
// 首字节(>=0xC0)/续字节(0x80-0xBF)均非 ASCII 空白(0x09-0x0D,0x20)，故对 UTF-8 安全）
std::string trimSpace(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

// 按 '\n' 切分（空串 → [""], 与 Go strings.Split 一致）
std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : s) {
        if (c == '\n') { lines.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    lines.push_back(cur);
    return lines;
}

// 用 '\n' 连接
std::string joinLines(const std::vector<std::string>& lines) {
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i) out.push_back('\n');
        out += lines[i];
    }
    return out;
}

// 模糊替换结果：ok=true 时 content 为替换后文本；ok=false 时 error 为失败原因
struct FuzzyResult {
    bool ok;
    std::string content;
    std::string error;
};

// L4：逐行去缩进匹配——对 oldText 与 content 各行做 TrimSpace 后比对，
// 容忍缩进/首尾空白差异；匹配连续多行片段，要求唯一命中。
FuzzyResult lineByLineReplace(const std::string& content, const std::string& oldText,
                              const std::string& newText) {
    std::vector<std::string> contentLines = splitLines(content);
    std::vector<std::string> oldLines = splitLines(trimSpace(oldText));

    if (oldLines.empty() || contentLines.size() < oldLines.size()) {
        return {false, "", "找不到该代码片段"};
    }
    for (auto& l : oldLines) l = trimSpace(l);

    size_t matchCount = 0;
    size_t matchStart = 0, matchEnd = 0;   // matchCount==0 时无意义
    for (size_t i = 0; i + oldLines.size() <= contentLines.size(); ++i) {
        bool isMatch = true;
        for (size_t j = 0; j < oldLines.size(); ++j) {
            if (trimSpace(contentLines[i + j]) != oldLines[j]) { isMatch = false; break; }
        }
        if (isMatch) {
            ++matchCount;
            matchStart = i;
            matchEnd = i + oldLines.size();
        }
    }

    if (matchCount == 0) return {false, "", "在文件中未找到 old_text，请检查内容和缩进"};
    if (matchCount > 1)  return {false, "",
        "模糊匹配到了 " + std::to_string(matchCount) + " 处代码，请提供更多上下文以定位"};

    std::vector<std::string> out;
    out.insert(out.end(), contentLines.begin(), contentLines.begin() + matchStart);
    out.push_back(newText);
    out.insert(out.end(), contentLines.begin() + matchEnd, contentLines.end());
    return {true, joinLines(out), ""};
}

// 多级模糊替换（对应 Go fuzzyReplace）：
// L1 精确 → L2 换行归一化(\r\n→\n) → L3 trim 首尾空白 → L4 逐行去缩进
FuzzyResult fuzzyReplace(const std::string& originalContent,
                         const std::string& oldText,
                         const std::string& newText) {
    // L1：精确匹配
    size_t count = countOccurrences(originalContent, oldText);
    if (count == 1) return {true, replaceFirst(originalContent, oldText, newText), ""};
    if (count > 1)  return {false, "",
        "old_text 匹配到了 " + std::to_string(count) + " 处，请提供更多的上下文代码以确保唯一性"};

    // L2：换行符归一化
    std::string normalizedContent = replaceAll(originalContent, "\r\n", "\n");
    std::string normalizedOld     = replaceAll(oldText, "\r\n", "\n");
    count = countOccurrences(normalizedContent, normalizedOld);
    if (count == 1) return {true, replaceFirst(normalizedContent, normalizedOld, newText), ""};

    // L3：trim 首尾空白后匹配
    std::string trimmedOld = trimSpace(normalizedOld);
    if (!trimmedOld.empty()) {
        count = countOccurrences(normalizedContent, trimmedOld);
        if (count == 1) return {true, replaceFirst(normalizedContent, trimmedOld, newText), ""};
    }

    // L4：逐行去缩进匹配
    return lineByLineReplace(normalizedContent, normalizedOld, newText);
}

} // namespace

EditFileTool::EditFileTool(std::string workDir) {
    _workDir = std::move(workDir);  // 基类 Tool::_workDir（UTF-8）
    _definition._name = "edit_file";
    _definition._description =
        "对现有文件进行局部的字符串替换。这比重写整个文件更安全、更快速。"
        "请提供足够的 old_text 上下文以确保匹配的唯一性。";
    _definition._inputSchema = nlohmann::json::object();
    _definition._inputSchema["type"] = "object";
    _definition._inputSchema["properties"] = nlohmann::json::object();

    _definition._inputSchema["properties"]["path"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["path"]["type"] = "string";
    _definition._inputSchema["properties"]["path"]["description"] = "要修改的文件路径，如 src/main.cpp";

    _definition._inputSchema["properties"]["old_text"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["old_text"]["type"] = "string";
    _definition._inputSchema["properties"]["old_text"]["description"] =
        "文件中原有的文本。必须包含足够的上下文，以确保在文件中的唯一性。";

    _definition._inputSchema["properties"]["new_text"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["new_text"]["type"] = "string";
    _definition._inputSchema["properties"]["new_text"]["description"] = "要替换成的新文本";

    _definition._inputSchema["required"] = nlohmann::json::array({ "path", "old_text", "new_text" });
}

schema::ToolResult EditFileTool::execute(const schema::ToolCall& call) {
    schema::ToolResult result;
    result._toolCallId = call._id;

    // 1. 解析参数 path/old_text/new_text（均 UTF-8 字符串）
    const nlohmann::json args = call.parseArguments();
    if (!args.is_object() || !args.contains("path") || !args["path"].is_string() ||
        !args.contains("old_text") || !args["old_text"].is_string() ||
        !args.contains("new_text") || !args["new_text"].is_string()) {
        result._output = "参数解析失败: 缺少字符串字段 path/old_text/new_text";
        result._isError = true;
        return result;
    }
    const std::string relPath = args["path"].get<std::string>();
    const std::string oldText = args["old_text"].get<std::string>();
    const std::string newText = args["new_text"].get<std::string>();

    // 2. 解析到工作目录内（编辑要求文件已存在 → resolveInside：穿越检查 + 存在性）
    const fs::path base = resolveWorkBase();
    if (base.empty()) {
        result._output = "工作目录解析失败: " + _workDir;
        result._isError = true;
        return result;
    }
    fs::path full;
    if (auto hit = resolveInside(base, relPath)) full = *hit;
    if (full.empty()) {
        result._output = "读取文件失败，请确认路径是否正确: " + relPath;
        result._isError = true;
        return result;
    }

    // 3. binary 读全部字节 → 转 UTF-8（与 ReadFileTool 一致；old_text/new_text 亦为 UTF-8，匹配在 UTF-8 空间）
    std::ifstream ifs(full, std::ios::binary);
    if (!ifs) {
        result._output = "读取文件失败，请确认路径是否正确: " + full.u8string();
        result._isError = true;
        return result;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (ifs.bad()) {
        result._output = "读取文件内容失败: " + full.u8string();
        result._isError = true;
        return result;
    }
    content = contentCodec::toUtf8(content);

    // 4. 多级模糊替换
    const FuzzyResult fr = fuzzyReplace(content, oldText, newText);
    if (!fr.ok) {
        result._output = fr.error;
        result._isError = true;
        return result;
    }

    // 5. binary 写回（UTF-8，与 WriteFileTool 一致；trunc 覆盖）
    std::ofstream ofs(full, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        result._output = "写回文件失败: " + full.u8string();
        result._isError = true;
        return result;
    }
    ofs.write(fr.content.data(), static_cast<std::streamsize>(fr.content.size()));
    if (!ofs) {
        result._output = "写回文件失败: " + full.u8string();
        result._isError = true;
        return result;
    }

    result._output = "✅ 成功修改文件: " + relPath;
    result._isError = false;
    return result;
}

} // namespace tool
} // namespace qh
