#include "provider/MockProvider.h"

namespace qh {
namespace provider {

GenerateResult MockProvider::generate(
    const schema::CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& /*tools*/) {
    ++_turn;
    GenerateResult result;
    result.message._role = schema::Role::Assistant;

    if (_turn == 1) {
        result.message._content = "让我来看看当前目录下有什么文件。";
        schema::ToolCall call;
        call._id = "call_123";
        call._name = "bash";
        call._arguments = "{\"command\": \"ls -la\"}";
        result.message._toolCalls.push_back(std::move(call));
    } else {
        result.message._content = "我看到了文件列表，里面包含 main.go，任务完成！";
    }
    return result;
}

} // namespace provider
} // namespace qh
