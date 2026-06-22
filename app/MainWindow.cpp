#include "MainWindow.h"
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    // 日志子窗口
    logDock_ = new QDockWidget(QStringLiteral("日志"), this);
    logView_ = new QPlainTextEdit(logDock_);
    logView_->setReadOnly(true);
    logDock_->setWidget(logView_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);

    // 对话子窗口（中央）
    chatDock_ = new QDockWidget(QStringLiteral("对话"), this);
    auto* center = new QWidget(chatDock_);
    auto* layout = new QVBoxLayout(center);
    chatView_ = new QTextBrowser(center);
    chatInput_ = new QLineEdit(center);
    chatInput_->setPlaceholderText(QStringLiteral("输入消息后回车发送..."));
    layout->addWidget(chatView_);
    layout->addWidget(chatInput_);
    chatDock_->setWidget(center);
    addDockWidget(Qt::TopDockWidgetArea, chatDock_);

    connect(chatInput_, &QLineEdit::returnPressed, this, &MainWindow::onChatSend);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::appendLog(const QString& text) {
    logView_->appendPlainText(text);
}

void MainWindow::onChatSend() {
    QString text = chatInput_->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    chatView_->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    chatInput_->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
    // TODO: 后续接入 Engine 进行 ReAct 推理
}

} // namespace app
} // namespace qh
