#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"
#include "QPostMessage.h"
#include "EngineThread.h"   // startEngine() 的 EngineThread::EngineKind 参数
#include <string>
#include "schema/Settings.h"

namespace qh {
namespace app {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private slots:
    void onChatSend();
    void onSettings();

private:
    void appendLog(const QString& text);
    // 启动引擎线程跑 prompt：守卫已在运行的线程，由 onChatSend 调用
    void startEngine(const std::string& prompt, EngineThread::EngineKind kind);

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
