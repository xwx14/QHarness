#include "ChatDock.h"
#include <QTextBrowser>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace qh {
namespace app {

ChatDock::ChatDock(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle(QStringLiteral("对话"));
    QWidget* center = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(center);
    _view = new QTextBrowser(center);
    _input = new QLineEdit(center);
    _input->setPlaceholderText(QStringLiteral("输入消息后回车发送..."));
    layout->addWidget(_view);
    layout->addWidget(_input);
    setWidget(center);
}

QTextBrowser* ChatDock::view() const {
    return _view;
}

QLineEdit* ChatDock::input() const {
    return _input;
}

} // namespace app
} // namespace qh
