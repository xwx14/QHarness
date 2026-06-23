#ifndef QH_APP_LOGDOCK_H
#define QH_APP_LOGDOCK_H
#include <QDockWidget>

class QPlainTextEdit;

namespace qh {
namespace app {

// 日志停靠窗口：创建并持有只读 QPlainTextEdit，暴露访问器
class LogDock : public QDockWidget {
public:
    explicit LogDock(QWidget* parent = nullptr);

    // 暴露内部日志视图（后续封装时再收口）
    QPlainTextEdit* view() const;

private:
    QPlainTextEdit* view_ = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_LOGDOCK_H
