#include "MainWindow.h"
#include <QPushButton>     // connect button()->clicked
#include <QPlainTextEdit>  // _chatDock->input()->toPlainText + _logDock->view()->appendPlainText
#include <QShortcut>       // Ctrl+Enter 快捷发送
#include <QKeySequence>
#include <QTextBrowser>    // _chatDock->view()->append
#include <QToolBar>        // 「测试」按钮工具栏
#include <QDir>            // workDir = currentPath
#include "EngineThread.h"

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

    // 实时消息发送器（Qt 实现）：Chat→对话窗口，其余→日志窗口；core 组件后续 setPostMessage 注入
    _postMessage = new QPostMessage(_logDock->view(), _chatDock->view(), this);
    _postMessage->post(qh::schema::Level::Info, "QHarnesscc 已就绪");

    connect(_chatDock->button(), &QPushButton::clicked, this, &MainWindow::onChatSend);

    // Ctrl+Enter 快捷发送（普通回车保持换行）
    auto* sendShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onChatSend);

    // 「测试」按钮：触发 test() 跑 mock ReAct 循环（参考 Go cmd/claw/main.go）
    auto* toolbar = addToolBar(QStringLiteral("main"));
    auto* testBtn = new QPushButton(QStringLiteral("测试 ReAct"), this);
    toolbar->addWidget(testBtn);
    connect(testBtn, &QPushButton::clicked, this, &MainWindow::test);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}


void MainWindow::test()
{
    // 并发保护：上一轮仍在跑则忽略
    if (_engineThread && _engineThread->isRunning()) {
        return;
    }
    // 异步驱动：每次 new 一套 mock+engine（封装在 EngineThread 内，turn 计数天然重置）
    _engineThread = new EngineThread(_postMessage,
                                     "帮我检查当前目录的文件",
                                     QDir::currentPath().toStdString(), this);
    QThread* const expected = _engineThread;
    connect(_engineThread, &QThread::finished, this, [this, expected] {
        if (_engineThread == expected) { _engineThread = nullptr; }
    });
    connect(_engineThread, &QThread::finished, _engineThread, &QObject::deleteLater);
    _engineThread->start();
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
