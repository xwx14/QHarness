#ifndef QH_APP_CHATDOCK_H
#define QH_APP_CHATDOCK_H
#include <QDockWidget>

class QTextBrowser;
class QLineEdit;

namespace qh {
namespace app {

// 对话停靠窗口：创建并持有 QTextBrowser（显示）+ QLineEdit（输入），暴露访问器
class ChatDock : public QDockWidget {
public:
    explicit ChatDock(QWidget* parent = nullptr);

    QTextBrowser* view() const;   // 暴露显示控件
    QLineEdit* input() const;     // 暴露输入控件

private:
    QTextBrowser* _view = nullptr;
    QLineEdit* _input = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_CHATDOCK_H
