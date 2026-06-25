#include "EngineThread.h"
#include "engine/Engine.h"             // 基类 + setPostMessage
#include "engine/EngineReActLoop.h"    // make_unique<EngineReActLoop>
#include "engine/Engine2StageReAct.h"  // make_unique<Engine2StageReAct>
#include "provider/Provider.h"         // 基类
#include "provider/MockProvider.h"
#include "provider/MockTwoStageProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include <utility>   // std::move

namespace qh {
namespace app {

EngineThread::EngineThread(QPostMessage* postMessage, std::string prompt,
                           std::string workDir, EngineKind kind, QObject* parent)
    : QThread(parent), _postMessage(postMessage), _prompt(std::move(prompt)) {
    // 公共工具子树（两种引擎都用）
    _mockBash    = std::make_unique<qh::tool::MockBashTool>();
    _toolManager = std::make_unique<qh::tool::ToolManager>();
    _toolManager->registerTool(*_mockBash);

    // 按 kind 创建对应的 mock + engine（每次 new 一套，turn 计数天然重置）
    if (kind == EngineKind::TwoStageReAct) {
        _mockProvider = std::make_unique<qh::provider::MockTwoStageProvider>();
        _engine = std::make_unique<qh::engine::Engine2StageReAct>(
            _mockProvider.get(), _toolManager.get(), std::move(workDir));
    } else {
        _mockProvider = std::make_unique<qh::provider::MockProvider>();
        _engine = std::make_unique<qh::engine::EngineReActLoop>(
            _mockProvider.get(), _toolManager.get(), std::move(workDir));
    }
    _engine->setPostMessage(_postMessage);   // 复用 MainWindow._postMessage
}

EngineThread::~EngineThread() {
    // 先等 worker 线程退出（其引用本对象的 _engine 等成员），再由成员逆序析构销毁 engine 子树
    wait();
}

void EngineThread::run() {
    _engine->run(_prompt);   // worker 线程执行 ReAct 循环
}

} // namespace app
} // namespace qh
