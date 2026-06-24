#ifndef QH_APP_ENGINETHREAD_H
#define QH_APP_ENGINETHREAD_H
#include <QThread>
#include <string>
#include "engine/Engine.h"
#include "QPostMessage.h"

namespace qh {
namespace app {

// 异步引擎线程：在 worker 线程跑 engine->run(prompt)，引擎日志经 _postMessage 跨线程投递到 LogDock
// engine/postMessage 均非拥有（调用方保证生命周期长于本线程）
class EngineThread : public QThread {
    Q_OBJECT
public:
    EngineThread(qh::engine::Engine* engine, QPostMessage* postMessage,
                 std::string prompt, QObject* parent = nullptr);
    void run() override;   // worker 线程入口

private:
    qh::engine::Engine* _engine;     // 非拥有
    QPostMessage* _postMessage;      // 非拥有（MainWindow._postMessage）
    std::string _prompt;
};

} // namespace app
} // namespace qh
#endif // QH_APP_ENGINETHREAD_H
