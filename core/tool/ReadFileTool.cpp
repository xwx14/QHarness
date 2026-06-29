#include "tool/ReadFileTool.h"
#include "tool/PathCodec.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <system_error>
#include <utility>

namespace qh {
namespace tool {

namespace fs = std::filesystem;

ReadFileTool::ReadFileTool(std::string workDir)
    : _workDir(std::move(workDir)) {
    _definition._name = "read_file";
    _definition._description = "读取指定路径的文件内容。请提供相对工作区的路径。";
    // JSON Schema：单个 path 字符串参数
    _definition._inputSchema = nlohmann::json::object();
    _definition._inputSchema["type"] = "object";
    _definition._inputSchema["properties"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["path"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["path"]["type"] = "string";
    _definition._inputSchema["properties"]["path"]["description"] =
        "要读取的文件路径，如 cmd/claw/main.go";
    _definition._inputSchema["required"] = nlohmann::json::array({ "path" });
}

namespace {
// 在 base 工作目录下解析单个候选 path：通过路径穿越检查且文件存在则返回 full。
// 返回 nullopt 表示该候选无效（越界/不存在/解析失败），应继续尝试下一候选。
std::optional<fs::path> resolveCandidate(const fs::path& base, const fs::path& cand) {
    std::error_code ec;
    const fs::path full = fs::weakly_canonical(base / cand, ec);
    if (ec) return std::nullopt;
    const fs::path rel = fs::relative(full, base, ec);
    // rel 为空或以 ".." 开头 → 越出工作目录
    if (ec || rel.empty() ||
        rel.native().rfind(fs::path("..").native(), 0) == 0) {
        return std::nullopt;
    }
    std::error_code existEc;
    if (!fs::exists(full, existEc) || existEc) return std::nullopt;
    return full;
}
} // namespace

schema::ToolResult ReadFileTool::execute(const schema::ToolCall& call) {
    schema::ToolResult result;
    result._toolCallId = call._id;

    // 1. 延迟解析：从原始 JSON 文本取出 path（容错解析，非法 JSON 退化为空对象）。
    //    JSON/nlohmann 要求 UTF-8，故经 AI 传入的 path 必为 UTF-8。
    const nlohmann::json args = call.parseArguments();
    if (!args.is_object() || !args.contains("path") || !args["path"].is_string()) {
        result._output = "参数解析失败: 缺少字符串字段 path";
        result._isError = true;
        return result;
    }
    const std::string relPath = args["path"].get<std::string>();  // UTF-8

    // 2. 工作目录 base：UTF-8 串按平台编码构造，取首个可规范化的候选
    fs::path base;
    for (const fs::path& cand : pathCodec::candidatePaths(_workDir)) {
        std::error_code ec;
        fs::path b = fs::weakly_canonical(cand, ec);
        if (!ec) { base = std::move(b); break; }
    }
    if (base.empty()) {
        result._output = "工作目录解析失败: " + _workDir;
        result._isError = true;
        return result;
    }

    // 3. 路径解析：对 path 的多种编码候选逐个尝试（穿越检查 + 存在性），取首个命中
    fs::path full;
    for (const fs::path& cand : pathCodec::candidatePaths(relPath)) {
        if (auto hit = resolveCandidate(base, cand)) {
            full = *hit;
            break;
        }
    }
    if (full.empty()) {
        result._output = "打开文件失败: " + relPath;
        result._isError = true;
        return result;
    }

    // 4. 物理 IO：文本模式读取全部内容（Windows 下自动将 CRLF 规范化为 LF）
    std::ifstream ifs(full);
    if (!ifs) {
        result._output = "打开文件失败: " + full.u8string();
        result._isError = true;
        return result;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    if (ifs.bad()) {
        result._output = "读取文件内容失败: " + full.u8string();
        result._isError = true;
        return result;
    }

    // 5. 长度截断保护：超过阈值按字节截断，避免超大文件撑爆上下文
    if (content.size() > kMaxLen) {
        content = content.substr(0, kMaxLen) +
            "\n\n...[由于内容过长，已被系统截断至前 " + std::to_string(kMaxLen) + " 字节]...";
    }

    result._output = std::move(content);
    result._isError = false;
    return result;
}

} // namespace tool
} // namespace qh
