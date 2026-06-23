#include "MainWindow.h"
#include <QLineEdit>       // connect returnPressed + input()->text/clear
#include <QPlainTextEdit>  // logDock_->view()->appendPlainText
#include <QTextBrowser>    // chatDock_->view()->append

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    // 日志子窗口（底部）
    logDock_ = new LogDock(this);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);

    // 对话子窗口（顶部）
    chatDock_ = new ChatDock(this);
    addDockWidget(Qt::TopDockWidgetArea, chatDock_);

    connect(chatDock_->input(), &QLineEdit::returnPressed, this, &MainWindow::onChatSend);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::appendLog(const QString& text) {
    logDock_->view()->appendPlainText(text);
}

void MainWindow::onChatSend() {
    QString text = chatDock_->input()->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    chatDock_->view()->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    chatDock_->input()->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
}

} // namespace app
} // namespace qh
