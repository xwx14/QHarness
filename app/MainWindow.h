#pragma once
#include <QMainWindow>

class QDockWidget;
class QPlainTextEdit;
class QTextBrowser;
class QLineEdit;

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

    QDockWidget* logDock_ = nullptr;
    QDockWidget* chatDock_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QTextBrowser* chatView_ = nullptr;
    QLineEdit* chatInput_ = nullptr;
};

} // namespace app
} // namespace qh
