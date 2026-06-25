#include "TestHarness.h"
#include "engine/EngineReActLoop.h"
#include "provider/MockProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include "schema/PostMessage.h"
#include "schema/Message.h"
#include <string>
#include <vector>

namespace {

// 收集 PostMessage 消息的 fake
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

bool hasLevelContaining(const FakePostMessage& pm, qh::schema::Level level, const std::string& sub) {
    for (size_t i = 0; i < pm._messages.size(); ++i) {
        if (pm._levels[i] == level && pm._messages[i].find(sub) != std::string::npos) return true;
    }
    return false;
}

} // namespace

QH_TEST(enginereactloop_runs_react_cycle_with_mock) {
    qh::provider::MockProvider provider;       // turn 计数：第1轮 bash 调用，第2轮纯文本
    qh::tool::MockBashTool bash;
    qh::tool::ToolManager tm;
    QH_CHECK(tm.registerTool(bash));

    qh::engine::EngineReActLoop engine(&provider, &tm, "./");
    FakePostMessage pm;
    engine.setPostMessage(&pm);

    engine.run("帮我检查当前目录的文件");

    // 循环正常完成：收到"任务完成"Info 消息
    QH_CHECK(hasInfoContaining(pm, "任务完成"));
    // 第1轮触发了工具执行
    QH_CHECK(hasInfoContaining(pm, "执行工具"));
    // 模型对外回复走 Chat 级别（第2轮完成文本）
    QH_CHECK(hasLevelContaining(pm, qh::schema::Level::Chat, "看到了文件"));
    // generate 与工具均成功，无 error
    QH_CHECK(!hasAnyError(pm));
}
