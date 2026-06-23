#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(message_omitempty_plain) {
    qh::schema::Message m;
    m._role = qh::schema::Role::User;
    m._content = "hello";

    json j = m;
    QH_CHECK_EQ(std::string(j["role"]), std::string("user"));
    QH_CHECK_EQ(std::string(j["content"]), std::string("hello"));
    QH_CHECK(!j.contains("tool_calls"));   // omitempty
    QH_CHECK(!j.contains("tool_call_id")); // omitempty

    auto back = j.get<qh::schema::Message>();
    QH_CHECK(back._role == qh::schema::Role::User);
    QH_CHECK(back._toolCalls.empty());
}

QH_TEST(message_with_toolcalls) {
    qh::schema::Message m;
    m._role = qh::schema::Role::Assistant;
    m._content = "thinking...";
    qh::schema::ToolCall tc;
    tc._id = "call_1";
    tc._name = "bash";
    tc._arguments = R"({"command":"pwd"})";
    m._toolCalls.push_back(tc);

    json j = m;
    QH_CHECK(j.contains("tool_calls"));
    QH_CHECK_EQ(j["tool_calls"].size(), (size_t)1);
    QH_CHECK(j["tool_calls"][0]["arguments"].is_object());

    auto back = j.get<qh::schema::Message>();
    QH_CHECK_EQ(back._toolCalls.size(), (size_t)1);
    QH_CHECK_EQ(back._toolCalls[0]._name, std::string("bash"));
    QH_CHECK_EQ(back._toolCalls[0]._arguments, std::string(R"({"command":"pwd"})"));
}

QH_TEST(message_tool_response) {
    qh::schema::Message m;
    m._role = qh::schema::Role::User;
    m._content = "/home/user";
    m._toolCallId = "call_1";

    json j = m;
    QH_CHECK(j.contains("tool_call_id"));
    QH_CHECK_EQ(std::string(j["tool_call_id"]), std::string("call_1"));
}
