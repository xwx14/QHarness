#ifndef QH_APP_QPOSTMESSAGE_H
#define QH_APP_QPOSTMESSAGE_H
#include <QObject>
#include <QString>
#include <QMetaType>
#include "schema/PostMessage.h"

// 注册 Level 到 Qt 元类型系统：QPostMessage::messagePosted 信号跨线程（QueuedConnection）
// 传递 Level 参数所需——enum class 默认未注册，跨线程投递会被 Qt 静默丢弃
Q_DECLARE_METATYPE(qh::schema::Level)

class QPlainTextEdit;
class QTextBrowser;

namespace qh {
namespace app {

// PostMessage 的 Qt 实现：按级别路由——Chat→对话窗口（QTextBrowser），其余→日志窗口（QPlainTextEdit）
// 跨线程安全：post() emit 信号，AutoConnection 在接收方（GUI 线程）自动排队
class QPostMessage : public QObject, public qh::schema::PostMessage {
    Q_OBJECT
public:
    explicit QPostMessage(QPlainTextEdit* logView, QTextBrowser* chatView, QObject* parent = nullptr);
    void post(qh::schema::Level level, const std::string& message) override;

signals:
    void messagePosted(qh::schema::Level level, const QString& message);

private:
    QPlainTextEdit* _logView = nullptr;   // 日志窗口（Info/Warn/Error/Think）
    QTextBrowser* _chatView = nullptr;    // 对话窗口（Chat）
};

} // namespace app
} // namespace qh
#endif // QH_APP_QPOSTMESSAGE_H
