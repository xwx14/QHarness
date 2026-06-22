#include "ToolCall.h"

namespace qh {
namespace schema {

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

} // namespace schema
} // namespace qh
