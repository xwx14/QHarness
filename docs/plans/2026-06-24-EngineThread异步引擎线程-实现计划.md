# EngineThread 异步引擎线程 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** app 新增 `EngineThread`（QThread 子类，非拥有 Engine*，复用 `_postMessage`），把 `MainWindow::test()` 改为异步驱动引擎，循环日志经 `QPostMessage` 跨线程实时显示到 LogDock，UI 不阻塞。

**Architecture:** EngineThread 在 worker 线程跑 `engine->run(prompt)`；engine 经 `_postMessage`（QPostMessage，GUI 亲和）emit 信号，receiver LogDock 在 GUI 线程，AutoConnection 自动 QueuedConnection 跨线程安全。MainWindow 成员（unique_ptr）持有 mock+engine 保证异步生命周期。

**Tech Stack:** C++17、CMake、MSVC、Qt5（QThread/信号槽）、core（MockProvider/MockBashTool/ToolManager/EngineReActLoop）。

## Global Constraints

- **语言/编译**：C++17；MSVC `/utf-8`；app 链接 `qharness_core`，可 include core 头（`engine/`/`provider/`/`tool/`）。
- **命名**：类成员下划线前缀（`_engine`/`_postMessage`/`_prompt`）；驼峰命名法。
- **包含防御**：`#ifndef/#define/#endif`，宏 `QH_APP_ENGINETHREAD_H`。
- **AUTOMOC**：app `CMakeLists.txt` 已 `set(CMAKE_AUTOMOC ON)`，`Q_OBJECT` 自动处理。
- **Qt 线程安全**：QObject 信号槽 `AutoConnection` 自动跨线程 `QueuedConnection`；QThread 析构前必须确保 `run()` 已结束（`wait()`）。
- **不主动 git**：遵循 `CLAUDE.md`「未经主动要求不执行 git 提交」。**本计划不含 `git commit` 步骤**；每 Task 以「构建 + 冒烟通过」为 gate，是否提交由执行阶段与用户确认。
- **运行坑**：`qharness_app.exe` 必须在 `build/out/<config>/` 运行（Qt DLL 与 `qharness_core.dll` 已部署）。

## File Structure

**新增**：
- `app/EngineThread.h` / `.cpp` —— QThread 子类，worker 线程跑 `engine->run(prompt)`。

**修改**：
- `app/CMakeLists.txt` —— 登记 EngineThread。
- `app/MainWindow.h` —— forward declare core mock + EngineThread；加 `<memory>`、`~MainWindow()`、mock/engine/`_engineThread` 成员。
- `app/MainWindow.cpp` —— include EngineThread.h；定义 `~MainWindow()`（wait 线程）；`test()` 改异步。

---

### Task 1: EngineThread 类

**Files:**
- Create: `app/EngineThread.h`
- Create: `app/EngineThread.cpp`
- Modify: `app/CMakeLists.txt`

**Interfaces:**
- Consumes: `qh::engine::Engine`（`engine/Engine.h`，有 `setPostMessage`/`run`）、`QPostMessage`（`app/QPostMessage.h`）。
- Produces: `qh::app::EngineThread` —— `EngineThread(Engine*, QPostMessage*, std::string prompt, QObject* parent)`；`run()` override（worker 线程跑 `engine->run(prompt)`）。Task 2 的 MainWindow 创建并 start 它。

- [ ] **Step 1: 写实现** `app/EngineThread.h`

```cpp
#ifndef QH_APP_ENGINETHREAD_H
#define QH_APP_ENGINETHREAD_H
#include <QThread>
#include <string>
#include "engine/Engine.h"
#include "QPostMessage.h"

namespace qh {
namespace app {

// 异步引擎线程：在 worker 线程跑 engine->run(prompt)，引擎日志经 _postMessage 跨线程投递到 LogDock
// engine/postMessage 均非拥有（调用方保证生命周期长于本线程）
class EngineThread : public QThread {
    Q_OBJECT
public:
    EngineThread(qh::engine::Engine* engine, QPostMessage* postMessage,
                 std::string prompt, QObject* parent = nullptr);
    void run() override;   // worker 线程入口

private:
    qh::engine::Engine* _engine;     // 非拥有
    QPostMessage* _postMessage;      // 非拥有（MainWindow._postMessage）
    std::string _prompt;
};

} // namespace app
} // namespace qh
#endif // QH_APP_ENGINETHREAD_H
```

