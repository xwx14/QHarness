#include "engine/EngineReActLoop.h"
#include "provider/Provider.h"
#include "tool/ToolManager.h"
#include "schema/Message.h"
#include "schema/CancellationToken.h"
#include <string>
#include <vector>

namespace qh {
namespace engine {

void EngineReActLoop::run(const std::string& userPrompt) {
    info("引擎启动，锁定工作区: " + _workDir);

    // 1. 初始化会话上下文（system + user）；真实场景后续由 Composer 组装
    std::vector<schema::Message> history = {
        {schema::Role::System, "You are go-tiny-claw, an expert coding assistant. You have full access to tools in the workspace."},
        {schema::Role::User, userPrompt},
    };

    // 2. ReAct 主循环：思考 → 行动 → 观察，直到模型不再请求工具
    for (int turn = 1; ; ++turn) {
        info("========== [Turn " + std::to_string(turn) + "] 开始 ==========");

        schema::CancellationToken token;
        std::vector<schema::ToolDefinition> tools = _toolManager->getAvailableTools();

        info("正在思考 (Reasoning)...");
        provider::GenerateResult result = _provider->generate(token, history, tools);
        if (!result.error.empty()) {
            error("模型生成失败: " + result.error);
            return;
        }

        history.push_back(result.message);
        if (!result.message._content.empty()) {
            chat(result.message._content);
        }

        // 退出条件：模型未请求任何工具，任务完成
        if (result.message._toolCalls.empty()) {
            info("任务完成，退出循环。");
            break;
        }

        info("模型请求调用 " + std::to_string(result.message._toolCalls.size()) + " 个工具...");
        for (const auto& call : result.message._toolCalls) {
            info("  -> 执行工具: " + call._name + ", 参数: " + call._arguments);
            schema::ToolResult toolResult = _toolManager->execute(call);
            if (toolResult._isError) {
                error("  -> 工具执行报错: " + toolResult._output);
            } else {
                info("  -> 工具执行成功 (返回 " + std::to_string(toolResult._output.size()) + " 字节)");
            }
            // 观察（Observation）回填为 User 消息，携带 ToolCallID 维系推理链
            schema::Message observation;
            observation._role = schema::Role::User;
            observation._content = toolResult._output;
            observation._toolCallId = call._id;
            history.push_back(observation);
        }
    }
}

} // namespace engine
} // namespace qh
