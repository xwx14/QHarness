#include "LogDock.h"
#include <QPlainTextEdit>

namespace qh {
namespace app {

LogDock::LogDock(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle(QStringLiteral("日志"));
    _view = new QPlainTextEdit(this);
    _view->setReadOnly(true);
    setWidget(_view);
}

QPlainTextEdit* LogDock::view() const {
    return _view;
}

} // namespace app
} // namespace qh
