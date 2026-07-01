#include "tool/WriteFileTool.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <system_error>
#include <utility>

namespace qh {
namespace tool {

namespace fs = std::filesystem;

WriteFileTool::WriteFileTool(std::string workDir) {
    _workDir = std::move(workDir);  // 基类 Tool::_workDir（UTF-8）
    _definition._name = "write_file";
    _definition._description = "新建或覆盖写入文件。请提供相对工作区的路径与内容。";
    _definition._inputSchema = nlohmann::json::object();
    _definition._inputSchema["type"] = "object";
    _definition._inputSchema["properties"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["path"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["path"]["type"] = "string";
    _definition._inputSchema["properties"]["path"]["description"] = "要写入的文件路径，如 src/main.cpp";
    _definition._inputSchema["properties"]["content"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["content"]["type"] = "string";
    _definition._inputSchema["properties"]["content"]["description"] = "文件内容（UTF-8 文本）";
    _definition._inputSchema["required"] = nlohmann::json::array({ "path", "content" });
}

schema::ToolResult WriteFileTool::execute(const schema::ToolCall& call) {
    schema::ToolResult result;
    result._toolCallId = call._id;

    // 1. 解析参数（path + content，均 UTF-8）
    const nlohmann::json args = call.parseArguments();
    if (!args.is_object() || !args.contains("path") || !args["path"].is_string() ||
        !args.contains("content") || !args["content"].is_string()) {
        result._output = "参数解析失败: 缺少字符串字段 path/content";
        result._isError = true;
        return result;
    }
    const std::string relPath = args["path"].get<std::string>();
    const std::string content = args["content"].get<std::string>();

    // 2. 解析到工作目录内的可写路径（基类：编码转换 + 规范化 + 穿越检查，不要求存在）
    const fs::path base = resolveWorkBase();
    if (base.empty()) {
        result._output = "工作目录解析失败: " + _workDir;
        result._isError = true;
        return result;
    }
    auto hit = resolveInsideForWrite(base, relPath);
    if (!hit) {
        result._output = "拒绝写入: 路径越出工作目录范围";
        result._isError = true;
        return result;
    }
    const fs::path full = *hit;

    // 3. 按需创建父目录
    std::error_code ec;
    if (full.has_parent_path()) fs::create_directories(full.parent_path(), ec);
    if (ec) {
        result._output = "创建目录失败: " + full.u8string();
        result._isError = true;
        return result;
    }

    // 4. UTF-8 binary 写入（content 已是 UTF-8）；存在则覆盖（trunc）
    std::ofstream ofs(full, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        result._output = "打开文件失败: " + full.u8string();
        result._isError = true;
        return result;
    }
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!ofs) {
        result._output = "写入失败: " + full.u8string();
        result._isError = true;
        return result;
    }

    result._output = "已写入 " + full.u8string() + " (" +
                     std::to_string(content.size()) + " 字节)";
    result._isError = false;
    return result;
}

std::optional<std::string> WriteFileTool::resourceKey(const schema::ToolCall& call) const {
    const nlohmann::json args = call.parseArguments();
    if (!args.is_object() || !args.contains("path") || !args["path"].is_string())
        return std::nullopt;
    const std::string relPath = args["path"].get<std::string>();
    const fs::path base = resolveWorkBase();
    if (base.empty()) return std::nullopt;
    auto hit = resolveInsideForWrite(base, relPath);   // 穿越检查 + 规范化，不查存在性
    return hit ? std::optional<std::string>(hit->string()) : std::nullopt;
}

} // namespace tool
} // namespace qh
