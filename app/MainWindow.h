#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"
#include "QPostMessage.h"
#include "EngineThread.h"   // test() 的 EngineThread::EngineKind 参数
#include <string>
#include "schema/Settings.h"

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
    void onSettings();

private:
    void appendLog(const QString& text);

    LogDock* _logDock = nullptr;
    ChatDock* _chatDock = nullptr;
    QPostMessage* _postMessage = nullptr;
    EngineThread* _engineThread = nullptr;   // parent this；自带 mock+engine 子树；finished 时清 nullptr + deleteLater
    void loadSettings();
    void saveSettings();

    schema::Settings _settings;   // 当前配置（内存副本）
    std::string _configPath;      // <exeDir>/config/setting.json
};

} // namespace app
} // namespace qh
#endif // QH_APP_MAINWINDOW_H
