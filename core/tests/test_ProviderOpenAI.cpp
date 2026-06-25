#include "MockHttpServer.h"
#include "provider/ProviderOpenAI.h"
#include "schema/Message.h"
#include "schema/CancellationToken.h"
#include <nlohmann/json.hpp>
#include "TestHarness.h"

using json = nlohmann::json;

// 响应：纯文本
static const std::string kOpenaiTextResp = R"({
  "choices":[{"message":{"role":"assistant","content":"hello"},"finish_reason":"stop"}]
})";

// 响应：工具调用
static const std::string kOpenaiToolResp = R"({
  "choices":[{"message":{"role":"assistant","content":null,"tool_calls":[
    {"id":"call_1","type":"function","function":{"name":"bash","arguments":"{\"command\":\"ls\"}"}}
  ]},"finish_reason":"tool_calls"}]
})";

QH_TEST(ProviderOpenAI_text_response) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/chat/completions", 200, kOpenaiTextResp);

    qh::provider::ProviderOpenAI p("k", svr.baseUrl(), "gpt-5.4");
    qh::schema::CancellationToken cancel;
    std::vector<qh::schema::Message> msgs{qh::schema::Message{}};
    msgs[0]._role = qh::schema::Role::User;
    msgs[0]._content = "hi";

    auto r = p.generate(cancel, msgs, {});
    QH_CHECK(r.error.empty());
    QH_CHECK(r.message._role == qh::schema::Role::Assistant);
    QH_CHECK_EQ(r.message._content, std::string("hello"));
}

QH_TEST(ProviderOpenAI_tool_response) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/chat/completions", 200, kOpenaiToolResp);

    qh::provider::ProviderOpenAI p("k", svr.baseUrl(), "gpt-5.4");
    qh::schema::CancellationToken cancel;
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK(r.error.empty());
    QH_CHECK_EQ(r.message._toolCalls.size(), (size_t)1);
    QH_CHECK_EQ(r.message._toolCalls[0]._id, std::string("call_1"));
    QH_CHECK_EQ(r.message._toolCalls[0]._name, std::string("bash"));
    QH_CHECK_EQ(r.message._toolCalls[0]._arguments, std::string("{\"command\":\"ls\"}"));
}

QH_TEST(ProviderOpenAI_request_format) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/chat/completions", 200, kOpenaiTextResp);

    qh::provider::ProviderOpenAI p("SECRET", svr.baseUrl(), "gpt-5.4");
    qh::schema::CancellationToken cancel;
    std::vector<qh::schema::Message> msgs{qh::schema::Message{}};
    msgs[0]._role = qh::schema::Role::User;
    msgs[0]._content = "ping";

    qh::schema::ToolDefinition td;
    td._name = "bash"; td._description = "run shell"; td._inputSchema = json::object();
    p.generate(cancel, msgs, {td});

    json sent = json::parse(svr.lastBody(), nullptr, false);
    QH_CHECK(sent.contains("model"));
    QH_CHECK_EQ(sent["model"].get<std::string>(), std::string("gpt-5.4"));
    QH_CHECK(sent["messages"].is_array() && !sent["messages"].empty());
    QH_CHECK(sent.contains("tools") && sent["tools"][0]["function"]["name"] == "bash");
    QH_CHECK(sent["tool_choice"] == "auto");
}

QH_TEST(ProviderOpenAI_tool_result_message) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/chat/completions", 200, kOpenaiTextResp);

    qh::provider::ProviderOpenAI p("k", svr.baseUrl(), "gpt-5.4");
    qh::schema::CancellationToken cancel;
    qh::schema::Message tr;
    tr._role = qh::schema::Role::User;
    tr._toolCallId = "call_1";
    tr._content = "file.txt";
    p.generate(cancel, {tr}, {});

    json sent = json::parse(svr.lastBody(), nullptr, false);
    QH_CHECK_EQ(sent["messages"][0]["role"].get<std::string>(), std::string("tool"));
    QH_CHECK_EQ(sent["messages"][0]["tool_call_id"].get<std::string>(), std::string("call_1"));
}

QH_TEST(ProviderOpenAI_http_error) {
    qh::test::MockHttpServer svr; QH_CHECK(svr.start());
    svr.onPost("/v1/chat/completions", 401, R"({"error":{"message":"invalid key"}})");

    qh::provider::ProviderOpenAI p("k", svr.baseUrl(), "gpt-5.4");
    qh::schema::CancellationToken cancel;
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK(!r.error.empty());
    QH_CHECK(r.error.find("invalid key") != std::string::npos);
}

QH_TEST(ProviderOpenAI_cancelled) {
    qh::provider::ProviderOpenAI p("k", "http://127.0.0.1:18181/v1", "gpt-5.4");
    qh::schema::CancellationToken cancel;
    cancel.cancel();
    auto r = p.generate(cancel, {qh::schema::Message{}}, {});
    QH_CHECK_EQ(r.error, std::string("cancelled"));
}
