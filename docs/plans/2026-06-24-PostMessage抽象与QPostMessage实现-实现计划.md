# PostMessage 抽象 + QPostMessage 实现 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增 `PostMessage` 抽象 + `PostMessageInterface` mixin（core/schema），7 个骨架类继承 mixin 获得消息注入能力，app 实现 `QPostMessage`（QObject）对接 LogDock 跨线程显示。

**Architecture:** 依赖反转——core 定义纯 C++17 抽象 `PostMessage`（发送目标）与 `PostMessageInterface`（持有者 mixin，DRY 集中注入逻辑）；7 个骨架类继承 mixin；app 的 `QPostMessage` 继承 `QObject`+`PostMessage`，用信号槽 `AutoConnection` 跨线程投递到 `QPlainTextEdit`。

**Tech Stack:** C++17、CMake、MSVC（VS17）、Qt5 Widgets（仅 app）、vendored nlohmann/json；core 测试用自带 `TestHarness.h`。

## Global Constraints

- **语言/编译**：C++17；MSVC `/utf-8`；core 为 SHARED DLL，公共类整类 `QH_API` 导出，**core 不依赖 Qt**。
- **命名**：类成员变量下划线前缀（`_postMessage`、`_view`）；驼峰命名法用于函数/类名；命名空间 `qh::schema`、`qh::app` 等。
- **包含防御**：`#ifndef/#define/#endif`，宏 `QH_<模块>_<文件>_H`（`PostMessage.h`→`QH_SCHEMA_POSTMESSAGE_H`，`QPostMessage.h`→`QH_APP_QPOSTMESSAGE_H`）。
- **CMakeLists**：所有 `.h`/`.cpp` 列入源列表；core 头文件用根相对路径 include（如 `#include "schema/PostMessage.h"`）；app 用 `AUTOMOC`（已开，Q_OBJECT 自动处理）。
- **构建输出**：DLL/exe/tests 统一 `build/out/{Release,Debug}`。
- **测试框架**：`QH_TEST(name){...}`；`QH_CHECK(cond)`；`QH_CHECK_EQ(a,b)`（用 `==`，失败打印左右值——**枚举如 `Level` 比较用 `QH_CHECK(x==y)`，不可 `QH_CHECK_EQ`**）；`size()` 用 `QH_CHECK_EQ(x, static_cast<size_t>(N))`。
- **不主动 git**：遵循 `CLAUDE.md`「未经主动要求不执行 git 提交」。**本计划不含 `git commit` 步骤**；每 Task 以「构建 + 全部测试通过」为完成 gate，是否提交由执行阶段与用户确认。
- **运行坑**：`qharness_app.exe`/`qharness_core_tests.exe` 必须在 `build/out/<config>/` 运行（Qt DLL 与 `qharness_core.dll` 已部署至此）。

## File Structure

**新增**：
- `core/schema/PostMessage.h` / `.cpp` —— `Level` + `PostMessage`（发送抽象）+ `PostMessageInterface`（持有者 mixin）
- `app/QPostMessage.h` / `.cpp` —— Qt 实现，信号槽对接 `QPlainTextEdit`
- `core/tests/test_PostMessage.cpp` —— Level/levelToString + FakePostMessage + mixin 注入测试

**修改（继承 `PostMessageInterface`）**：`Engine.h`、`Provider.h`、`Tool.h`、`ToolManager.h`、`Interaction.h`、`Memory.h`、`Composer.h`
**修改**：`core/tests/skeleton_compile_check.cpp`（注入接口验证）、`core/CMakeLists.txt`、`app/CMakeLists.txt`、`app/MainWindow.h/.cpp`（接入 + 冒烟）

---

### Task 1: PostMessage + PostMessageInterface（core/schema）

**Files:**
- Create: `core/schema/PostMessage.h`
- Create: `core/schema/PostMessage.cpp`
- Create: `core/tests/test_PostMessage.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::schema::Level{Info,Warn,Error}`、`levelToString(Level)->std::string`、`qh::schema::PostMessage`（纯虚 `post(Level, const std::string&)`）、`qh::schema::PostMessageInterface`（mixin：`setPostMessage(PostMessage*)` + `protected PostMessage* _postMessage`）。Task 2 的 7 类继承 `PostMessageInterface`；Task 3 的 `QPostMessage` 继承 `PostMessage`。

