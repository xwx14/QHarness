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
    // arguments 作为原始 JSON 值（对象/数组），而非被二次转义的字符串
    const std::string raw = tc.arguments.empty() ? std::string("{}") : tc.arguments;
    j["arguments"] = nlohmann::json::parse(raw); // 解析失败会抛异常——由调用方保证参数合法
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
