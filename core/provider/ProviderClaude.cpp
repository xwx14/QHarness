#include "provider/ProviderClaude.h"
#include "provider/ProviderHttp.h"
#include <nlohmann/json.hpp>

namespace qh {
namespace provider {

using json = nlohmann::json;

ProviderClaude::ProviderClaude(std::string apiKey, std::string baseUrl, std::string model)
    : _apiKey(std::move(apiKey)),
      _baseUrl(std::move(baseUrl)),
      _model(std::move(model)) {}

void ProviderClaude::setTemperature(double t) { _temperature = t; }
void ProviderClaude::setMaxTokens(int n) { _maxTokens = n; }

json ProviderClaude::buildClaudeRequest(
    const std::vector<schema::Message>& messages,
    const std::vector<schema::ToolDefinition>& tools) const {
    json req;
    req["model"] = _model;
    req["max_tokens"] = _maxTokens.value_or(4096); // Anthropic 必填
    req["messages"] = json::array();

    for (const auto& m : messages) {
        // system 抽到顶层字段
        if (m._role == schema::Role::System) {
            if (!m._content.empty()) req["system"] = m._content;
            continue;
        }
        json jmsg;
        if (!m._toolCallId.empty()) {
            // 工具结果：user 消息 + tool_result block
            jmsg["role"] = "user";
            json c = json::array();
            c.push_back({{"type", "tool_result"},
                         {"tool_use_id", m._toolCallId},
                         {"content", m._content}});
            jmsg["content"] = c;
        } else if (m._role == schema::Role::Assistant && !m._toolCalls.empty()) {
            // assistant 工具调用：content block 数组（text? + tool_use[]）
            jmsg["role"] = "assistant";
            json c = json::array();
            if (!m._content.empty()) c.push_back({{"type", "text"}, {"text", m._content}});
            for (const auto& tc : m._toolCalls) {
                c.push_back({{"type", "tool_use"},
                             {"id", tc._id},
                             {"name", tc._name},
                             {"input", tc.parseArguments()}}); // 对象
            }
            jmsg["content"] = c;
        } else {
            jmsg["role"] = schema::roleToString(m._role);
            jmsg["content"] = m._content; // 字符串
        }
        req["messages"].push_back(jmsg);
    }

    if (_temperature.has_value()) req["temperature"] = *_temperature;
    if (!tools.empty()) {
        json ta = json::array();
        for (const auto& t : tools) {
            ta.push_back({{"name", t._name},
                          {"description", t._description},
                          {"input_schema", t._inputSchema}});
        }
        req["tools"] = ta;
        req["tool_choice"] = {{"type", "auto"}};
    }
    return req;
}

schema::Message ProviderClaude::parseClaudeResponse(const std::string& body) const {
    schema::Message msg;
    msg._role = schema::Role::Assistant;
    auto j = json::parse(body, nullptr, false);
    if (j.is_discarded() || !j.contains("content") || !j["content"].is_array()) return msg;

    for (const auto& block : j["content"]) {
        std::string type = block.value("type", "");
        if (type == "text") {
            msg._content += block.value("text", "");
        } else if (type == "tool_use") {
            schema::ToolCall tc;
            tc._id = block.value("id", "");
            tc._name = block.value("name", "");
            if (block.contains("input") && block["input"].is_object())
                tc._arguments = block["input"].dump(); // 对象 → 原始文本
            else tc._arguments = "{}";
            msg._toolCalls.push_back(std::move(tc));
        }
    }
    return msg;
}

std::string ProviderClaude::parseClaudeError(int status, const std::string& body) {
    auto j = json::parse(body, nullptr, false);
    if (!j.is_discarded() && j.contains("error") && j["error"].contains("message"))
        return "Anthropic " + std::to_string(status) + ": " + j["error"]["message"].get<std::string>();
    return "Anthropic HTTP " + std::to_string(status) + ": " + body.substr(0, 200);
}

GenerateResult ProviderClaude::generate(
    const schema::CancellationToken& cancel,
    const std::vector<schema::Message>& messages,
    const std::vector<schema::ToolDefinition>& tools) {
    GenerateResult result;
    if (cancel.isCancelled()) { result.error = "cancelled"; return result; }

    std::string body = buildClaudeRequest(messages, tools).dump();
    info(std::string("→ POST /v1/messages  model=") + _model);

    std::vector<std::pair<std::string, std::string>> headers = {
        {"x-api-key", _apiKey},
        {"anthropic-version", "2023-06-01"}};
    HttpResponse resp = ProviderHttp::post(_baseUrl, "/messages", headers, body, cancel);

    if (cancel.isCancelled()) { result.error = "cancelled"; return result; }
    if (!resp.ok) { result.error = resp.error; error(resp.error); return result; }
    if (resp.status != 200) {
        result.error = parseClaudeError(resp.status, resp.body);
        error(result.error);
        return result;
    }

    result.message = parseClaudeResponse(resp.body);
    if (!result.message._content.empty()) chat(result.message._content);
    if (!result.message._toolCalls.empty())
        think("工具调用 " + std::to_string(result.message._toolCalls.size()) + " 个");
    return result;
}

} // namespace provider
} // namespace qh
