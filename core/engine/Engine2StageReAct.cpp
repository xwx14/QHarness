#include "engine/Engine2StageReAct.h"
#include "provider/Provider.h"
#include "tool/ToolManager.h"
#include "schema/Message.h"
#include "schema/CancellationToken.h"
#include <string>
#include <utility>
#include <vector>

namespace qh {
namespace engine {

Engine2StageReAct::Engine2StageReAct(provider::Provider* provider,
                                     tool::ToolManager* toolManager,
                                     std::string workDir, bool enableThinking)
    : Engine(provider, toolManager, std::move(workDir)), _enableThinking(enableThinking) {}

void Engine2StageReAct::setEnableThinking(bool enable) {
    _enableThinking = enable;
}

void Engine2StageReAct::run(const std::string& userPrompt) {
    info("引擎启动，锁定工作区: " + _workDir);
    info(_enableThinking ? "慢思考模式 (Thinking Phase): 开启"
                         : "慢思考模式 (Thinking Phase): 关闭");

    // 初始化会话上下文（system + user）
    std::vector<schema::Message> history = {
        {schema::Role::System, "You are go-tiny-claw, an expert coding assistant. You have full access to tools in the workspace."},
        {schema::Role::User, userPrompt},
    };

    for (int turn = 1; ; ++turn) {
        info("========== [Turn " + std::to_string(turn) + "] 开始 ==========");
        schema::CancellationToken token;
        std::vector<schema::ToolDefinition> tools = _toolManager->getAvailableTools();

        // ===== Phase 1: 慢思考（仅当 _enableThinking）——剥夺工具，强制纯文本规划 =====
        if (_enableThinking) {
            info("[Phase 1] 剥夺工具访问权，强制进入慢思考与规划阶段...");
            provider::GenerateResult thinkResult = _provider->generate(token, history, {});
            if (!thinkResult.error.empty()) {
                error("Thinking 阶段生成失败: " + thinkResult.error);
                return;
            }
            if (!thinkResult.message._content.empty()) {
                think(thinkResult.message._content);
                history.push_back(thinkResult.message);
            }
        }

        // ===== Phase 2: 行动——恢复工具挂载，顺着规划执行 =====
        info("[Phase 2] 恢复工具挂载，等待模型采取行动...");
        provider::GenerateResult actionResult = _provider->generate(token, history, tools);
        if (!actionResult.error.empty()) {
            error("Action 阶段生成失败: " + actionResult.error);
            return;
        }
        history.push_back(actionResult.message);
        if (!actionResult.message._content.empty()) {
            chat(actionResult.message._content);
        }

        // 退出条件：模型未请求工具，任务完成
        if (actionResult.message._toolCalls.empty()) {
            info("模型未请求调用工具，任务完成。");
            break;
        }

        // 执行工具 + 观察（Observation）回填为 User 消息，携带 ToolCallID 维系推理链
        const auto& calls = actionResult.message._toolCalls;
        info("模型请求调用 " + std::to_string(calls.size()) + " 个工具，并发度="
             + (_maxToolConcurrency <= 0 ? std::string("无上限") : std::to_string(_maxToolConcurrency)));
        std::vector<schema::ToolResult> toolResults = _toolManager->executeAll(calls, _maxToolConcurrency);
        for (size_t i = 0; i < calls.size(); ++i) {
            const auto& tr = toolResults[i];
            if (tr._isError) error("  -> 工具[" + calls[i]._name + "] 报错: " + tr._output);
            else info("  -> 工具[" + calls[i]._name + "] 成功 (返回 "
                      + std::to_string(tr._output.size()) + " 字节)");
            schema::Message observation;
            observation._role = schema::Role::User;
            observation._content = tr._output;
            observation._toolCallId = calls[i]._id;
            history.push_back(observation);
        }
    }
}

} // namespace engine
} // namespace qh
