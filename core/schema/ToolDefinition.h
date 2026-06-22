#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 大模型可调用工具的元信息（供模型理解工具有何用途）
struct ToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json inputSchema; // 对应 JSON Schema
};

void to_json(nlohmann::json& j, const ToolDefinition& td);
void from_json(const nlohmann::json& j, ToolDefinition& td);

} // namespace schema
} // namespace qh
