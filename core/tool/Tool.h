#ifndef QH_TOOL_H
#define QH_TOOL_H
#include "schema/Message.h"
#include "schema/PostMessage.h"
#include "qh_export.h"
#include <filesystem>
#include <optional>
#include <string>

namespace qh {
namespace tool {

// 工具抽象基类（edit/read/bash 等工具的基类）；继承 PostMessageInterface 获得实时消息注入
// ToolDefinition 作为成员，构造时构建（派生经 Tool(def) 注入），definition() 返回成员引用
class QH_API Tool : public schema::PostMessageInterface {
public:
    Tool() = default;
    explicit Tool(schema::ToolDefinition def) : _definition(std::move(def)) {}
    virtual ~Tool() = default;

    const schema::ToolDefinition& definition() const { return _definition; }
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;

    // 该调用锁定的资源键：相同键的调用在 executeAll 中被串行化执行；
    // nullopt 表示不参与串行化（自由并发）。默认 nullopt。
    virtual std::optional<std::string> resourceKey(const schema::ToolCall& call) const {
        return std::nullopt;
    }

protected:
    schema::ToolDefinition _definition;

    // 工作目录（UTF-8）：文件类工具用以限定可访问范围（路径穿越防护）；默认空。
    std::string _workDir;

    // 将 _workDir(UTF-8) 按平台编码转换并规范化为绝对基目录 base。
    // 解析失败（候选均无法规范化，如 _workDir 为空）返回空 path。
    std::filesystem::path resolveWorkBase() const;

    // 解析 relPath 到工作目录 base 内的绝对路径：对 relPath 的多种编码候选逐个尝试，
    // 通过路径穿越检查（规范化后仍位于 base 之内）且文件存在则返回；越界/不存在/解析失败返回 nullopt。
    std::optional<std::filesystem::path> resolveInside(const std::filesystem::path& base,
                                                       const std::string& relPath) const;

    // 解析 relPath 到工作目录 base 内的「可写」绝对路径：穿越检查（位于 base 内）但
    // 不要求目标已存在（写新文件）。父目录可不存在（调用方按需 create_directories）。
    std::optional<std::filesystem::path> resolveInsideForWrite(const std::filesystem::path& base,
                                                               const std::string& relPath) const;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_H
