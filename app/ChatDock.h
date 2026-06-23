#ifndef QH_APP_CHATDOCK_H
#define QH_APP_CHATDOCK_H
#include <QDockWidget>

class QTextBrowser;
class QPlainTextEdit;
class QPushButton;

namespace qh {
namespace app {

// 对话停靠窗口：创建并持有 QTextBrowser（显示）+ QPlainTextEdit（多行输入）+ QPushButton（发送），暴露访问器
class ChatDock : public QDockWidget {
public:
    explicit ChatDock(QWidget* parent = nullptr);

    QTextBrowser* view() const;        // 暴露显示控件
    QPlainTextEdit* input() const;     // 暴露多行输入控件（回车换行）
    QPushButton* button() const;       // 暴露发送按钮

private:
    QTextBrowser* _view = nullptr;
    QPlainTextEdit* _input = nullptr;
    QPushButton* _button = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_CHATDOCK_H
