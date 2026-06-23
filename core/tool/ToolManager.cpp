#include "tool/ToolManager.h"

namespace qh {
namespace tool {

bool ToolManager::registerTool(Tool& tool) {
    const std::string name = tool.definition().name;
    if (tools_.count(name) > 0) {
        return false;  // 重名：不覆盖
    }
    tools_.emplace(name, &tool);
    return true;
}

Tool* ToolManager::getTool(const std::string& name) const {
    auto it = tools_.find(name);
    return it == tools_.end() ? nullptr : it->second;
}

bool ToolManager::hasTool(const std::string& name) const {
    return tools_.count(name) > 0;
}

bool ToolManager::unregisterTool(const std::string& name) {
    return tools_.erase(name) > 0;
}

std::vector<schema::ToolDefinition> ToolManager::getAvailableTools() const {
    std::vector<schema::ToolDefinition> defs;
    defs.reserve(tools_.size());
    for (const auto& kv : tools_) {
        defs.push_back(kv.second->definition());
    }
    return defs;
}

schema::ToolResult ToolManager::execute(const schema::ToolCall& call) {
    Tool* tool = getTool(call.name);
    if (tool == nullptr) {
        schema::ToolResult result;
        result.toolCallId = call.id;
        result.output = "未找到工具: " + call.name;
        result.isError = true;
        return result;
    }
    return tool->execute(call);
}

} // namespace tool
} // namespace qh
