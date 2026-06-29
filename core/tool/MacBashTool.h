#ifndef QH_TOOL_MACBASH_H
#define QH_TOOL_MACBASH_H
#include <string>
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// macOS bash 命令工具（/bin/sh -c）：在注入的工作目录执行，捕获 stdout/stderr，
// 超时可杀进程，UTF-8 输出截断返回。仅 macOS(__APPLE__)编译。实现同 LinuxBashTool（POSIX）。
class QH_API MacBashTool : public Tool {
public:
    explicit MacBashTool(std::string workDir);
    schema::ToolResult execute(const schema::ToolCall& call) override;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_MACBASH_H
