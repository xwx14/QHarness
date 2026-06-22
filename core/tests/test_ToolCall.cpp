#include "TestHarness.h"
#include "schema/ToolCall.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(toolcall_roundtrip_raw_json) {
    qh::schema::ToolCall tc;
    tc.id = "call_1";
    tc.name = "bash";
    tc.arguments = R"({"command":"ls -la"})";

    json j = tc;                                          // to_json
    QH_CHECK_EQ(std::string(j["id"]), std::string("call_1"));
    QH_CHECK(j["arguments"].is_object());                 // 原始JSON对象，非字符串
    QH_CHECK_EQ(std::string(j["arguments"]["command"]), std::string("ls -la"));

    auto back = j.get<qh::schema::ToolCall>();            // from_json
    QH_CHECK_EQ(back.id, std::string("call_1"));
    QH_CHECK_EQ(back.name, std::string("bash"));
    QH_CHECK_EQ(back.arguments, std::string(R"({"command":"ls -la"})"));
}

QH_TEST(toolcall_parse_arguments) {
    qh::schema::ToolCall tc;
    tc.arguments = R"({"x":1,"y":[2,3]})";
    auto parsed = tc.parseArguments();
    QH_CHECK_EQ(parsed["x"].get<int>(), 1);
    QH_CHECK_EQ(parsed["y"][0].get<int>(), 2);
}
