#include "Message.h"

namespace qh {
namespace schema {

void to_json(nlohmann::json& j, const Message& m) {
    j = nlohmann::json{{"role", m.role}, {"content", m.content}};
    if (!m.toolCalls.empty()) {
        j["tool_calls"] = m.toolCalls; // vector<ToolCall> 自动序列化
    }
    if (!m.toolCallId.empty()) {
        j["tool_call_id"] = m.toolCallId;
    }
}

void from_json(const nlohmann::json& j, Message& m) {
    j.at("role").get_to(m.role);
    j.at("content").get_to(m.content);
    if (j.contains("tool_calls")) {
        j.at("tool_calls").get_to(m.toolCalls);
    }
    if (j.contains("tool_call_id")) {
        j.at("tool_call_id").get_to(m.toolCallId);
    }
}

} // namespace schema
} // namespace qh
