#include "TestHarness.h"
#include "tool/MockBashTool.h"
#include "schema/Message.h"
#include <string>

QH_TEST(mockbashtool_definition_name_is_bash) {
    qh::tool::MockBashTool t;
    QH_CHECK_EQ(t.definition()._name, std::string("bash"));
}

QH_TEST(mockbashtool_execute_returns_fixed_output) {
    qh::tool::MockBashTool t;
    qh::schema::ToolCall call;
    call._id = "call_42";
    call._name = "bash";
    call._arguments = "{\"command\":\"ls -la\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK_EQ(r._toolCallId, std::string("call_42"));
    QH_CHECK_EQ(r._output, std::string("-rw-r--r-- 1 user group 234 Oct 24 10:00 main.go\n"));
    QH_CHECK(!r._isError);
}
