#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"
#include "QPostMessage.h"

namespace qh {
namespace app {

class EngineThread;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void test();
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
