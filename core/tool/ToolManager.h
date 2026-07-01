#ifndef QH_TOOL_TOOLMANAGER_H
#define QH_TOOL_TOOLMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>
#include <future>
#include <map>
#include "schema/Message.h"
#include "tool/Tool.h"
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 工具管理器：持有多个工具的非拥有引用，负责注册/查找/注销/列举/分发执行
class QH_API ToolManager : public schema::PostMessageInterface {
public:
    // 注册工具：以 tool.definition()._name 为键；重名返回 false（不覆盖、不抛错）
    bool registerTool(Tool& tool);

    // 按名查找工具；找不到返回 nullptr
    Tool* getTool(const std::string& name) const;

    // 是否存在同名工具
    bool hasTool(const std::string& name) const;

    // 注销工具：按名移除；不存在返回 false
    bool unregisterTool(const std::string& name);

    // 列出全部已注册工具定义
    std::vector<schema::ToolDefinition> getAvailableTools() const;

    // 按 call._name 路由执行工具；未命中返回 isError=true 的结果（不抛异常）
    schema::ToolResult execute(const schema::ToolCall& call);

    // 按 maxConcurrency 并发执行 calls：同 resourceKey 的调用串行、异 key 并发；
    // 结果按 calls 原序返回。maxConcurrency<=0 表示无上限。任何工具抛异常都被隔离为 isError。
    std::vector<schema::ToolResult> executeAll(const std::vector<schema::ToolCall>& calls,
                                               int maxConcurrency);

private:
    std::unordered_map<std::string, Tool*> _tools;  // 非拥有：调用方保证工具生命周期
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_TOOLMANAGER_H
