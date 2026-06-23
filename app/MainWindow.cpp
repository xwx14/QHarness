#include "MainWindow.h"
#include <QPushButton>     // connect button()->clicked
#include <QPlainTextEdit>  // _chatDock->input()->toPlainText + _logDock->view()->appendPlainText
#include <QShortcut>       // Ctrl+Enter 快捷发送
#include <QKeySequence>
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

    connect(_chatDock->button(), &QPushButton::clicked, this, &MainWindow::onChatSend);

    // Ctrl+Enter 快捷发送（普通回车保持换行）
    auto* sendShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onChatSend);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::appendLog(const QString& text) {
    _logDock->view()->appendPlainText(text);
}

void MainWindow::onChatSend() {
    QString text = _chatDock->input()->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }
    _chatDock->view()->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    _chatDock->input()->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
}

} // namespace app
} // namespace qh
