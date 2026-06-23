#include "MainWindow.h"
#include <QLineEdit>       // connect returnPressed + input()->text/clear
#include <QPlainTextEdit>  // _logDock->view()->appendPlainText
#include <QTextBrowser>    // _chatDock->view()->append

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    // 日志子窗口（底部）
    _logDock = new LogDock(this);
    addDockWidget(Qt::BottomDockWidgetArea, _logDock);

    // 对话子窗口（顶部）
    _chatDock = new ChatDock(this);
    addDockWidget(Qt::TopDockWidgetArea, _chatDock);

    connect(_chatDock->input(), &QLineEdit::returnPressed, this, &MainWindow::onChatSend);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::appendLog(const QString& text) {
    _logDock->view()->appendPlainText(text);
}

void MainWindow::onChatSend() {
    QString text = _chatDock->input()->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    _chatDock->view()->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    _chatDock->input()->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
}

} // namespace app
} // namespace qh
