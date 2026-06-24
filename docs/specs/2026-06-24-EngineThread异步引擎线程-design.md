# EngineThread 异步引擎线程 设计

- 日期：2026-06-24
- 状态：设计已确认，待实现
- 背景：`EngineReActLoop::run` 当前在 GUI 线程同步执行（mock 两轮瞬时，真实 provider 会阻塞 UI）。需用 QThread 异步化：app 新增 `EngineThread`（含 Engine 指针，负责运行，消息传到 LogDock），MainWindow 通过它异步驱动引擎。

## 设计决策

| 决策点 | 选择 | 理由 |
|---|---|---|
| 形态 | `EngineThread : public QThread`（重写 `run()`） | 直接、契合"跑一个长函数"场景 |
| Engine 所有权 | **非拥有** `Engine*` | 用户指定；调用方（MainWindow）成员持有 mock+engine 保证异步生命周期 |
| 消息传递 | 复用 `MainWindow._postMessage`（QPostMessage） | DRY，单一消息通道（UI+引擎共用）；EngineThread 构造内 `engine->setPostMessage` |
| prompt | 构造参数传入 | run() 无参（QThread 签名），prompt 作成员 |
| 并发 test | 忽略重复触发（`isRunning` 时 return） | YAGNI；避免并发悬空 |

## 1. EngineThread 类（新增）

文件：`app/EngineThread.h` + `app/EngineThread.cpp`。`QThread` 子类，`Q_OBJECT`（AUTOMOC）。命名空间 `qh::app`。

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
class EngineThread : public QThread {
    Q_OBJECT
public:
    // engine/postMessage 均非拥有（调用方保证生命周期长于本线程）
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

## 2. MainWindow 改造

`MainWindow.h` 新增成员（持有 mock + engine，保证异步生命周期）：
```cpp
#include <memory>
// ...
private:
    std::unique_ptr<qh::provider::MockProvider> _mockProvider;
    std::unique_ptr<qh::tool::MockBashTool> _mockBash;
    std::unique_ptr<qh::tool::ToolManager> _toolManager;
    std::unique_ptr<qh::engine::EngineReActLoop> _engine;
    EngineThread* _engineThread = nullptr;   // parent this；finished 时清 nullptr + deleteLater
```

`MainWindow::test()` 改为异步：
```cpp
void MainWindow::test() {
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

> `onChatSend()` 中既有的 `test()` 调用保持不变（用户代码）。

## 3. 线程模型（跨线程安全关键）

- **对象亲和性**：EngineThread 对象在 GUI 线程构造（`parent this`），亲和性 GUI；其 `run()` 在 worker 线程执行
- **消息跨线程**：worker 线程内 `engine->run` 调 `_postMessage->info/error` → `emit messagePosted`。`_postMessage`（QPostMessage）亲和性 GUI（MainWindow 创建），receiver 为 LogDock::view()（GUI）→ `AutoConnection` 自动判定为 `QueuedConnection`（emit 线程 worker ≠ receiver 线程 GUI）→ 消息排队到 GUI 事件循环，`appendPlainText` 在 GUI 线程执行。**线程安全**
- **生命周期**：mock + engine 由 MainWindow 成员（unique_ptr）持有，生命周期 = MainWindow，长于任何一次 worker 线程运行（test() 重建前确保旧线程已结束：`isRunning` 检查 + 忽略并发）
- **线程对象清理**：`finished` 信号 → 清 `_engineThread = nullptr`（避免悬空成员）+ `deleteLater`（Qt 事件循环删除线程对象）

## 4. CMakeLists

`app/CMakeLists.txt`：`qharness_app` 源列表加 `EngineThread.h`/`.cpp`（AUTOMOC 已开，Q_OBJECT 自动处理）。

## 5. 测试

- EngineThread 是 QThread（QObject），依赖 QApplication 事件循环，难以纯单测
- **GUI 冒烟**：点击「测试 ReAct」按钮 → `test()` 异步启动 → LogDock 实时滚动显示 ReAct 循环日志（Turn 1 思考→工具执行→Turn 2 任务完成），UI 不阻塞（按钮可点）
- run 本身的 ReAct 逻辑已由 `test_EngineReActLoop`（core 集成测试，104 checks）覆盖

## 范围边界（YAGNI）

本次**不做**：
- finished 的 UI 反馈（如跑时禁用按钮、状态栏提示）
- 真实 provider（OpenAI/Claude）接入——仍用 MockProvider/MockBashTool
- 多引擎并发（同一时刻仅一个 EngineThread 跑，并发触发忽略）
- 线程取消/中断（CancellationToken 已存在但 test 不主动取消）

## 受影响文件清单

**新增**：
- `app/EngineThread.h` / `.cpp`

**修改**：
- `app/MainWindow.h`（mock + engine + _engineThread 成员）
- `app/MainWindow.cpp`（`test()` 改异步、include `<memory>`/`<QDir>`/core mock 头已在）
- `app/CMakeLists.txt`（登记 EngineThread）
