#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"
#include "QPostMessage.h"
#include "EngineThread.h"   // test() 的 EngineThread::EngineKind 参数

namespace qh {
namespace app {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    // 触发 mock 引擎 ReAct 循环：kind 选择单阶段(ReAct)或两阶段(TwoStageReAct)
    void test(EngineThread::EngineKind kind = EngineThread::EngineKind::ReAct);
private slots:
    void onChatSend();

private:
    void appendLog(const QString& text);

    LogDock* _logDock = nullptr;
    ChatDock* _chatDock = nullptr;
    QPostMessage* _postMessage = nullptr;
    EngineThread* _engineThread = nullptr;   // parent this；自带 mock+engine 子树；finished 时清 nullptr + deleteLater
};

} // namespace app
} // namespace qh
#endif // QH_APP_MAINWINDOW_H
