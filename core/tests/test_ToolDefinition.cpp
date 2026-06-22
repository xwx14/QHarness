#include "TestHarness.h"
#include "schema/ToolDefinition.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(tooldefinition_roundtrip) {
    qh::schema::ToolDefinition td;
    td.name = "bash";
    td.description = "执行 shell 命令";
    td.inputSchema = json::parse(R"({"type":"object","properties":{"command":{"type":"string"}}})");

    json j = td;
    QH_CHECK_EQ(std::string(j["name"]), std::string("bash"));
    QH_CHECK_EQ(std::string(j["description"]), std::string("执行 shell 命令"));
    QH_CHECK(j["input_schema"]["type"].get<std::string>() == "object");

    auto back = j.get<qh::schema::ToolDefinition>();
    QH_CHECK_EQ(back.name, std::string("bash"));
    QH_CHECK(back.inputSchema["properties"]["command"]["type"].get<std::string>() == "string");
}
