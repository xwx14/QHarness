#ifndef QH_APP_ENGINETHREAD_H
#define QH_APP_ENGINETHREAD_H
#include <QThread>
#include <memory>
#include <string>
#include "QPostMessage.h"
#include "schema/Settings.h"

namespace qh {
namespace provider { class Provider; }   // 基类（OpenAI/Claude/Mock 之一）
namespace tool { class MockBashTool; class ToolManager; }
namespace engine { class Engine; }       // 基类（EngineReActLoop 或 Engine2StageReAct）

namespace app {

// 异步引擎线程：在 worker 线程跑 engine->run(prompt)，引擎日志经 _postMessage 跨线程投递到 LogDock/ChatDock
// 按 EngineKind + Settings 创建 provider+engine 子树，自洽生命周期；
// _postMessage 非拥有（MainWindow 持有，跨多次运行复用）；_settings 为值拷贝，worker 线程只读，与 UI 零共享。
class EngineThread : public QThread {
    Q_OBJECT
public:
    enum class EngineKind { ReAct, TwoStageReAct };

    // 构造内按 kind+settings 创建 provider+engine 子树并组装依赖链；postMessage 非拥有，须长于本线程
    EngineThread(QPostMessage* postMessage, std::string prompt,
                 schema::Settings settings, EngineKind kind, QObject* parent = nullptr);
    ~EngineThread();   // .cpp 实现：wait() 后成员逆序析构（unique_ptr 需完整类型）
    void run() override;   // worker 线程入口

private:
    // 声明顺序决定析构逆序 = _engine→_toolManager→_bashTool→_provider
    // 保证引用者先于被引用者析构（_engine/_toolManager 持有下层 raw 引用）
    std::unique_ptr<qh::provider::Provider> _provider;
    std::unique_ptr<qh::tool::MockBashTool> _bashTool;
    std::unique_ptr<qh::tool::ToolManager>  _toolManager;
    std::unique_ptr<qh::engine::Engine>     _engine;
    QPostMessage* _postMessage = nullptr;   // 非拥有（MainWindow._postMessage）
    std::string _prompt;
    schema::Settings _settings;
};

} // namespace app
} // namespace qh
#endif // QH_APP_ENGINETHREAD_H
