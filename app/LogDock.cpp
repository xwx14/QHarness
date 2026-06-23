#include "LogDock.h"
#include <QPlainTextEdit>

namespace qh {
namespace app {

LogDock::LogDock(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle(QStringLiteral("日志"));
    view_ = new QPlainTextEdit(this);
    view_->setReadOnly(true);
    setWidget(view_);
}

QPlainTextEdit* LogDock::view() const {
    return view_;
}

} // namespace app
} // namespace qh
