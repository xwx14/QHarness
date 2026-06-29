#ifndef QH_TOOL_LINUXBASH_H
#define QH_TOOL_LINUXBASH_H
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
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_LINUXBASH_H
