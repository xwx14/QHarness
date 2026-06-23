# LogDock / ChatDock 抽类设计（暴露指针版）

> 日期：2026-06-23
> 范围：`app` 模块（`qharness_app` 的 `MainWindow`）
> 背景：用户要求 `logDock_` 与 `chatDock_` 用单独类实现（继承 `QDockWidget`），各自实现控件添加逻辑。dock 类**暴露控件指针访问器**，暂不封装方法/信号，后续再封装。

## 1. 背景与目标

**现状**（`app/MainWindow.h` + `MainWindow.cpp`）：
- `MainWindow : public QMainWindow` 持 `QDockWidget* logDock_`/`chatDock_` 与控件 `QPlainTextEdit* logView_`、`QTextBrowser* chatView_`、`QLineEdit* chatInput_`。
- 构造函数内联创建两个 dock 及其内部控件、布局组装、`addDockWidget`。
- 私有方法 `appendLog(QString)`、私有槽 `onChatSend()`（读 `chatInput_`、追加 `chatView_`、清空输入、记日志）。

**目标**：把两个 dock 抽成独立 `QDockWidget` 子类，各自封装自己的控件创建与布局组装，并通过访问器暴露内部控件指针；`MainWindow` 持 dock 类指针，经访问器操作控件——业务逻辑（`appendLog`/`onChatSend`）与信号连接仍留在 `MainWindow`。**接口封装（方法/信号）留待后续步骤。**

## 2. 核心决策（已与用户确认）

| 决策点 | 选择 | 理由 |
|---|---|---|
| dock 类封装程度 | **暴露控件指针访问器**，暂不封装方法/信号 | 用户要求；最小改动把 dock 抽成类，后续再封装 |
| 控件创建归属 | dock 类构造函数内创建控件 + `setWidget` | 各自实现控件添加逻辑（用户要求） |
| 业务逻辑归属 | `appendLog`/`onChatSend` 留在 `MainWindow` | dock 暂不封装行为 |
| MainWindow 控件指针 | 移除 `logView_`/`chatView_`/`chatInput_`，经 dock 访问器获取 | 控件所有权归 dock，MainWindow 经访问器用 |
| `onChatSend` 签名 | 保持无参（读 `chatDock_->input()`） | 与现状一致，最小改动 |
| `Q_OBJECT` | dock 类无自定义信号槽，**不加** `Q_OBJECT` | YAGNI；未来加信号槽时再补 |

## 3. 类设计

### 3.1 LogDock（`app/LogDock.h` + `.cpp`）

```cpp
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
```

`.cpp`：构造 `setWindowTitle(QStringLiteral("日志"))`；`view_ = new QPlainTextEdit(this)`，`view_->setReadOnly(true)`；`setWidget(view_)`。`view()` → `return view_;`。

### 3.2 ChatDock（`app/ChatDock.h` + `.cpp`）

```cpp
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
    QTextBrowser* view_ = nullptr;
    QLineEdit* input_ = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_CHATDOCK_H
```

`.cpp`：构造 `setWindowTitle(QStringLiteral("对话"))`；建容器 `QWidget* center = new QWidget(this)` + `QVBoxLayout* layout = new QVBoxLayout(center)`；`view_ = new QTextBrowser(center)`；`input_ = new QLineEdit(center)`，`input_->setPlaceholderText(QStringLiteral("输入消息后回车发送..."))`；`layout->addWidget(view_)`、`layout->addWidget(input_)`；`setWidget(center)`。`view()` → `return view_;`，`input()` → `return input_;`。

### 3.3 MainWindow 改动

```cpp
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private slots:
    void onChatSend();
private:
    void appendLog(const QString& text);
    LogDock* logDock_ = nullptr;
    ChatDock* chatDock_ = nullptr;
};
```

- 移除原前向声明 `QDockWidget`/`QPlainTextEdit`/`QTextBrowser`/`QLineEdit`，include `LogDock.h`/`ChatDock.h`。
- 构造：`logDock_ = new LogDock(this)` + `addDockWidget(Qt::BottomDockWidgetArea, logDock_)`；`chatDock_ = new ChatDock(this)` + `addDockWidget(Qt::TopDockWidgetArea, chatDock_)`；`connect(chatDock_->input(), &QLineEdit::returnPressed, this, &MainWindow::onChatSend)`；`appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"))`。
- `appendLog(const QString& text)`：`logDock_->view()->appendPlainText(text)`。
- `onChatSend()`：`QString text = chatDock_->input()->text().trimmed()`；空则返回；`chatDock_->view()->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped())`；`chatDock_->input()->clear()`；`appendLog(QStringLiteral("[Chat] 用户发送: ") + text)`。
- 移除成员 `logDock_`/`chatDock_` 旧 `QDockWidget*` 类型与 `logView_`/`chatView_`/`chatInput_`。

## 4. 文件与 CMakeLists

| 文件 | 动作 |
|---|---|
| `app/LogDock.h` / `LogDock.cpp` | 新建 |
| `app/ChatDock.h` / `ChatDock.cpp` | 新建 |
| `app/MainWindow.h` | 修改（成员类型、include、移除前向声明与控件指针） |
| `app/MainWindow.cpp` | 修改（构造用 dock 访问器、`appendLog`/`onChatSend` 经访问器） |
| `app/CMakeLists.txt` | 修改（`qharness_app` 源列表加 `LogDock.h`/`LogDock.cpp`/`ChatDock.h`/`ChatDock.cpp`） |

## 5. Q_OBJECT / AUTOMOC

- `LogDock`/`ChatDock` 无自定义信号槽，**不加** `Q_OBJECT`，不需要 moc（普通 `QDockWidget` 子类）。`app/CMakeLists.txt` 已 `set(CMAKE_AUTOMOC ON)`，`MainWindow` 仍正常 moc。
- 未来为 dock 类加信号槽时，再补 `Q_OBJECT`，AUTOMOC 自动处理。

## 6. 验证

- 构建 `qharness_app` 成功（无编译/链接错误，含控件访问器的链路）。
- `build/out/Release/qharness_app.exe` 启动冒烟：窗口含「日志」（底）与「对话」（顶）两个 dock；对话输入回车后内容上屏、输入清空、日志记录发送；core 测试不受影响（`qharness_core_tests` 仍 63 checks/0 failures）。

## 7. 非目标（YAGNI）

- **暂不封装 dock 接口**（不做 `appendLog`/`appendMessage` 方法、`chatSent` 信号），仅暴露控件指针——后续步骤再收口封装。
- 不实现真实对话/日志后端（接入 Engine 是后续步骤）。
- 不为 dock 增加设置、持久化、多输入框等特性。
- 不改 dock 的停靠区域（仍 Bottom/Top）与外观。
- 不改 `app` 不导出（app 是可执行，非 DLL）的边界。
