#ifndef QH_TOOL_LINUXBASH_H
#define QH_TOOL_LINUXBASH_H
#include <optional>
#include <string>
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// Linux bash 命令工具（/bin/sh -c）：在注入的工作目录执行，捕获 stdout/stderr，
// 超时可杀进程，UTF-8 输出截断返回。仅非 Windows 的 POSIX(Linux) 编译。
class QH_API LinuxBashTool : public Tool {
public:
    explicit LinuxBashTool(std::string workDir);
    schema::ToolResult execute(const schema::ToolCall& call) override;

    // bash 命令的目标文件无法从 command 静态推断；所有 bash 归一桶串行（避免命令互相干扰）
    std::optional<std::string> resourceKey(const schema::ToolCall& call) const override {
        return "__bash__";
    }
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_LINUXBASH_H
