#include "tool/ReadFileTool.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <utility>

namespace qh {
namespace tool {

namespace fs = std::filesystem;

ReadFileTool::ReadFileTool(std::string workDir) {
    _workDir = std::move(workDir);  // 基类 Tool::_workDir（UTF-8）
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

    // 2. 解析到工作目录内的绝对路径（基类：编码转换 + 规范化 + 穿越检查 + 存在性）
    const fs::path base = resolveWorkBase();
    if (base.empty()) {
        result._output = "工作目录解析失败: " + _workDir;
        result._isError = true;
        return result;
    }
    fs::path full;
    if (auto hit = resolveInside(base, relPath)) {
        full = *hit;
    }
    if (full.empty()) {
        result._output = "打开文件失败: " + relPath;
        result._isError = true;
        return result;
    }

    // 3. 物理 IO：文本模式读取全部内容（Windows 下自动将 CRLF 规范化为 LF）
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

    // 4. 长度截断保护：超过阈值按字节截断，避免超大文件撑爆上下文
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
