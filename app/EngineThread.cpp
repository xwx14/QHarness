#include "EngineThread.h"
#include "engine/EngineReActLoop.h"   // make_unique<EngineReActLoop> 需完整类型
#include "engine/Engine.h"            // setPostMessage
#include "provider/MockProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include <utility>   // std::move

namespace qh {
namespace app {

EngineThread::EngineThread(QPostMessage* postMessage, std::string prompt,
                           std::string workDir, QObject* parent)
    : QThread(parent), _postMessage(postMessage), _prompt(std::move(prompt)) {
    // 组装本次运行的 mock+engine 子树（每次 new 一套，MockProvider turn 计数天然重置）
    _mockProvider = std::make_unique<qh::provider::MockProvider>();
    _mockBash     = std::make_unique<qh::tool::MockBashTool>();
    _toolManager  = std::make_unique<qh::tool::ToolManager>();
    _toolManager->registerTool(*_mockBash);
    _engine = std::make_unique<qh::engine::EngineReActLoop>(
        _mockProvider.get(), _toolManager.get(), std::move(workDir));
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
