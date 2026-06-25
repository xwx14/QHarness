#include "provider/MockTwoStageProvider.h"

namespace qh {
namespace provider {

GenerateResult MockTwoStageProvider::generate(
    const schema::CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& tools) {
    GenerateResult result;
    result.message._role = schema::Role::Assistant;

    // tools 为空：Phase 1 慢思考阶段（引擎剥夺工具，强制纯文本推理）
    if (tools.empty()) {
        result.message._content =
            "【推理中】目标是检查文件。我不能直接盲猜，我需要先调用 bash 工具执行 "
            "ls 命令，看看当前目录下有什么，然后再做定夺。";
        return result;
    }

    // tools 非空：Phase 2 行动阶段
    ++_actionTurn;
    if (_actionTurn == 1) {
        // 第一轮 Action：顺着刚才的 Thinking，精准调用工具
        result.message._content = "我要执行我刚才计划的步骤了。";
        schema::ToolCall call;
        call._id = "call_123";
        call._name = "bash";
        call._arguments = "{\"command\": \"ls -la\"}";
        result.message._toolCalls.push_back(std::move(call));
    } else {
        // 第二轮 Action：直接总结退出
        result.message._content = "根据工具返回的结果，我看到了 main.go，任务圆满完成！";
    }
    return result;
}

} // namespace provider
} // namespace qh
