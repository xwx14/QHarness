#include "TestHarness.h"
#include "tool/Tool.h"
#include "tool/ToolManager.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

namespace {

// 测试用桩工具：definition() 返回固定定义，execute() 回填固定输出
class FakeTool : public qh::tool::Tool {
public:
    FakeTool(std::string name, std::string output = "ok")
        : name_(std::move(name)), output_(std::move(output)) {}

    qh::schema::ToolDefinition definition() const override {
        qh::schema::ToolDefinition d;
        d.name = name_;
        d.description = "fake tool for test";
        d.inputSchema = json::object();
        return d;
    }

    qh::schema::ToolResult execute(const qh::schema::ToolCall& call) override {
        qh::schema::ToolResult r;
        r.toolCallId = call.id;
        r.output = output_;
        r.isError = false;
        return r;
    }

private:
    std::string name_;
    std::string output_;
};

// 构造一个带 id/name 的 ToolCall，参数给空 JSON 对象
qh::schema::ToolCall makeCall(const std::string& id, const std::string& name) {
    qh::schema::ToolCall c;
    c.id = id;
    c.name = name;
    c.arguments = "{}";
    return c;
}

} // namespace

QH_TEST(toolmanager_register_and_lookup) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");

    QH_CHECK(tm.registerTool(bash));
    QH_CHECK(tm.hasTool("bash"));
    qh::tool::Tool* found = tm.getTool("bash");
    QH_CHECK(found != nullptr);
    if (found) {
        QH_CHECK_EQ(std::string(found->definition().name), std::string("bash"));
    }
}

QH_TEST(toolmanager_register_duplicate_not_replaced) {
    qh::tool::ToolManager tm;
    FakeTool a("dup", "a-out");
    FakeTool b("dup", "b-out");

    QH_CHECK(tm.registerTool(a));
    QH_CHECK(!tm.registerTool(b));  // 重名返回 false，不覆盖

    // 执行应走 a（未被 b 替换）
    auto r = tm.execute(makeCall("c1", "dup"));
    QH_CHECK_EQ(r.output, std::string("a-out"));
}

QH_TEST(toolmanager_list_definitions) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");
    FakeTool edit("edit");
    tm.registerTool(bash);
    tm.registerTool(edit);

    auto defs = tm.getAvailableTools();
    QH_CHECK_EQ(defs.size(), static_cast<size_t>(2));

    // 集合语义：包含 bash 和 edit（unordered_map 顺序不保证）
    bool hasBash = false, hasEdit = false;
    for (const auto& d : defs) {
        if (d.name == "bash") hasBash = true;
        if (d.name == "edit") hasEdit = true;
    }
    QH_CHECK(hasBash);
    QH_CHECK(hasEdit);
}

QH_TEST(toolmanager_execute_dispatches_to_tool) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash", "ls-output");
    tm.registerTool(bash);

    auto r = tm.execute(makeCall("call_1", "bash"));
    QH_CHECK_EQ(r.toolCallId, std::string("call_1"));
    QH_CHECK_EQ(r.output, std::string("ls-output"));
    QH_CHECK(!r.isError);
}

QH_TEST(toolmanager_execute_unknown_returns_error) {
    qh::tool::ToolManager tm;
    auto r = tm.execute(makeCall("call_2", "nope"));

    QH_CHECK(r.isError);
    QH_CHECK_EQ(r.toolCallId, std::string("call_2"));
    QH_CHECK(r.output.find("nope") != std::string::npos);  // 错误信息含工具名
}
