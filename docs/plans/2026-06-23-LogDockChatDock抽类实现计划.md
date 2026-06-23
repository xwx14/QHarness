# LogDock / ChatDock 抽类实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 `MainWindow` 的两个内联 dock 抽成 `LogDock`/`ChatDock`（`QDockWidget` 子类），各自封装控件创建并通过访问器暴露控件指针，`MainWindow` 经访问器操作控件。

**Architecture:** `LogDock` 持只读 `QPlainTextEdit`、`ChatDock` 持 `QTextBrowser`+`QLineEdit`，均暴露 `view()`/`input()` 访问器（暂不封装方法/信号）；`MainWindow` 持 dock 类指针，`appendLog`/`onChatSend` 与 `returnPressed` 连接经访问器完成。app 是 GUI，验证靠构建 + 启动冒烟（无单元测试）。

**Tech Stack:** C++17、Qt 5.14.2 Widgets（`QDockWidget`/`QPlainTextEdit`/`QTextBrowser`/`QLineEdit`）、CMake `AUTOMOC`、MSVC `/utf-8`。

## Global Constraints

（取自设计文档 `docs/specs/2026-06-23-LogDockChatDock抽类设计.md` 与项目 CLAUDE.md）

- C++17、Qt 5.14.2（msvc2017_64）、MSVC、`/utf-8`。
- `app` 链接 `qharness_core`（SHARED）+ `Qt5::Core`/`Qt5::Widgets`；`CMAKE_AUTOMOC ON`。
- 命名空间 `qh::app`；include guard `QH_APP_<文件>_H`，用 `#ifndef/#define/#endif`。
- 头文件列入 `app/CMakeLists.txt` 的 `qharness_app` 源列表（VS 工程可见）。
- dock 类**不加** `Q_OBJECT`（无自定义信号槽）；`MainWindow` 保留 `Q_OBJECT`（AUTOMOC 处理）。
- 源码注释中文；变量/函数驼峰命名。
- 构建输出统一 `build/out/<config>`（含 `qharness_core.dll`、`qharness_app.exe`）。
- git 提交信息中文；提交前需用户确认（项目约定）。

构建/验证命令（仓库根 `E:/MyHarness/QHarnesscc`，bash）：

```bash
cmake -S . -B build                                          # CMakeLists 改动后重新配置
cmake --build build --config Release --target qharness_app   # 构建 app（含 AUTOMOC）
build/out/Release/qharness_core_tests.exe                    # core 测试不回归（63/0）
build/out/Release/qharness_app.exe                           # GUI 冒烟（手动启动）
```

---

## Task 1: 新建 LogDock / ChatDock（暴露指针）+ CMakeLists 登记

**Files:**
- Create: `app/LogDock.h`、`app/LogDock.cpp`
- Create: `app/ChatDock.h`、`app/ChatDock.cpp`
- Modify: `app/CMakeLists.txt`（`qharness_app` 源列表加 4 个文件）

**Interfaces:**
- Consumes: Qt Widgets（`QDockWidget`/`QPlainTextEdit`/`QTextBrowser`/`QLineEdit`/`QVBoxLayout`）。
- Produces:
  - `qh::app::LogDock : public QDockWidget`：`explicit LogDock(QWidget* parent = nullptr);`、`QPlainTextEdit* view() const;`
  - `qh::app::ChatDock : public QDockWidget`：`explicit ChatDock(QWidget* parent = nullptr);`、`QTextBrowser* view() const;`、`QLineEdit* input() const;`
- 本任务**不**接入 `MainWindow`（dock 类暂为新增未用代码，仅验证编译）。

- [ ] **Step 1: 新建 `app/LogDock.h`**

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

- [ ] **Step 2: 新建 `app/LogDock.cpp`**

```cpp
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
```

- [ ] **Step 3: 新建 `app/ChatDock.h`**

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

- [ ] **Step 4: 新建 `app/ChatDock.cpp`**

```cpp
#include "ChatDock.h"
#include <QTextBrowser>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace qh {
namespace app {

ChatDock::ChatDock(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle(QStringLiteral("对话"));
    QWidget* center = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(center);
    view_ = new QTextBrowser(center);
    input_ = new QLineEdit(center);
    input_->setPlaceholderText(QStringLiteral("输入消息后回车发送..."));
    layout->addWidget(view_);
    layout->addWidget(input_);
    setWidget(center);
}

QTextBrowser* ChatDock::view() const {
    return view_;
}

QLineEdit* ChatDock::input() const {
    return input_;
}

} // namespace app
} // namespace qh
```

- [ ] **Step 5: 修改 `app/CMakeLists.txt`，登记 4 个新文件**

把 `add_executable(qharness_app WIN32 ...)` 的源列表改为：