- [ ] **Step 1: 写失败测试** `core/tests/test_PostMessage.cpp`

```cpp
#include "TestHarness.h"
#include "schema/PostMessage.h"
#include <string>
#include <vector>

namespace {

class FakePostMessage : public qh::schema::PostMessage {
public:
    void post(qh::schema::Level level, const std::string& msg) override {
        _levels.push_back(level);
        _messages.push_back(msg);
    }
    std::vector<qh::schema::Level> _levels;
    std::vector<std::string> _messages;
};

} // namespace

QH_TEST(postmessage_level_to_string) {
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Info), std::string("[INFO]"));
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Warn), std::string("[WARN]"));
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Error), std::string("[ERROR]"));
}

QH_TEST(postmessage_post_collects_messages) {
    FakePostMessage pm;
    pm.post(qh::schema::Level::Info, "hello");
    pm.post(qh::schema::Level::Error, "boom");
    QH_CHECK_EQ(pm._messages.size(), static_cast<size_t>(2));
    QH_CHECK_EQ(pm._messages[0], std::string("hello"));
    QH_CHECK(pm._levels[1] == qh::schema::Level::Error);
}

// 验证 PostMessageInterface 注入：继承它的类获得 setPostMessage + _postMessage
namespace {
class Holder : public qh::schema::PostMessageInterface {
public:
    qh::schema::PostMessage* posted() { return _postMessage; }
};
} // namespace

QH_TEST(postmessage_interface_injection) {
    Holder h;
    QH_CHECK(h.posted() == nullptr);  // 默认空
    FakePostMessage pm;
    h.setPostMessage(&pm);
    QH_CHECK(h.posted() == &pm);  // 注入后可取回
}
```

- [ ] **Step 2: 改 `core/CMakeLists.txt`，登记源与测试**

`qharness_core` 源列表的 `schema/Message.h schema/Message.cpp` 行之后插入：

```cmake
    schema/Message.h        schema/Message.cpp
    schema/PostMessage.h    schema/PostMessage.cpp
    engine/Engine.h
```

`qharness_core_tests` 源列表的 `tests/test_Message.cpp` 行之后插入：

```cmake
    tests/test_Message.cpp
    tests/test_PostMessage.cpp
    tests/test_ToolManager.cpp
```

- [ ] **Step 3: 重新配置 CMake**

Run: `cmake -S . -B build`（工作目录 `E:/MyHarness/QHarnesscc`）
Expected: 配置成功。

- [ ] **Step 4: 构建测试目标，验证失败（红）**

Run: `cmake --build build --config Release --target qharness_core_tests`
Expected: 编译失败 —— `schema/PostMessage.h`: No such file or directory。

- [ ] **Step 5: 写实现** `core/schema/PostMessage.h`

```cpp
#ifndef QH_SCHEMA_POSTMESSAGE_H
#define QH_SCHEMA_POSTMESSAGE_H
#include "qh_export.h"
#include <string>

namespace qh {
namespace schema {

// 消息级别（日志/通知的严重性）
enum class Level { Info, Warn, Error };

QH_API std::string levelToString(Level level);

// 实时消息发送抽象：core 类持有其指针，运行时向 Qt 窗口或其他接口推送日志/通知
// 纯 C++17 抽象，不依赖 Qt；app 提供 QPostMessage（QObject）实现
class QH_API PostMessage {
public:
    virtual ~PostMessage() = default;
    virtual void post(Level level, const std::string& message) = 0;
};

// 持有 PostMessage 的 mixin：core 类继承它获得「注入实时消息发送器」能力（DRY）
class QH_API PostMessageInterface {
public:
    virtual ~PostMessageInterface() = default;
    void setPostMessage(PostMessage* pm) { _postMessage = pm; }
protected:
    PostMessage* _postMessage = nullptr;  // 非拥有，由 app 注入
};

} // namespace schema
} // namespace qh
#endif // QH_SCHEMA_POSTMESSAGE_H
```

- [ ] **Step 6: 写实现** `core/schema/PostMessage.cpp`

