#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"

namespace qh {
namespace app {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onChatSend();

private:
    void appendLog(const QString& text);

    LogDock* logDock_ = nullptr;
    ChatDock* chatDock_ = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_MAINWINDOW_H