- [ ] **Step 2: 写实现** `app/EngineThread.cpp`

```cpp
#include "EngineThread.h"

namespace qh {
namespace app {

EngineThread::EngineThread(qh::engine::Engine* engine, QPostMessage* postMessage,
                           std::string prompt, QObject* parent)
    : QThread(parent), _engine(engine), _postMessage(postMessage), _prompt(std::move(prompt)) {
    _engine->setPostMessage(_postMessage);   // 复用 MainWindow._postMessage
}

void EngineThread::run() {
    _engine->run(_prompt);   // worker 线程执行 ReAct 循环
}

} // namespace app
} // namespace qh
```

- [ ] **Step 3: 改** `app/CMakeLists.txt`，登记 EngineThread

在 `qharness_app` 源列表的 `QPostMessage.cpp` 之后插入两行：

```cmake
    QPostMessage.h
    QPostMessage.cpp
    EngineThread.h
    EngineThread.cpp
    ChatDock.h
    ChatDock.cpp
```

- [ ] **Step 4: 重新配置 + 构建 app，验证编译（AUTOMOC 处理 Q_OBJECT）**

Run: `cmake -S . -B build && cmake --build build --config Release --target qharness_app`（工作目录 `E:/MyHarness/QHarnesscc`）
Expected: 构建成功（moc 生成 EngineThread 元对象代码；`qharness_app.exe` 产出至 `build/out/Release`）。

---

### Task 2: MainWindow 异步改造

**Files:**
- Modify: `app/MainWindow.h`
- Modify: `app/MainWindow.cpp`

**Interfaces:**
- Consumes: `EngineThread`（Task 1）、core mock（MockProvider/MockBashTool/ToolManager/EngineReActLoop）、`QPostMessage`（`_postMessage`）。
- Produces: `MainWindow::test()` 异步——点击「测试 ReAct」按钮后，worker 线程跑 ReAct 循环，LogDock 实时滚动日志，UI 不阻塞。

- [ ] **Step 1: 改** `app/MainWindow.h`（forward declare + 成员 + 析构声明）

整体替换为：

```cpp
#ifndef QH_APP_MAINWINDOW_H
#define QH_APP_MAINWINDOW_H
#include <QMainWindow>
#include <memory>
#include "LogDock.h"
#include "ChatDock.h"
#include "QPostMessage.h"

namespace qh {
namespace provider { class MockProvider; }
namespace tool { class MockBashTool; class ToolManager; }
namespace engine { class EngineReActLoop; }

namespace app {

class EngineThread;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();   // unique_ptr 析构需完整类型；且需 wait 工作线程
    void test();
private slots:
    void onChatSend();

private:
    void appendLog(const QString& text);

    LogDock* _logDock = nullptr;
    ChatDock* _chatDock = nullptr;
    QPostMessage* _postMessage = nullptr;

    // mock + engine 由成员持有，保证异步生命周期（worker 线程引用期间有效）
    std::unique_ptr<qh::provider::MockProvider> _mockProvider;
    std::unique_ptr<qh::tool::MockBashTool> _mockBash;
    std::unique_ptr<qh::tool::ToolManager> _toolManager;
    std::unique_ptr<qh::engine::EngineReActLoop> _engine;
    EngineThread* _engineThread = nullptr;   // parent this；finished 时清 nullptr + deleteLater
};

} // namespace app
} // namespace qh
#endif // QH_APP_MAINWINDOW_H
```

