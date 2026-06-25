#include "MockHttpServer.h"
#include "provider/ProviderClaude.h"
#include "schema/Message.h"
#include "schema/CancellationToken.h"
#include <nlohmann/json.hpp>
#include "TestHarness.h"

using json = nlohmann::json;

static const std::string kClaudeTextResp = R"({
  "content":[{"type":"text","text":"bonjour"}],
  "stop_reason":"end_turn"
})";

static const std::string kClaudeToolResp = R"({
  "content":[{"type":"text","text":"thinking..."},
             {"type":"tool_use","id":"tu_1","name":"bash","input":{"command":"ls"}}],
  "stop_reason":"tool_use"
})";

QH_TEST(ProviderClaude_text_response) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/messages", 200, kClaudeTextResp);
    qh::provider::ProviderClaude p("k", svr.baseUrl(), "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK(r.error.empty());
    QH_CHECK_EQ(r.message._content, std::string("bonjour"));
}

QH_TEST(ProviderClaude_tool_response) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/messages", 200, kClaudeToolResp);
    qh::provider::ProviderClaude p("k", svr.baseUrl(), "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK(r.error.empty());
    QH_CHECK_EQ(r.message._content, std::string("thinking..."));
    QH_CHECK_EQ(r.message._toolCalls.size(), (size_t)1);
    QH_CHECK_EQ(r.message._toolCalls[0]._name, std::string("bash"));
    QH_CHECK_EQ(r.message._toolCalls[0]._arguments, std::string(R"({"command":"ls"})"));
}

QH_TEST(ProviderClaude_request_system_and_maxtokens) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/messages", 200, kClaudeTextResp);
    qh::provider::ProviderClaude p("k", svr.baseUrl(), "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;

    std::vector<qh::schema::Message> msgs(2);
    msgs[0]._role = qh::schema::Role::System; msgs[0]._content = "be nice";
    msgs[1]._role = qh::schema::Role::User;   msgs[1]._content = "hi";
    p.generate(cancel, msgs, {});

    json sent = json::parse(svr.lastBody(), nullptr, false);
    QH_CHECK_EQ(sent["model"].get<std::string>(), std::string("claude-sonnet-4-6"));
    QH_CHECK(sent.contains("max_tokens"));                 // 必填
    QH_CHECK_EQ(sent["max_tokens"].get<int>(), 4096);      // 默认
    QH_CHECK_EQ(sent["system"].get<std::string>(), std::string("be nice")); // 顶层
    // system 不进 messages 数组
    QH_CHECK(sent["messages"].is_array());
    for (const auto& mm : sent["messages"]) QH_CHECK(mm["role"] != "system");
}

QH_TEST(ProviderClaude_request_tool_result) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/messages", 200, kClaudeTextResp);
    qh::provider::ProviderClaude p("k", svr.baseUrl(), "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;

    qh::schema::Message tr;
    tr._role = qh::schema::Role::User; tr._toolCallId = "tu_1"; tr._content = "out";
    p.generate(cancel, {tr}, {});

    json sent = json::parse(svr.lastBody(), nullptr, false);
    const auto& content = sent["messages"][0]["content"];
    QH_CHECK(content.is_array());
    QH_CHECK_EQ(content[0]["type"].get<std::string>(), std::string("tool_result"));
    QH_CHECK_EQ(content[0]["tool_use_id"].get<std::string>(), std::string("tu_1"));
}

QH_TEST(ProviderClaude_http_error) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/messages", 500, R"({"error":{"message":"server boom"}})");
    qh::provider::ProviderClaude p("k", svr.baseUrl(), "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK(!r.error.empty());
    QH_CHECK(r.error.find("server boom") != std::string::npos);
}

QH_TEST(ProviderClaude_cancelled) {
    qh::provider::ProviderClaude p("k", "http://127.0.0.1:18181/v1", "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;
    cancel.cancel();
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK_EQ(r.error, std::string("cancelled"));
}

QH_TEST(ProviderClaude_request_tools) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/messages", 200, kClaudeTextResp);

    qh::provider::ProviderClaude p("k", svr.baseUrl(), "claude-sonnet-4-6");
    qh::schema::CancellationToken cancel;
    qh::schema::ToolDefinition td;
    td._name = "bash"; td._description = "run shell"; td._inputSchema = json::object();
    p.generate(cancel, {qh::schema::Message{}}, {td});

    json sent = json::parse(svr.lastBody(), nullptr, false);
    QH_CHECK(sent.contains("tools"));
    QH_CHECK_EQ(sent["tools"][0]["name"].get<std::string>(), std::string("bash"));
    QH_CHECK(sent["tools"][0].contains("input_schema"));
    QH_CHECK(sent["tool_choice"]["type"] == "auto");
}
