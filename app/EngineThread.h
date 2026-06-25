#ifndef QH_APP_ENGINETHREAD_H
#define QH_APP_ENGINETHREAD_H
#include <QThread>
#include <memory>
#include <string>
#include "QPostMessage.h"

namespace qh {
namespace provider { class MockProvider; }
namespace tool { class MockBashTool; class ToolManager; }
namespace engine { class EngineReActLoop; }

namespace app {

// 异步引擎线程：在 worker 线程跑 engine->run(prompt)，引擎日志经 _postMessage 跨线程投递到 LogDock
// 拥有本次运行的 mock+engine 子树（_mockProvider/_mockBash/_toolManager/_engine），自洽生命周期；
// _postMessage 非拥有（MainWindow 持有，跨多次运行复用）
class EngineThread : public QThread {
    Q_OBJECT
public:
    // 构造内创建 mock+engine 子树并组装依赖链；postMessage 非拥有，须长于本线程
    EngineThread(QPostMessage* postMessage, std::string prompt,
                 std::string workDir, QObject* parent = nullptr);
    ~EngineThread();   // .cpp 实现：wait() 后成员逆序析构（unique_ptr 需完整类型）
    void run() override;   // worker 线程入口

private:
    // 声明顺序决定析构逆序 = _engine→_toolManager→_mockBash→_mockProvider
    // 保证引用者先于被引用者析构（_engine/_toolManager 持有下层 raw 引用）
    std::unique_ptr<qh::provider::MockProvider>  _mockProvider;
    std::unique_ptr<qh::tool::MockBashTool>      _mockBash;
    std::unique_ptr<qh::tool::ToolManager>       _toolManager;
    std::unique_ptr<qh::engine::EngineReActLoop> _engine;
    QPostMessage* _postMessage = nullptr;   // 非拥有（MainWindow._postMessage）
    std::string _prompt;
};

} // namespace app
} // namespace qh
#endif // QH_APP_ENGINETHREAD_H
