#include "QPostMessage.h"
#include <QPlainTextEdit>
#include <QString>

namespace qh {
namespace app {

QPostMessage::QPostMessage(QPlainTextEdit* view, QObject* parent)
    : QObject(parent), _view(view) {
    // 信号 → view 追加；lambda 加级别前缀。AutoConnection 跨线程自动 QueuedConnection
    connect(this, &QPostMessage::messagePosted, _view,
        [this](qh::schema::Level level, const QString& msg) {
            if (_view) {
                _view->appendPlainText(
                    QString::fromStdString(qh::schema::levelToString(level)) + " " + msg);
            }
        });
}

void QPostMessage::post(qh::schema::Level level, const std::string& message) {
    emit messagePosted(level, QString::fromStdString(message));
}

} // namespace app
} // namespace qh
