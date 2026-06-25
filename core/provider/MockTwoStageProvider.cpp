#include "provider/MockTwoStageProvider.h"

namespace qh {
namespace provider {

GenerateResult MockTwoStageProvider::generate(
    const schema::CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& tools) {
    GenerateResult result;
    result.message._role = schema::Role::Assistant;

    if (tools.empty()) {
        // Phase 1 慢思考：剥夺工具，纯文本规划
        result.message._content = "让我先分析一下任务，规划执行步骤。";
    } else {
        // Phase 2 行动：第1次 bash 调用，第2次完成文本
        ++_actionTurn;
        if (_actionTurn == 1) {
            schema::ToolCall call;
            call._id = "call_2stage_1";
            call._name = "bash";
            call._arguments = "{\"command\": \"ls -la\"}";
            result.message._toolCalls.push_back(std::move(call));
        } else {
            result.message._content = "我看到了文件列表，任务完成！";
        }
    }
    return result;
}

} // namespace provider
} // namespace qh
