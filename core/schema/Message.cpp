#include "schema/Message.h"

namespace qh {
namespace schema {

std::string roleToString(Role role) {
    switch (role) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
    }
    return "user"; // 兜底
}

Role roleFromString(const std::string& str) {
    if (str == "system")    return Role::System;
    if (str == "assistant") return Role::Assistant;
    return Role::User;
}

nlohmann::json ToolCall::parseArguments() const {
    if (arguments.empty()) {
        return nlohmann::json::object();
    }
    try {
        return nlohmann::json::parse(arguments);
    } catch (...) {
        return nlohmann::json::object(); // 解析失败兜底为空对象
    }
}

void to_json(nlohmann::json& j, const ToolCall& tc) {
    j = nlohmann::json{{"id", tc.id}, {"name", tc.name}};
    // arguments 作为原始 JSON 值（对象/数组）；容错：非法 JSON 退化为空对象，不抛异常
    const std::string raw = tc.arguments.empty() ? std::string("{}") : tc.arguments;
    auto parsed = nlohmann::json::parse(raw, nullptr, false);
    j["arguments"] = parsed.is_discarded() ? nlohmann::json::object() : parsed;
}

void from_json(const nlohmann::json& j, ToolCall& tc) {
    j.at("id").get_to(tc.id);
    j.at("name").get_to(tc.name);
    if (j.contains("arguments") && !j["arguments"].is_null()) {
        tc.arguments = j["arguments"].dump(); // 保留原始文本
    } else {
        tc.arguments = "{}";
    }
}

void to_json(nlohmann::json& j, const ToolResult& tr) {
    j = nlohmann::json{
        {"tool_call_id", tr.toolCallId},
        {"output", tr.output},
        {"is_error", tr.isError}};
}

void from_json(const nlohmann::json& j, ToolResult& tr) {
    j.at("tool_call_id").get_to(tr.toolCallId);
    j.at("output").get_to(tr.output);
    j.at("is_error").get_to(tr.isError);
}

void to_json(nlohmann::json& j, const ToolDefinition& td) {
    j = nlohmann::json{
        {"name", td.name},
        {"description", td.description},
        {"input_schema", td.inputSchema}};
}

void from_json(const nlohmann::json& j, ToolDefinition& td) {
    j.at("name").get_to(td.name);
    j.at("description").get_to(td.description);
    j.at("input_schema").get_to(td.inputSchema);
}

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
