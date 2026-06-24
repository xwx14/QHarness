# PostMessage 抽象 + QPostMessage 实现 设计

- 日期：2026-06-24
- 状态：设计已确认，待实现
- 背景：core 类（Engine/Provider/Tool 等）运行时需实时向 Qt 窗口（LogDock）或其他接口推送日志/通知消息；core 不依赖 Qt，故用依赖反转——core 定义纯 C++ 抽象 `PostMessage`，app 提供 Qt 实现 `QPostMessage`。

## 设计决策

| 决策点 | 选择 | 理由 |
|---|---|---|
| 接口形态 | `post(Level, const std::string&)` + `Level{Info,Warn,Error}` | 统一接口 + 级别，覆盖日志/通知；Level 在 LogDock 加前缀区分 |
| 位置 | `core/schema/PostMessage.h` | 用户指定；PostMessage 为消息相关抽象，归 schema 模块 |
| 注入方式 | `PostMessageInterface` mixin，7 类继承 | DRY：注入逻辑集中到 mixin，避免每类重复写 setter/成员；用户指定 |
| QPostMessage 对接 | Qt 信号槽（`messagePosted` → `appendPlainText`） | idiomatic、解耦；`AutoConnection` 跨线程自动 `QueuedConnection` |
| 所有权 | `PostMessage*` 非拥有裸指针 | app 管 QPostMessage 生命周期（MainWindow 成员，Qt 父子清理） |

## 1. PostMessage 抽象（新增）

文件：`core/schema/PostMessage.h` + `core/schema/PostMessage.cpp`。纯 C++17、`QH_API` 导出、命名空间 `qh::schema`。**不做 JSON 序列化**（行为抽象，非数据结构）。

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
    // 发送一条消息（级别 + 内容）；实现负责跨线程投递到实际接收端
    virtual void post(Level level, const std::string& message) = 0;
};

} // namespace schema
} // namespace qh
#endif // QH_SCHEMA_POSTMESSAGE_H
```

`levelToString`：`Info→"[INFO]"`、`Warn→"[WARN]"`、`Error→"[ERROR]"`。

## 2. PostMessageInterface mixin + 7 个骨架类继承（DRY 注入）

集中注入逻辑到一个 mixin 接口类，7 个类继承它获得「可注入实时消息发送器」能力，避免每类重复写 `setPostMessage`/`_postMessage`。

**PostMessageInterface**（并入 `core/schema/PostMessage.h`，紧邻 PostMessage；遵循 schema 单文件多类风格）：

```cpp
// 持有 PostMessage 的 mixin：core 类继承它获得「注入实时消息发送器」能力（DRY）
class QH_API PostMessageInterface {
public:
    virtual ~PostMessageInterface() = default;
    void setPostMessage(PostMessage* pm) { _postMessage = pm; }
protected:
    PostMessage* _postMessage = nullptr;  // 非拥有，由 app 注入
};
```

7 个类继承 `schema::PostMessageInterface`（各自 `#include "schema/PostMessage.h"`）：
`Engine`、`Provider`、`Tool`、`ToolManager`、`Interaction`、`Memory`、`Composer`。`_postMessage` 为 `protected`，派生类（EngineReActLoop、ProviderOpenAI/Claude/MockProvider、各 Tool 派生、InteractionFeishu、MemoryFile）可直接访问发送。

示例（Engine）：
```cpp
#include "schema/PostMessage.h"
class QH_API Engine : public schema::PostMessageInterface {
public:
    virtual ~Engine() = default;  // Engine 亦为基类（EngineReActLoop 继承），显式虚析构
    virtual void run(const std::string& userPrompt) = 0;
};
```

> 注：`PostMessageInterface` 仅作 mixin（提供成员 + setter，无纯虚），不参与多态删除链；7 个类原本无基类，单继承它不引入菱形。

## 3. QPostMessage（app，新增）

文件：`app/QPostMessage.h` + `app/QPostMessage.cpp`。`QObject` + `PostMessage` 多继承，`Q_OBJECT`。

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

实现：构造时 `connect(messagePosted → _view)`，lambda 加级别前缀后 `appendPlainText`；`post()` 内 `emit messagePosted(level, QString::fromStdString(message))`。

```cpp
QPostMessage::QPostMessage(QPlainTextEdit* view, QObject* parent)
    : QObject(parent), _view(view) {
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
```

## 4. MainWindow 接入 + GUI 冒烟

- `MainWindow.h` 加成员 `QPostMessage* _postMessage = nullptr;`（include `QPostMessage.h`）
- 构造函数：`_postMessage = new QPostMessage(_logDock->view(), this);`
- 冒烟：构造末尾 `_postMessage->post(qh::schema::Level::Info, "QHarnesscc 已就绪");`，运行 GUI 验证 LogDock 显示该行（含 `[INFO]` 前缀）
- core 组件注入接口（`setPostMessage`）就绪，待 Engine 等创建时注入

## 5. 线程安全 / 所有权

- `PostMessage*` **非拥有**：QPostMessage 是 MainWindow 的 QObject 子，Qt 父子机制自动清理
- 跨线程：core 工作线程 `post()` → `emit messagePosted` → `AutoConnection`（接收方 `_view` 在 GUI 线程）自动 `QueuedConnection` → GUI 线程执行 `appendPlainText`，线程安全
- core 类不依赖 Qt：PostMessage 是纯 C++ 抽象，依赖反转

## 6. CMakeLists 登记

- `core/CMakeLists.txt`：`qharness_core` 加 `schema/PostMessage.h`/`.cpp`；`qharness_core_tests` 加 `tests/test_PostMessage.cpp`
- `app/CMakeLists.txt`：`qharness_app` 加 `QPostMessage.h`/`.cpp`（AUTOMOC 已开，Q_OBJECT 自动处理）

## 7. 测试

- `core/tests/test_PostMessage.cpp`：`Level` 枚举 + `levelToString`；`FakePostMessage`（实现 PostMessage，收集 `_levels`/`_messages`）验证 `post` 被调用
- `core/tests/skeleton_compile_check.cpp`：各基类 `setPostMessage(...)` 验证注入接口可用（编译/链接期）
- app GUI 冒烟：MainWindow 发 `[INFO] QHarnesscc 已就绪` → LogDock 显示（手动验证）

## 范围边界（YAGNI）

本次**不做**：
- core 类实际消息发送逻辑（方法体仍 TODO，待各模块实现时按需 `_postMessage->post(...)`）
- 时间戳、颜色、消息过滤（Level 前缀足够，颜色/过滤待后续）
- 真正创建 core 组件并注入（Engine 等尚未实例化于 app，待后续步骤）

## 受影响文件清单

**新增**：
- `core/schema/PostMessage.h` / `.cpp`（含 `Level`、`PostMessage`、`PostMessageInterface` 三者）
- `app/QPostMessage.h` / `.cpp`
- `core/tests/test_PostMessage.cpp`

**修改**（继承 `PostMessageInterface`）：
- `core/engine/Engine.h`
- `core/provider/Provider.h`
- `core/tool/Tool.h`、`core/tool/ToolManager.h`
- `core/interaction/Interaction.h`
- `core/memory/Memory.h`
- `core/context/Composer.h`
- `core/tests/skeleton_compile_check.cpp`（注入接口验证）
- `core/CMakeLists.txt`、`app/CMakeLists.txt`
- `app/MainWindow.h` / `.cpp`（持有 QPostMessage + 冒烟）