```cmake
add_executable(qharness_app WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
    LogDock.h
    LogDock.cpp
    ChatDock.h
    ChatDock.cpp
)
```

- [ ] **Step 6: 重新配置并构建 app（验证 dock 类编译）**

Run:
```bash
cmake -S . -B build && cmake --build build --config Release --target qharness_app
```
Expected: 配置成功；构建成功（`LogDock.cpp`、`ChatDock.cpp` 编译，含 AUTOMOC 处理；`MainWindow` 未改动仍链接通过）；无 error。

- [ ] **Step 7: 提交（需用户确认后执行）**

```bash
git add app/LogDock.h app/LogDock.cpp app/ChatDock.h app/ChatDock.cpp app/CMakeLists.txt
git commit -m "新增LogDock/ChatDock停靠窗口类(继承QDockWidget,暴露控件指针访问器,暂未接入MainWindow)"
```

---

## Task 2: MainWindow 接入 LogDock / ChatDock + 启动冒烟

**Files:**
- Modify: `app/MainWindow.h`
- Modify: `app/MainWindow.cpp`

**Interfaces:**
- Consumes: Task 1 的 `LogDock::view()`、`ChatDock::view()`/`input()`。
- Produces: `MainWindow` 持 `LogDock* logDock_`、`ChatDock* chatDock_`；`appendLog`/`onChatSend` 经访问器操作控件；`returnPressed` 连到 `chatDock_->input()`。

- [ ] **Step 1: 改 `app/MainWindow.h`**

```cpp
#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include "LogDock.h"
#include "ChatDock.h"

namespace qh {
namespace app {

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

} // namespace app
} // namespace qh
#endif // QH_APP_MAINWINDOW_H
```

- [ ] **Step 2: 改 `app/MainWindow.cpp`**

```cpp
#include "MainWindow.h"
#include <QLineEdit>       // connect returnPressed + input()->text/clear
#include <QPlainTextEdit>  // logDock_->view()->appendPlainText
#include <QTextBrowser>    // chatDock_->view()->append

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    // 日志子窗口（底部）
    logDock_ = new LogDock(this);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);

    // 对话子窗口（顶部）
    chatDock_ = new ChatDock(this);
    addDockWidget(Qt::TopDockWidgetArea, chatDock_);

    connect(chatDock_->input(), &QLineEdit::returnPressed, this, &MainWindow::onChatSend);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::appendLog(const QString& text) {
    logDock_->view()->appendPlainText(text);
}

void MainWindow::onChatSend() {
    QString text = chatDock_->input()->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    chatDock_->view()->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    chatDock_->input()->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
}

} // namespace app
} // namespace qh
```

- [ ] **Step 3: 构建 app + core 测试不回归**

Run:
```bash
cmake --build build --config Release --target qharness_app && cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe | grep -E "checks|FAIL"; echo "exit=${PIPESTATUS[0]}"
```
Expected: app 构建成功（AUTOMOC 重新处理 `MainWindow` 的 moc）；core 测试 `63 checks, 0 failures`，`exit=0`（不回归）。

- [ ] **Step 4: GUI 启动冒烟**

Run（启动 app 6 秒后由 timeout 终止）:
```bash
timeout 6 build/out/Release/qharness_app.exe; echo "app_exit=$?"
```
Expected: app 正常启动并运行至被 timeout 杀掉，`app_exit=124`（`timeout` 正常终止码）。若 `app_exit` 为 `139`(段错误) 或 `-1073741515`(0xC0000135 缺 DLL) 等，说明启动失败，需排查。
> 若自动化 timeout 启动有干扰，可改为人工双击 `build/out/Release/qharness_app.exe` 确认：窗口含「日志」（底）+「对话」（顶）两个 dock，对话输入回车后内容上屏、输入清空、日志记录发送。

- [ ] **Step 5: 提交（需用户确认后执行）**

```bash
git add app/MainWindow.h app/MainWindow.cpp
git commit -m "MainWindow接入LogDock/ChatDock(移除内联控件,经访问器操作,逻辑与returnPressed连接保留)"
```

---

## 验收标准

- `LogDock`（`QPlainTextEdit* view()`）与 `ChatDock`（`QTextBrowser* view()` + `QLineEdit* input()`）作为独立 `QDockWidget` 子类存在，控件创建封装在各自构造函数。
- `MainWindow` 持 `LogDock*`/`ChatDock*`，不再直接持有 `logView_`/`chatView_`/`chatInput_`；`appendLog`/`onChatSend` 经访问器操作控件。
- 构建 `qharness_app` 成功（AUTOMOC 正常）；core 测试 `63 checks/0 failures` 不回归。
- `build/out/Release/qharness_app.exe` 启动冒烟：两个 dock 正常显示与交互。
