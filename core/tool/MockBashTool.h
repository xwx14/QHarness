#ifndef QH_TOOL_MOCKBASH_H
#define QH_TOOL_MOCKBASH_H
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 模拟 bash 工具（参考 Go mockRegistry.Execute）：definition 名称注册为 "bash"，
// execute 忽略真实命令参数，返回固定的 main.go ls 输出，用于测试引擎 tool-call 闭环
class QH_API MockBashTool : public Tool {
public:
    MockBashTool();
    schema::ToolResult execute(const schema::ToolCall& call) override;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_MOCKBASH_H
