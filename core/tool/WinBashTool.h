#ifndef QH_TOOL_WINBASH_H
#define QH_TOOL_WINBASH_H
#include <string>
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// Windows 命令行工具（cmd.exe /c）：在注入的工作目录执行命令，捕获 stdout/stderr，
// 超时可杀进程，输出按本地代码页(GBK)转 UTF-8 后截断返回。仅 Windows 编译。
class QH_API WinBashTool : public Tool {
public:
    explicit WinBashTool(std::string workDir);
    schema::ToolResult execute(const schema::ToolCall& call) override;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_WINBASH_H
