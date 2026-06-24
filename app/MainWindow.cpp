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
#include "EngineThread.h"

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


MainWindow::~MainWindow() {
    // 析构前确保工作线程已结束：worker 线程引用 _engine/_postMessage（raw），
    // 必须在其结束后方可析构这些成员（QThread 析构时 run 仍在也会出错）
    if (_engineThread && _engineThread->isRunning()) {
        _engineThread->wait();
    }
    // _engineThread（parent this）与 unique_ptr 成员由 Qt/默认析构清理
}

void MainWindow::test()
{
    // 并发保护：上一轮仍在跑则忽略
    if (_engineThread && _engineThread->isRunning()) {
        return;
    }
    // 重建 mock + engine（重置 MockProvider turn 计数）
    _mockProvider = std::make_unique<qh::provider::MockProvider>();
    _mockBash = std::make_unique<qh::tool::MockBashTool>();
    _toolManager = std::make_unique<qh::tool::ToolManager>();
    _toolManager->registerTool(*_mockBash);
    _engine = std::make_unique<qh::engine::EngineReActLoop>(
        _mockProvider.get(), _toolManager.get(), QDir::currentPath().toStdString());

    // 异步驱动：EngineThread（parent this）跑 engine->run
    _engineThread = new EngineThread(_engine.get(), _postMessage,
                                     "帮我检查当前目录的文件", this);
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
