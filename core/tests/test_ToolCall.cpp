#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(toolcall_roundtrip_raw_json) {
    qh::schema::ToolCall tc;
    tc._id = "call_1";
    tc._name = "bash";
    tc._arguments = R"({"command":"ls -la"})";

    json j = tc;                                          // to_json
    QH_CHECK_EQ(std::string(j["id"]), std::string("call_1"));
    QH_CHECK(j["arguments"].is_object());                 // 原始JSON对象，非字符串
    QH_CHECK_EQ(std::string(j["arguments"]["command"]), std::string("ls -la"));

    auto back = j.get<qh::schema::ToolCall>();            // from_json
    QH_CHECK_EQ(back._id, std::string("call_1"));
    QH_CHECK_EQ(back._name, std::string("bash"));
    QH_CHECK_EQ(back._arguments, std::string(R"({"command":"ls -la"})"));
}

QH_TEST(toolcall_parse_arguments) {
    qh::schema::ToolCall tc;
    tc._arguments = R"({"x":1,"y":[2,3]})";
    auto parsed = tc.parseArguments();
    QH_CHECK_EQ(parsed["x"].get<int>(), 1);
    QH_CHECK_EQ(parsed["y"][0].get<int>(), 2);
}

QH_TEST(toolcall_invalid_arguments_nothrow) {
    qh::schema::ToolCall tc;
    tc._id = "call_err";
    tc._name = "bad_tool";
    tc._arguments = "not a json"; // 非法 JSON

    // 容错：to_json 对非法 arguments 不抛异常
    bool threw = false;
    json j;
    try {
        j = tc; // 隐式调用 to_json
    } catch (...) {
        threw = true;
    }
    QH_CHECK(!threw);
    QH_CHECK(j["arguments"].is_object()); // 退化为空对象
    QH_CHECK(j["arguments"].empty());
}
