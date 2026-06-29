#include "tool/ReadFileTool.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>

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

schema::ToolResult ReadFileTool::execute(const schema::ToolCall& call) {
    schema::ToolResult result;
    result._toolCallId = call._id;

    // 1. 延迟解析：从原始 JSON 文本取出 path（容错解析，非法 JSON 退化为空对象）
    const nlohmann::json args = call.parseArguments();
    if (!args.is_object() || !args.contains("path") || !args["path"].is_string()) {
        result._output = "参数解析失败: 缺少字符串字段 path";
        result._isError = true;
        return result;
    }
    const std::string relPath = args["path"].get<std::string>();

    // 2. 路径穿越防护：规范化后校验目标仍位于工作目录之内
    std::error_code ec;
    const fs::path base = fs::weakly_canonical(fs::path(_workDir), ec);
    const fs::path full = fs::weakly_canonical(base / fs::path(relPath), ec);
    if (ec) {
        result._output = "路径解析失败: " + ec.message();
        result._isError = true;
        return result;
    }
    const fs::path rel = fs::relative(full, base, ec);
    // rel 为空或以 ".." 开头 → 越出工作目录，拒绝
    if (ec || rel.empty() || rel.native().rfind(fs::path("..").native(), 0) == 0) {
        result._output = "拒绝访问: 路径越出工作目录范围";
        result._isError = true;
        return result;
    }

    // 3. 物理 IO：二进制读取全部内容
    std::ifstream ifs(full, std::ios::binary);
    if (!ifs) {
        result._output = "打开文件失败: " + full.string();
        result._isError = true;
        return result;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    if (ifs.bad()) {
        result._output = "读取文件内容失败: " + full.string();
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
