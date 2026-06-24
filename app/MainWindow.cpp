#include "MainWindow.h"
#include <QPushButton>     // connect button()->clicked
#include <QPlainTextEdit>  // _chatDock->input()->toPlainText + _logDock->view()->appendPlainText
#include <QShortcut>       // Ctrl+Enter 快捷发送
#include <QKeySequence>
#include <QTextBrowser>    // _chatDock->view()->append
#include <QToolBar>        // 「测试」按钮工具栏
#include <QDir>            // workDir = currentPath
#include "engine/EngineReActLoop.h"
#include "provider/MockProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    // 日志子窗口（底部）
    _logDock = new LogDock(this);
    addDockWidget(Qt::BottomDockWidgetArea, _logDock);

    // 实时消息发送器（Qt 实现），接入日志视图；core 组件后续 setPostMessage 注入
    _postMessage = new QPostMessage(_logDock->view(), this);
    _postMessage->post(qh::schema::Level::Info, "QHarnesscc 已就绪");

    // 对话子窗口（顶部）
    _chatDock = new ChatDock(this);
    addDockWidget(Qt::TopDockWidgetArea, _chatDock);

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
    // 参考 Go cmd/claw/main.go：用 mock 组件跑 ReAct 循环，验证引擎 tool-call 闭环
    qh::provider::MockProvider provider;       // turn 计数：第1轮 bash 调用，第2轮纯文本
    qh::tool::MockBashTool bash;
    qh::tool::ToolManager toolManager;
    toolManager.registerTool(bash);

    // 引擎注入 mock + 工作目录 + 实时消息发送器（循环日志实时显示到 LogDock）
    qh::engine::EngineReActLoop engine(&provider, &toolManager, QDir::currentPath().toStdString());
    engine.setPostMessage(_postMessage);

    // 启动 ReAct 循环（同步；mock 两轮瞬时完成，真实 provider 后续改异步）
    engine.run("帮我检查当前目录的文件");
}

void MainWindow::appendLog(const QString& text) {
    _logDock->view()->appendPlainText(text);
}

void MainWindow::onChatSend() {
    test();
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