```cpp
#include "schema/PostMessage.h"

namespace qh {
namespace schema {

std::string levelToString(Level level) {
    switch (level) {
        case Level::Info:  return "[INFO]";
        case Level::Warn:  return "[WARN]";
        case Level::Error: return "[ERROR]";
    }
    return "[INFO]";
}

} // namespace schema
} // namespace qh
```

- [ ] **Step 7: 构建 + 运行全部测试，验证通过（绿）**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全绿（原 79 + 3 个 `postmessage_*` 用例共 8 断言 ≈ 87 checks / 0 failures）。

---

### Task 2: 7 个骨架类继承 PostMessageInterface

> 重构（改基类继承），无新增测试，以全部测试回归 + `skeleton_compile_check` 注入接口验证为准。

**Files:**
- Modify: `core/engine/Engine.h`、`core/provider/Provider.h`、`core/tool/Tool.h`、`core/tool/ToolManager.h`、`core/interaction/Interaction.h`、`core/memory/Memory.h`、`core/context/Composer.h`
- Modify: `core/tests/skeleton_compile_check.cpp`

**Interfaces:**
- Consumes: `PostMessageInterface`（Task 1）。
- Produces: 7 类获得 `setPostMessage(PostMessage*)` 与 `protected _postMessage`；派生类（EngineReActLoop/ProviderOpenAI/Claude/MockProvider/各 Tool/InteractionFeishu/MemoryFile）继承可用。

- [ ] **Step 1: 整体替换** `core/engine/Engine.h`

```cpp
#ifndef QH_ENGINE_H
#define QH_ENGINE_H
#include <string>
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace engine {

// 运行过程类（抽象基类）；继承 PostMessageInterface 获得实时消息注入能力
class QH_API Engine : public schema::PostMessageInterface {
public:
    virtual ~Engine() = default;
    virtual void run(const std::string& userPrompt) = 0;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_H
```

- [ ] **Step 2: 改** `core/provider/Provider.h`（加 include + 继承，其余不变）

在 `#include "provider/CancellationToken.h"` 之后加一行：

```cpp
#include "provider/CancellationToken.h"
#include "schema/PostMessage.h"
#include "qh_export.h"
```

把 `class QH_API Provider {` 改为：

```cpp
class QH_API Provider : public schema::PostMessageInterface {
```

（`GenerateResult` 结构、`generate` 新签名、`virtual ~Provider() = default;` 均不变。）

- [ ] **Step 3: 整体替换** `core/tool/Tool.h`

```cpp
#ifndef QH_TOOL_H
#define QH_TOOL_H
#include "schema/Message.h"
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 工具抽象基类（edit/read/bash 等工具的基类）；继承 PostMessageInterface 获得实时消息注入
class QH_API Tool : public schema::PostMessageInterface {
public:
    virtual ~Tool() = default;
    virtual schema::ToolDefinition definition() const = 0;
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_H
```

- [ ] **Step 4: 改** `core/tool/ToolManager.h`（加 include + 继承）

在 `#include "tool/Tool.h"` 之后加 `#include "schema/PostMessage.h"`，并把 `class QH_API ToolManager {` 改为：

```cpp
class QH_API ToolManager : public schema::PostMessageInterface {
```

（其余方法/成员不变。）

- [ ] **Step 5: 整体替换** `core/interaction/Interaction.h`

```cpp
#ifndef QH_INTERACTION_H
#define QH_INTERACTION_H
#include <string>
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace interaction {

// 外部交互连接抽象基类（飞书等外部渠道）；继承 PostMessageInterface 获得实时消息注入
class QH_API Interaction : public schema::PostMessageInterface {
public:
    virtual ~Interaction() = default;
    virtual void send(const std::string& message) = 0;
};

} // namespace interaction
} // namespace qh
#endif // QH_INTERACTION_H
```

- [ ] **Step 6: 整体替换** `core/memory/Memory.h`

```cpp
#ifndef QH_MEMORY_H
#define QH_MEMORY_H
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace memory {

// 记忆系统抽象基类；继承 PostMessageInterface 获得实时消息注入
class QH_API Memory : public schema::PostMessageInterface {
public:
    virtual ~Memory() = default;
    virtual void load() = 0;
    virtual void save() = 0;
};

} // namespace memory
} // namespace qh
#endif // QH_MEMORY_H
```

