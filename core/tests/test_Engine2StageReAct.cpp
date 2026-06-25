#include "TestHarness.h"
#include "engine/Engine2StageReAct.h"
#include "provider/MockTwoStageProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include "schema/PostMessage.h"
#include "schema/Message.h"
#include <string>
#include <vector>

namespace {

// 收集 PostMessage 消息的 fake（复用 test_EngineReActLoop 的模式）
class FakePostMessage : public qh::schema::PostMessage {
public:
    void post(qh::schema::Level level, const std::string& msg) override {
        _levels.push_back(level);
        _messages.push_back(msg);
    }
    std::vector<qh::schema::Level> _levels;
    std::vector<std::string> _messages;
};

bool hasInfoContaining(const FakePostMessage& pm, const std::string& sub) {
    for (size_t i = 0; i < pm._messages.size(); ++i) {
        if (pm._levels[i] == qh::schema::Level::Info &&
            pm._messages[i].find(sub) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool hasAnyError(const FakePostMessage& pm) {
    for (size_t i = 0; i < pm._levels.size(); ++i) {
        if (pm._levels[i] == qh::schema::Level::Error) return true;
    }
    return false;
}

} // namespace

// enableThinking 默认 true：应同时出现思考 trace、工具执行、任务完成
QH_TEST(engine2stage_thinking_on_runs_two_phases) {
    qh::provider::MockTwoStageProvider provider;
    qh::tool::MockBashTool bash;
    qh::tool::ToolManager tm;
    tm.registerTool(bash);

    qh::engine::Engine2StageReAct engine(&provider, &tm, "./");   // enableThinking 默认 true
    FakePostMessage pm;
    engine.setPostMessage(&pm);

    engine.run("帮我检查当前目录的文件");

    QH_CHECK(hasInfoContaining(pm, "内部思考 Trace"));   // Phase1 产出
    QH_CHECK(hasInfoContaining(pm, "执行工具"));          // Phase2 工具调用
    QH_CHECK(hasInfoContaining(pm, "任务完成"));          // 循环正常退出
    QH_CHECK(!hasAnyError(pm));
}

// enableThinking=false：跳过 Phase1，无思考 trace，仍完成 ReAct 循环
QH_TEST(engine2stage_thinking_off_behaves_like_react) {
    qh::provider::MockTwoStageProvider provider;
    qh::tool::MockBashTool bash;
    qh::tool::ToolManager tm;
    tm.registerTool(bash);

    qh::engine::Engine2StageReAct engine(&provider, &tm, "./", false);   // 显式关闭
    FakePostMessage pm;
    engine.setPostMessage(&pm);

    engine.run("帮我检查当前目录的文件");

    QH_CHECK(!hasInfoContaining(pm, "内部思考 Trace"));   // 关闭则无 Phase1
    QH_CHECK(hasInfoContaining(pm, "执行工具"));
    QH_CHECK(hasInfoContaining(pm, "任务完成"));
    QH_CHECK(!hasAnyError(pm));
}
