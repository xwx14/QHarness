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

    QDockWidget* logDock_;
    QDockWidget* chatDock_;
    QPlainTextEdit* logView_;
    QTextBrowser* chatView_;
    QLineEdit* chatInput_;
};

} // namespace app
} // namespace qh
