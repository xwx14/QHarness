#include "QPostMessage.h"
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QString>

namespace qh {
namespace app {

QPostMessage::QPostMessage(QPlainTextEdit* logView, QTextBrowser* chatView, QObject* parent)
    : QObject(parent), _logView(logView), _chatView(chatView) {
    // 运行期注册 Level（跨线程 QueuedConnection 传递所需；enum class 默认未注册）
    qRegisterMetaType<qh::schema::Level>("qh::schema::Level");
    // 信号 → 按级别路由：Chat 进对话窗口（带 AI 前缀），其余进日志窗口（带级别前缀）
    // AutoConnection 跨线程自动 QueuedConnection（在接收方 GUI 线程执行）
    // 级别→view 路由集中于此，便于未来为 Think 增加配置选项
    connect(this, &QPostMessage::messagePosted, _logView,
        [this](qh::schema::Level level, const QString& msg) {
            if (level == qh::schema::Level::Chat) {
                if (_chatView) {
                    _chatView->append(QStringLiteral("<b>AI:</b> ") + msg);
                }
            } else {
                if (_logView) {
                    _logView->appendPlainText(
                        QString::fromStdString(qh::schema::levelToString(level)) + " " + msg);
                }
            }
        });
}

void QPostMessage::post(qh::schema::Level level, const std::string& message) {
    emit messagePosted(level, QString::fromStdString(message));
}

} // namespace app
} // namespace qh
