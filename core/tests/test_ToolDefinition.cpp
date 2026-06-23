#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(tooldefinition_roundtrip) {
    qh::schema::ToolDefinition td;
    td._name = "bash";
    td._description = "执行 shell 命令";
    td._inputSchema = json::parse(R"({"type":"object","properties":{"command":{"type":"string"}}})");

    json j = td;
    QH_CHECK_EQ(std::string(j["name"]), std::string("bash"));
    QH_CHECK_EQ(std::string(j["description"]), std::string("执行 shell 命令"));
    QH_CHECK(j["input_schema"]["type"].get<std::string>() == "object");

    auto back = j.get<qh::schema::ToolDefinition>();
    QH_CHECK_EQ(back._name, std::string("bash"));
    QH_CHECK(back._inputSchema["properties"]["command"]["type"].get<std::string>() == "string");
}
