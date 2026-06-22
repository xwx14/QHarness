#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 模型请求调用的具体工具
struct ToolCall {
    std::string id;          // 工具调用唯一 ID
    std::string name;        // 工具名（如 "bash"）
    std::string arguments;   // 原始 JSON 文本（延迟解析，解析责任交给具体工具）

    // 按需将原始 arguments 解析为结构化对象
    nlohmann::json parseArguments() const;
};

void to_json(nlohmann::json& j, const ToolCall& tc);
void from_json(const nlohmann::json& j, ToolCall& tc);

} // namespace schema
} // namespace qh
