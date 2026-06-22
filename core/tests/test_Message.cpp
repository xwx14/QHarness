#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(message_omitempty_plain) {
    qh::schema::Message m;
    m.role = qh::schema::Role::User;
    m.content = "hello";

    json j = m;
    QH_CHECK_EQ(std::string(j["role"]), std::string("user"));
    QH_CHECK_EQ(std::string(j["content"]), std::string("hello"));
    QH_CHECK(!j.contains("tool_calls"));   // omitempty
    QH_CHECK(!j.contains("tool_call_id")); // omitempty

    auto back = j.get<qh::schema::Message>();
    QH_CHECK(back.role == qh::schema::Role::User);
    QH_CHECK(back.toolCalls.empty());
}

QH_TEST(message_with_toolcalls) {
    qh::schema::Message m;
    m.role = qh::schema::Role::Assistant;
    m.content = "thinking...";
    qh::schema::ToolCall tc;
    tc.id = "call_1";
    tc.name = "bash";
    tc.arguments = R"({"command":"pwd"})";
    m.toolCalls.push_back(tc);

    json j = m;
    QH_CHECK(j.contains("tool_calls"));
    QH_CHECK_EQ(j["tool_calls"].size(), (size_t)1);
    QH_CHECK(j["tool_calls"][0]["arguments"].is_object());

    auto back = j.get<qh::schema::Message>();
    QH_CHECK_EQ(back.toolCalls.size(), (size_t)1);
    QH_CHECK_EQ(back.toolCalls[0].name, std::string("bash"));
}

QH_TEST(message_tool_response) {
    qh::schema::Message m;
    m.role = qh::schema::Role::User;
    m.content = "/home/user";
    m.toolCallId = "call_1";

    json j = m;
    QH_CHECK(j.contains("tool_call_id"));
    QH_CHECK_EQ(std::string(j["tool_call_id"]), std::string("call_1"));
}
