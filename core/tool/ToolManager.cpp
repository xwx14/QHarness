#include "tool/ToolManager.h"

namespace qh {
namespace tool {

bool ToolManager::registerTool(Tool& tool) {
    const std::string name = tool.definition()._name;
    if (_tools.count(name) > 0) {
        return false;  // 重名：不覆盖
    }
    _tools.emplace(name, &tool);
    return true;
}

Tool* ToolManager::getTool(const std::string& name) const {
    auto it = _tools.find(name);
    return it == _tools.end() ? nullptr : it->second;
}

bool ToolManager::hasTool(const std::string& name) const {
    return _tools.count(name) > 0;
}

bool ToolManager::unregisterTool(const std::string& name) {
    return _tools.erase(name) > 0;
}

std::vector<schema::ToolDefinition> ToolManager::getAvailableTools() const {
    std::vector<schema::ToolDefinition> defs;
    defs.reserve(_tools.size());
    for (const auto& kv : _tools) {
        defs.push_back(kv.second->definition());
    }
    return defs;
}

schema::ToolResult ToolManager::execute(const schema::ToolCall& call) {
    Tool* tool = getTool(call._name);
    if (tool == nullptr) {
        schema::ToolResult result;
        result._toolCallId = call._id;
        result._output = "未找到工具: " + call._name;
        result._isError = true;
        return result;
    }
    return tool->execute(call);
}

} // namespace tool
} // namespace qh