- [ ] **Step 7: 改** `core/context/Composer.h`（加 include + 继承）

在 `#include "schema/Message.h"` 之后加 `#include "schema/PostMessage.h"`，并把 `class QH_API Composer {` 改为：

```cpp
class QH_API Composer : public schema::PostMessageInterface {
```

（构造函数、`build()`、`_workDir`/`_planMode` 成员不变。）

- [ ] **Step 8: 改** `core/tests/skeleton_compile_check.cpp`（验证注入接口）

在 `touchSkeletons()` 函数体的 `context::Composer cc("./", false); (void)cc.build();` 之后，追加：

```cpp
    // 验证 PostMessageInterface 注入接口（各骨架类继承后可用 setPostMessage）
    schema::PostMessage* nullPM = nullptr;
    e.setPostMessage(nullPM);
    po.setPostMessage(nullPM);
    pc.setPostMessage(nullPM);
    mf.setPostMessage(nullPM);
    rf.setPostMessage(nullPM);
    cc.setPostMessage(nullPM);
    tool::ToolManager tm;
    tm.setPostMessage(nullPM);
```

并在函数内 `provider::MockProvider mp;` 创建处之后（generate 调用所在的 `{}` 块内）加 `mp.setPostMessage(nullPM);`，即把该块改为：

```cpp
    {
        provider::CancellationToken token;
        std::vector<schema::Message> msgs;
        std::vector<schema::ToolDefinition> tools;
        (void)po.generate(token, msgs, tools);
        (void)pc.generate(token, msgs, tools);
        provider::MockProvider mp;
        (void)mp.generate(token, msgs, tools);
        mp.setPostMessage(nullPM);
    }
```

> 注：`nullPM` 需在 `{}` 块之前声明可见——把 `schema::PostMessage* nullPM = nullptr;` 移到 `touchSkeletons()` 函数体开头（`engine::EngineReActLoop e;` 之前）即可同时被块内 `mp` 与块后各实例使用。`tool::ToolManager` 需在文件顶部 include 块加 `#include "tool/ToolManager.h"`。

- [ ] **Step 9: 构建 + 运行全部测试，验证回归不破**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全绿（≈87 checks / 0 failures；继承重构不改变现有测试行为；`skeleton_compile_check` 编译通过即证明 7 类继承 + 注入接口可用）。

---

### Task 3: QPostMessage（app，Qt 实现）

**Files:**
- Create: `app/QPostMessage.h`
- Create: `app/QPostMessage.cpp`
- Modify: `app/CMakeLists.txt`

**Interfaces:**
- Consumes: `qh::schema::PostMessage`（Task 1）、`QPlainTextEdit`（Qt）。
- Produces: `qh::app::QPostMessage`（`QObject`+`PostMessage`，`Q_OBJECT`；构造 `QPostMessage(QPlainTextEdit* view, QObject* parent)`；`post(Level, const std::string&)` override；`signal messagePosted(Level, QString)`）。Task 4 的 MainWindow 创建并接入。

- [ ] **Step 1: 写实现** `app/QPostMessage.h`

```cpp
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
```

- [ ] **Step 2: 写实现** `app/QPostMessage.cpp`

```cpp
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
```

- [ ] **Step 3: 改** `app/CMakeLists.txt`，登记 QPostMessage

`qharness_app` 源列表中，在 `LogDock.cpp` 之后插入两行：

```cmake
    LogDock.h
    LogDock.cpp
    QPostMessage.h
    QPostMessage.cpp
    ChatDock.h
    ChatDock.cpp
```

- [ ] **Step 4: 重新配置 + 构建 app，验证编译（AUTOMOC 处理 Q_OBJECT）**

Run: `cmake -S . -B build && cmake --build build --config Release --target qharness_app`
Expected: 构建成功（moc 生成 `QPostMessage` 元对象代码；`qharness_app.exe` 产出至 `build/out/Release`）。

---

### Task 4: MainWindow 接入 QPostMessage + GUI 冒烟

