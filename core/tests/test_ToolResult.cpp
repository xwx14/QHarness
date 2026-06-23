#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(toolresult_roundtrip) {
    qh::schema::ToolResult tr;
    tr._toolCallId = "call_1";
    tr._output = "total 0";
    tr._isError = false;

    json j = tr;
    QH_CHECK_EQ(std::string(j["tool_call_id"]), std::string("call_1"));
    QH_CHECK_EQ(std::string(j["output"]), std::string("total 0"));
    QH_CHECK(j["is_error"].get<bool>() == false);

    auto back = j.get<qh::schema::ToolResult>();
    QH_CHECK_EQ(back._toolCallId, std::string("call_1"));
    QH_CHECK(back._isError == false);
}
