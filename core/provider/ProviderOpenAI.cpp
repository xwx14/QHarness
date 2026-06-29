#include "provider/ProviderOpenAI.h"
#include "provider/ProviderHttp.h"
#include <nlohmann/json.hpp>

namespace qh {
namespace provider {

using json = nlohmann::json;

ProviderOpenAI::ProviderOpenAI(std::string apiKey, std::string baseUrl, std::string model)
    : _apiKey(std::move(apiKey)),
      _baseUrl(std::move(baseUrl)),
      _model(std::move(model)) {}

void ProviderOpenAI::setTemperature(double t) { _temperature = t; }
void ProviderOpenAI::setMaxTokens(int n) { _maxTokens = n; }

namespace {
// 内部 Message -> OpenAI 单条消息 JSON
json toOpenaiMessage(const schema::Message& m) {
    // 工具结果消息：role=tool + tool_call_id
    if (!m._toolCallId.empty()) {
        return {{"role", "tool"},
                {"tool_call_id", m._toolCallId},
                {"content", m._content}};
    }
    json j;
    j["role"] = schema::roleToString(m._role);
    if (!m._content.empty()) j["content"] = m._content;
    if (!m._toolCalls.empty()) {
        json tcs = json::array();
        for (const auto& tc : m._toolCalls) {
            tcs.push_back({{"id", tc._id},
                           {"type", "function"},
                           {"function", {{"name", tc._name}, {"arguments", tc._arguments}}}});
        }
        j["tool_calls"] = tcs;
    }
    return j;
}
} // namespace

json ProviderOpenAI::buildOpenaiRequest(
    const std::vector<schema::Message>& messages,
    const std::vector<schema::ToolDefinition>& tools) const {
    json req;
    req["model"] = _model;
    req["messages"] = json::array();
    for (const auto& m : messages) req["messages"].push_back(toOpenaiMessage(m));

    if (_temperature.has_value()) req["temperature"] = *_temperature;
    if (_maxTokens.has_value()) req["max_completion_tokens"] = *_maxTokens;

    if (!tools.empty()) {
        json ta = json::array();
        for (const auto& t : tools) {
            ta.push_back({{"type", "function"},
                          {"function", {{"name", t._name},
                                        {"description", t._description},
                                        {"parameters", t._inputSchema}}}});
        }
        req["tools"] = ta;
        req["tool_choice"] = "auto";
    }
    return req;
}

schema::Message ProviderOpenAI::parseOpenaiResponse(const std::string& body) const {
    schema::Message msg;
    msg._role = schema::Role::Assistant;
    auto j = json::parse(body, nullptr, false);
    if (j.is_discarded() || !j.contains("choices") || j["choices"].empty()) return msg;

    const auto& choice = j["choices"][0];
    if (choice.contains("message")) {
        const auto& m = choice["message"];
        if (m.contains("content") && !m["content"].is_null())
            msg._content = m["content"].get<std::string>();
        if (m.contains("tool_calls") && m["tool_calls"].is_array()) {
            for (const auto& tc : m["tool_calls"]) {
                schema::ToolCall call;
                call._id = tc.value("id", "");
                if (tc.contains("function")) {
                    call._name = tc["function"].value("name", "");
                    const auto& a = tc["function"]["arguments"];
                    if (a.is_string()) call._arguments = a.get<std::string>();
                    else if (!a.is_null()) call._arguments = a.dump();
                    else call._arguments = "{}";
                }
                msg._toolCalls.push_back(std::move(call));
            }
        }
    }
    return msg;
}

std::string ProviderOpenAI::parseOpenaiError(int status, const std::string& body) {
    auto j = json::parse(body, nullptr, false);
    if (!j.is_discarded() && j.contains("error") && j["error"].contains("message"))
        return "OpenAI " + std::to_string(status) + ": " + j["error"]["message"].get<std::string>();
    return "OpenAI HTTP " + std::to_string(status) + ": " + body.substr(0, 200);
}

GenerateResult ProviderOpenAI::generate(
    const schema::CancellationToken& cancel,
    const std::vector<schema::Message>& messages,
    const std::vector<schema::ToolDefinition>& tools) {
    GenerateResult result;
    if (cancel.isCancelled()) { result.error = "cancelled"; return result; }

    std::string body = buildOpenaiRequest(messages, tools).dump();
    info(std::string("→ POST /v1/chat/completions  model=") + _model);

    std::vector<std::pair<std::string, std::string>> headers = {
        {"Authorization", "Bearer " + _apiKey}};
    HttpResponse resp = ProviderHttp::post(_baseUrl, "/chat/completions", headers, body, cancel);

    if (cancel.isCancelled()) { result.error = "cancelled"; return result; }
    if (!resp.ok) { result.error = resp.error; error(resp.error); return result; }
    if (resp.status != 200) {
        result.error = parseOpenaiError(resp.status, resp.body);
        error(result.error);
        return result;
    }

    result.message = parseOpenaiResponse(resp.body);
    // 对话窗口投递由引擎层统一负责（引擎掌握"最终回复 vs 中间步骤"语义）；
    // provider 仅调 API + 解析，不越权写对话窗口，否则与引擎重复投递致回复显示两遍
    if (!result.message._toolCalls.empty())
        think("工具调用 " + std::to_string(result.message._toolCalls.size()) + " 个");
    return result;
}

} // namespace provider
} // namespace qh
