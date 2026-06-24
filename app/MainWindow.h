#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include <memory>
#include "LogDock.h"
#include "ChatDock.h"
#include "QPostMessage.h"

namespace qh {
namespace provider { class MockProvider; }
namespace tool { class MockBashTool; class ToolManager; }
namespace engine { class EngineReActLoop; }

namespace app {

class EngineThread;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();   // unique_ptr 析构需完整类型；且需 wait 工作线程
    void test();
private slots:
    void onChatSend();

private:
    void appendLog(const QString& text);

    LogDock* _logDock = nullptr;
    ChatDock* _chatDock = nullptr;
    QPostMessage* _postMessage = nullptr;

    // mock + engine 由成员持有，保证异步生命周期（worker 线程引用期间有效）
    std::unique_ptr<qh::provider::MockProvider> _mockProvider;
    std::unique_ptr<qh::tool::MockBashTool> _mockBash;
    std::unique_ptr<qh::tool::ToolManager> _toolManager;
    std::unique_ptr<qh::engine::EngineReActLoop> _engine;
    EngineThread* _engineThread = nullptr;   // parent this；finished 时清 nullptr + deleteLater
};

} // namespace app
} // namespace qh
#endif // QH_APP_MAINWINDOW_H