- [ ] **Step 2: 改** `app/MainWindow.cpp`（include + 析构 + test 异步）

在文件顶部 include 块加 `#include "EngineThread.h"`（其余 core mock 头 `<QDir>` 已在）：

```cpp
#include "engine/EngineReActLoop.h"
#include "provider/MockProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include "EngineThread.h"
```

在 `MainWindow::MainWindow` 构造函数之后、`MainWindow::test()` 之前，插入析构定义：

```cpp
MainWindow::~MainWindow() {
    // 析构前确保工作线程已结束（QThread 析构时 run 仍在会出错）
    if (_engineThread && _engineThread->isRunning()) {
        _engineThread->wait();
    }
    // _engineThread（parent this）与 unique_ptr 成员由 Qt/默认析构清理
}
```

整体替换 `MainWindow::test()`（当前同步版）为异步版：

```cpp
void MainWindow::test()
{
    // 并发保护：上一轮仍在跑则忽略
    if (_engineThread && _engineThread->isRunning()) {
        return;
    }
    // 重建 mock + engine（重置 MockProvider turn 计数）
    _mockProvider = std::make_unique<qh::provider::MockProvider>();
    _mockBash = std::make_unique<qh::tool::MockBashTool>();
    _toolManager = std::make_unique<qh::tool::ToolManager>();
    _toolManager->registerTool(*_mockBash);
    _engine = std::make_unique<qh::engine::EngineReActLoop>(
        _mockProvider.get(), _toolManager.get(), QDir::currentPath().toStdString());

    // 异步驱动：EngineThread（parent this）跑 engine->run
    _engineThread = new EngineThread(_engine.get(), _postMessage,
                                     "帮我检查当前目录的文件", this);
    connect(_engineThread, &QThread::finished, this, [this] { _engineThread = nullptr; });
    connect(_engineThread, &QThread::finished, _engineThread, &QObject::deleteLater);
    _engineThread->start();
}
```

- [ ] **Step 3: 构建 app，验证编译**

Run: `cmake --build build --config Release --target qharness_app`
Expected: 构建成功。

- [ ] **Step 4: GUI 冒烟——异步运行 + UI 不阻塞**

Run（带超时）: `timeout 8 build/out/Release/qharness_app.exe; echo "exit=$?"`
Expected: 进程启动不崩溃（exit=124 超时正常）。代码正确性由逻辑保证：点击「测试 ReAct」会异步启动 EngineThread，worker 线程跑 ReAct 循环，LogDock 经 QueuedConnection 实时显示 Turn/工具/完成日志，UI 不冻结。

- [ ] **Step 5: 回归 core 测试（app 改动未碰 core）**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全绿（104 checks / 0 failures）。

---

## Self-Review

**1. Spec coverage**（对照设计 A–E）：
- §1 EngineThread 类 → Task 1 ✓
- §2 MainWindow 改造（成员 + test 异步）→ Task 2 ✓
- §3 线程模型（跨线程 QueuedConnection、生命周期、finished 清理）→ Task 2 test() + ~MainWindow ✓
- §4 CMakeLists → Task 1/2 ✓
- §5 测试（GUI 冒烟）→ Task 2 Step 4 ✓
- 范围边界（不加 finished UI 反馈/真实 provider/并发处理用忽略）→ 计划遵循 ✓

**2. Placeholder scan**：无 TBD/TODO；每步含完整代码 ✓

**3. Type consistency**：
- `EngineThread(Engine*, QPostMessage*, std::string, QObject*)` 在 .h/.cpp/MainWindow test() 一致 ✓
- `_engine`/`_postMessage`/`_prompt` 成员名一致 ✓
- MainWindow 成员 `unique_ptr<MockProvider/MockBashTool/ToolManager/EngineReActLoop>` + `EngineThread*` 一致 ✓
- forward declare（`qh::provider::MockProvider` 等）与 core 实际命名空间一致 ✓
- `~MainWindow()` 声明（.h）+ 定义（.cpp）一致 ✓
