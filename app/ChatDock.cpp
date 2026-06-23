#include "ChatDock.h"
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

namespace qh {
namespace app {

ChatDock::ChatDock(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle(QStringLiteral("对话"));
    QWidget* center = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(center);
    _view = new QTextBrowser(center);
    _input = new QPlainTextEdit(center);
    _input->setPlaceholderText(QStringLiteral("输入消息后点击「发送」或按 Ctrl+Enter 发送（回车换行）..."));
    _button = new QPushButton(QStringLiteral("发送"), center);
    QHBoxLayout* inputLayout = new QHBoxLayout;
    inputLayout->addWidget(_input);
    inputLayout->addWidget(_button);
    layout->addWidget(_view);
    layout->addLayout(inputLayout);
    setWidget(center);
}

QTextBrowser* ChatDock::view() const {
    return _view;
}

QPlainTextEdit* ChatDock::input() const {
    return _input;
}

QPushButton* ChatDock::button() const {
    return _button;
}

} // namespace app
} // namespace qh
