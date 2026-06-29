#include "MainWindow.h"
#include <QPushButton>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QKeySequence>
#include <QTextBrowser>
#include <QDir>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QCoreApplication>
#include "EngineThread.h"
#include "SettingsDialog.h"
#include "config/SettingsStore.h"

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    _logDock = new LogDock(this);
    addDockWidget(Qt::BottomDockWidgetArea, _logDock);

    _chatDock = new ChatDock(this);
    addDockWidget(Qt::TopDockWidgetArea, _chatDock);

    _postMessage = new QPostMessage(_logDock->view(), _chatDock->view(), this);
    _postMessage->post(qh::schema::Level::Info, "QHarnesscc 已就绪");

    connect(_chatDock->button(), &QPushButton::clicked, this, &MainWindow::onChatSend);

    auto* sendShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onChatSend);

    // 菜单栏：设置入口
    auto* settingsMenu = menuBar()->addMenu(QStringLiteral("设置(&S)"));
    auto* settingsAction = settingsMenu->addAction(QStringLiteral("设置…"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    // 启动加载配置
    QString configDir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(configDir);   // 目录不存在则创建（首次运行）
    _configPath = (configDir + "/setting.json").toStdString();
    loadSettings();

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::startEngine(const std::string& prompt, EngineThread::EngineKind kind) {
    if (_engineThread && _engineThread->isRunning()) {
        return;
    }
    // 用当前 _settings（值拷贝）驱动引擎装配；workDir 空时 EngineThread 内部回退 currentPath
    _engineThread = new EngineThread(_postMessage, prompt, _settings, kind, this);
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
    if (_engineThread && _engineThread->isRunning()) {
        _postMessage->post(qh::schema::Level::Warn, "引擎正在运行，请等待本轮结束");
        return;
    }
    _chatDock->view()->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    _chatDock->input()->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
    // enableThinking 决定引擎种类：true→两阶段 ReAct，false→普通 ReAct（与设置语义一致）
    const EngineThread::EngineKind kind = _settings._enableThinking
        ? EngineThread::EngineKind::TwoStageReAct
        : EngineThread::EngineKind::ReAct;
    startEngine(text.toStdString(), kind);
}

void MainWindow::loadSettings() {
    config::SettingsStore store(_configPath);
    _settings = store.load();
    if (!store.lastError().empty()) {
        _postMessage->post(qh::schema::Level::Warn,
                           "配置加载失败，已用默认: " + store.lastError());
    }
}

void MainWindow::saveSettings() {
    config::SettingsStore store(_configPath);
    if (!store.save(_settings)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString("无法写入配置: " + store.lastError()));
    }
}

void MainWindow::onSettings() {
    SettingsDialog dlg(this);
    dlg.setSettings(_settings);
    if (dlg.exec() == QDialog::Accepted) {
        _settings = dlg.getSettings();
        saveSettings();   // 失败已在 saveSettings 内弹窗，内存 _settings 仍保留
    }
}

} // namespace app
} // namespace qh
