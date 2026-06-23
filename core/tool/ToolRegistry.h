#ifndef QH_TOOL_TOOLREGISTRY_H
#define QH_TOOL_TOOLREGISTRY_H
#include <vector>
#include "schema/Message.h"

namespace qh {
namespace tool {

// 工具注册与分发执行的抽象接口（对应 Go 的 tools.Registry）
// engine 依赖此抽象而非具体 ToolManager（依赖倒置）
class ToolRegistry {
public:
    virtual ~ToolRegistry() = default;

    // 返回当前挂载的全部工具定义（对应 Go GetAvailableTools）
    virtual std::vector<schema::ToolDefinition> getAvailableTools() const = 0;

    // 按 call.name 路由并执行工具（对应 Go Execute）
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_TOOLREGISTRY_H
