#ifndef QH_APP_QPOSTMESSAGE_H
#define QH_APP_QPOSTMESSAGE_H
#include <QObject>
#include <QString>
#include "schema/PostMessage.h"

class QPlainTextEdit;

namespace qh {
namespace app {

// PostMessage 的 Qt 实现：把消息投递到 QPlainTextEdit（LogDock::view()）
// 跨线程安全：post() emit 信号，AutoConnection 在接收方（GUI 线程）自动排队
class QPostMessage : public QObject, public qh::schema::PostMessage {
    Q_OBJECT
public:
    explicit QPostMessage(QPlainTextEdit* view, QObject* parent = nullptr);
    void post(qh::schema::Level level, const std::string& message) override;

signals:
    void messagePosted(qh::schema::Level level, const QString& message);

private:
    QPlainTextEdit* _view = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_QPOSTMESSAGE_H
