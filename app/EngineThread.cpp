#include "EngineThread.h"

namespace qh {
namespace app {

EngineThread::EngineThread(qh::engine::Engine* engine, QPostMessage* postMessage,
                           std::string prompt, QObject* parent)
    : QThread(parent), _engine(engine), _postMessage(postMessage), _prompt(std::move(prompt)) {
    _engine->setPostMessage(_postMessage);   // 复用 MainWindow._postMessage
}

void EngineThread::run() {
    _engine->run(_prompt);   // worker 线程执行 ReAct 循环
}

} // namespace app
} // namespace qh
