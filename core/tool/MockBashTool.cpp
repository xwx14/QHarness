#include "tool/MockBashTool.h"
#include <nlohmann/json.hpp>

namespace qh {
namespace tool {



MockBashTool::MockBashTool()
{
	_definition._name = "bash";
	_definition._description = "执行 shell 命令（mock 实现，返回固定输出）";
	// bash 工具典型 schema：command 字符串参数
	_definition._inputSchema = nlohmann::json::object();
	_definition._inputSchema["type"] = "object";
	_definition._inputSchema["properties"] = nlohmann::json::object();
	_definition._inputSchema["properties"]["command"] = nlohmann::json::object();
	_definition._inputSchema["properties"]["command"]["type"] = "string";
	_definition._inputSchema["required"] = nlohmann::json::array({ "command" });
}

schema::ToolResult MockBashTool::execute(const schema::ToolCall& call) {
    schema::ToolResult r;
    r._toolCallId = call._id;
    r._output = "-rw-r--r-- 1 user group 234 Oct 24 10:00 main.go\n";
    r._isError = false;
    return r;
}

} // namespace tool
} // namespace qh
