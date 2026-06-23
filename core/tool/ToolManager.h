#ifndef QH_TOOL_TOOLMANAGER_H
#define QH_TOOL_TOOLMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>
#include "schema/Message.h"
#include "tool/Tool.h"
#include "tool/ToolRegistry.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 工具管理器：持有多个工具的非拥有引用，负责注册/查找/分发执行
class QH_API ToolManager : public ToolRegistry {
public:
    // 注册工具：以 tool.definition().name 为键；重名返回 false（不覆盖、不抛错）
    bool registerTool(Tool& tool);

    // 按名查找工具；找不到返回 nullptr
    Tool* getTool(const std::string& name) const;

    // 是否存在同名工具
    bool hasTool(const std::string& name) const;

    // 注销工具：按名移除；不存在返回 false
    bool unregisterTool(const std::string& name);

    // 实现 ToolRegistry 契约
    std::vector<schema::ToolDefinition> getAvailableTools() const override;
    schema::ToolResult execute(const schema::ToolCall& call) override;

private:
    std::unordered_map<std::string, Tool*> tools_;  // 非拥有：调用方保证工具生命周期
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_TOOLMANAGER_H