**Files:**
- Modify: `app/MainWindow.h`
- Modify: `app/MainWindow.cpp`

**Interfaces:**
- Consumes: `QPostMessage`（Task 3）、`LogDock::view()`。
- Produces: MainWindow 持有 `QPostMessage* _postMessage`，构造时创建并接入 `_logDock->view()`，发一条 `[INFO]` 冒烟消息。

- [ ] **Step 1: 改** `app/MainWindow.h`（加 include + 成员）

在 `#include "ChatDock.h"` 之后加 `#include "QPostMessage.h"`，并在 `private:` 区（`LogDock* _logDock` 同处）加成员：

```cpp
private:
    void appendLog(const QString& text);

    LogDock* _logDock = nullptr;
    ChatDock* _chatDock = nullptr;
    QPostMessage* _postMessage = nullptr;
```

- [ ] **Step 2: 改** `app/MainWindow.cpp`（构造函数创建 QPostMessage + 冒烟消息）

在构造函数 `_logDock` 创建并 `addDockWidget` 之后（即 `addDockWidget(Qt::BottomDockWidgetArea, _logDock);` 之后、`_chatDock` 创建之前）插入：

```cpp
    // 实时消息发送器（Qt 实现），接入日志视图；core 组件后续 setPostMessage 注入
    _postMessage = new QPostMessage(_logDock->view(), this);
    _postMessage->post(qh::schema::Level::Info, "QHarnesscc 已就绪");
```

（需确保 `MainWindow.cpp` 顶部已通过 `#include "MainWindow.h"` 间接引入 `schema/PostMessage.h`，从而 `qh::schema::Level::Info` 可见——已满足。）

- [ ] **Step 3: 构建 app，验证编译**

Run: `cmake --build build --config Release --target qharness_app`
Expected: 构建成功。

- [ ] **Step 4: GUI 冒烟——运行确认启动不崩溃**

Run（带超时，避免 `app.exec()` 阻塞）：`timeout 6 build/out/Release/qharness_app.exe; echo "exit=$?"`
Expected: 进程启动、不崩溃（超时被终止返回非 0，如 124，属正常；关键是无 Qt 致命错误/崩溃弹窗）。LogDock 应显示 `[INFO] QHarnesscc 已就绪`（与既有 `[UI]...` 行）——内容正确性由 `connect` 逻辑保证。

- [ ] **Step 5: 回归 core 测试（确保 app 改动未碰 core）**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全绿（≈87 checks / 0 failures）。

---

## Self-Review

**1. Spec coverage**（对照设计 A–G）：
- §1 PostMessage + PostMessageInterface → Task 1 ✓
- §2 mixin 注入 7 类 → Task 2 ✓
- §3 QPostMessage（信号槽对接 view）→ Task 3 ✓
- §4 MainWindow 接入 + 冒烟 → Task 4 ✓
- §5 线程安全/所有权 → 由 Task 3 信号槽 AutoConnection + 非拥有指针实现 ✓
- §6 CMakeLists → Task 1/3/4 各自登记 ✓
- §7 测试（test_PostMessage + skeleton_check + GUI 冒烟）→ Task 1/2/4 ✓
- 范围边界（不做 core 实际发送/时间戳颜色/真正注入 Engine）→ 计划未含，符合 YAGNI ✓

**2. Placeholder scan**：无 TBD/TODO（`ProviderOpenAI/Claude/MockProvider` 等方法体的 `// TODO` 是既有骨架占位，非计划占位）；每步含完整代码 ✓

**3. Type consistency**：
- `PostMessage::post(Level, const std::string&)` 在 PostMessage.h、QPostMessage、FakePostMessage、test 一致 ✓
- `PostMessageInterface` 全程 `qh::schema::PostMessageInterface`，`setPostMessage(PostMessage*)` 一致 ✓
- 7 类继承签名一致（`: public schema::PostMessageInterface`）✓
- `QPostMessage::messagePosted(qh::schema::Level, const QString&)` 在 .h/.cpp 一致 ✓
- 枚举 `Level` 比较统一 `QH_CHECK(x==y)` ✓
- `nullPM` 作用域：Step 8 已注明移到函数体开头以同时覆盖块内 `mp` 与块后实例 ✓
