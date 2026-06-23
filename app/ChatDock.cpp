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
    view_ = new QTextBrowser(center);
    input_ = new QLineEdit(center);
    input_->setPlaceholderText(QStringLiteral("输入消息后回车发送..."));
    layout->addWidget(view_);
    layout->addWidget(input_);
    setWidget(center);
}

QTextBrowser* ChatDock::view() const {
    return view_;
}

QLineEdit* ChatDock::input() const {
    return input_;
}

} // namespace app
} // namespace qh
