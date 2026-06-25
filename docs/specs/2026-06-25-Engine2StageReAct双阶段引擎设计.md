# Engine2StageReAct 双阶段引擎设计

- 日期：2026-06-25
- 状态：已批准，待实现
- 参考实现：`E:\MyHarness\go-tiny-claw\internal\engine\loop.go`（TwoStageReAct）

## 1. 背景与目标

Go 参考实现的 `AgentEngine.Run` 已升级为 **TwoStageReAct**：在每轮 ReAct 循环的 Action 阶段之前，插入一个**可开关的 Phase 1 慢思考阶段**（剥夺工具、强制纯文本规划），Phase 2 再带着思考 trace 恢复工具行动。现有 C++ `EngineReActLoop` 的循环体等价于它的 Phase 2（单阶段）。

目标：在 C++ 侧新建 `Engine2StageReAct` 类，忠实移植 TwoStageReAct 逻辑，与 `EngineReActLoop` 平级共存。

## 2. 设计决策

### 2.1 继承结构：直接继承 `Engine`

`Engine2StageReAct : public Engine`，与 `EngineReActLoop` 平级。

- **理由**：2Stage 的 `run` 循环骨架因 Phase1 插入而与 ReActLoop 不同，继承 ReActLoop 需整体 override 且 is-a 关系牵强；直接继承 `Engine` 语义清晰（"是一种 Engine"），`run` 逻辑自洽。
- **取舍**：Phase 2 循环体与 `EngineReActLoop` 有约 30 行重复，但两引擎循环结构存在微妙差异，强行抽公共基类属当下过度设计（KISS > 跨类 DRY，YAGNI）。

### 2.2 慢思考开关注入

对应 Go 的 `EnableThinking` 字段，采用**构造参数 + setter 双通道**：

- 构造参数 `bool enableThinking = true`（**默认开启**）
- setter `void setEnableThinking(bool enable)`（运行时可切换）

### 2.3 剥夺工具的表达

`Provider::generate(cancel, messages, tools)` 传**空 `std::vector<ToolDefinition>{}`** 即对应 Go 的 `nil`（Phase1 不挂载任何工具）。

## 3. 类设计

### 3.1 Engine2StageReAct（`core/engine/Engine2StageReAct.h` / `.cpp`）

```cpp
#ifndef QH_ENGINE_2STAGE_REACT_H
#define QH_ENGINE_2STAGE_REACT_H
#include "engine/Engine.h"
#include "qh_export.h"

namespace qh {
namespace engine {

// 双阶段 ReAct 引擎：Phase1 慢思考（剥夺工具，可开关）+ Phase2 行动（恢复工具）
// 忠实移植 Go loop.go 的 TwoStageReAct
class QH_API Engine2StageReAct : public Engine {
public:
    Engine2StageReAct(provider::Provider* provider, tool::ToolManager* toolManager,
                      std::string workDir, bool enableThinking = true);
    void setEnableThinking(bool enable);
    void run(const std::string& userPrompt) override;

private:
    bool _enableThinking;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_2STAGE_REACT_H
```

成员 `_enableThinking` 遵循项目约定（下划线开头）。`_provider`/`_toolManager`/`_workDir` 复用 `Engine` 基类 protected 成员；`info`/`error` 来自 `PostMessageInterface`。

### 3.2 run() 流程（移植 `loop.go`）

```
info("引擎启动，锁定工作区: " + _workDir)
info("慢思考模式 (Thinking Phase): " + (_enableThinking ? "开启" : "关闭"))

history = {
    {Role::System, "You are go-tiny-claw, an expert coding assistant. You have full access to tools in the workspace."},
    {Role::User, userPrompt},
}

for (turn = 1; ; ++turn) {
    info("========== [Turn " + turn + "] 开始 ==========")
    CancellationToken token
    tools = _toolManager->getAvailableTools()

    // Phase 1: 慢思考（仅当 _enableThinking）
    if (_enableThinking) {
        info("[Phase 1] 剥夺工具访问权，强制进入慢思考与规划阶段...")
        thinkResult = _provider->generate(token, history, {})   // 空 tools
        if (!thinkResult.error.empty()) { error("Thinking 阶段生成失败: " + ...); return; }
        if (!thinkResult.message._content.empty()) {
            info("内部思考 Trace: " + thinkResult.message._content)
            history.push_back(thinkResult.message)
        }
    }

    // Phase 2: 行动
    info("[Phase 2] 恢复工具挂载，等待模型采取行动...")
    actionResult = _provider->generate(token, history, tools)
    if (!actionResult.error.empty()) { error("Action 阶段生成失败: " + ...); return; }
    history.push_back(actionResult.message)
    if (!actionResult.message._content.empty()) info("模型: " + ...)

    // 退出条件
    if (actionResult.message._toolCalls.empty()) { info("模型未请求调用工具，任务完成。"); break; }

    // 执行工具 + 观察回填
    info("模型请求调用 N 个工具...")
    for (call : actionResult.message._toolCalls) {
        info("  -> 执行工具: " + call._name + ", 参数: " + call._arguments)
        toolResult = _toolManager->execute(call)
        if (toolResult._isError) error("  -> 工具执行报错: " + toolResult._output)
        else info("  -> 工具执行成功 (返回 N 字节)")
        observation = {Role::User, toolResult._output, toolCallId = call._id}
        history.push_back(observation)
    }
}
```

`_enableThinking == false` 时跳过 Phase1，行为等同 `EngineReActLoop`。

## 4. 测试设计

### 4.1 MockTwoStageProvider（`core/provider/MockTwoStageProvider.h` / `.cpp`）

现有 `MockProvider` 按 `generate` 调用次数返回且**忽略 tools 参数**，无法区分两阶段。新建 mock 按 **tools 是否为空** 区分：

- `tools` 为空（Phase1）→ 返回纯文本思考（Assistant，无 toolCalls），如 `"让我先分析一下任务，规划执行步骤。"`
- `tools` 非空（Phase2）→ 内部 `_actionTurn` 计数：第 1 次返回 `bash` 工具调用、第 2 次返回完成文本

### 4.2 测试用例（`core/tests/test_Engine2StageReAct.cpp`）

复用 `test_EngineReActLoop.cpp` 的 `FakePostMessage` / `hasInfoContaining` / `hasAnyError` 模式：

1. `engine2stage_thinking_on_runs_two_phases`：默认 `enableThinking=true`，断言日志含 "内部思考 Trace" + "执行工具" + "任务完成"，且无 error。
2. `engine2stage_thinking_off_behaves_like_react`：`setEnableThinking(false)` 后，断言日志**不含** "内部思考 Trace"，仍含 "执行工具" + "任务完成"。

## 5. CMake 注册

`core/CMakeLists.txt`：

- `qharness_core` 源列表增：
  - `engine/Engine2StageReAct.h` `engine/Engine2StageReAct.cpp`
  - `provider/MockTwoStageProvider.h` `provider/MockTwoStageProvider.cpp`
- `qharness_core_tests` 源列表增：
  - `tests/test_Engine2StageReAct.cpp`

include guard 命名遵循项目约定 `QH_<模块>_<文件>_H`（如 `QH_ENGINE_2STAGE_REACT_H`、`QH_PROVIDER_MOCK_2STAGE_H`）。

## 6. 不改动范围

`MainWindow` 继续使用 `EngineReActLoop`。本次仅新增 `Engine2StageReAct` 及其专用 mock 与测试，**不切换** UI 接入。如需将"测试 ReAct"按钮切到 2Stage，另行提议。

## 7. 错误处理

`generate` 返回非空 `error` → `error()` 日志 + `return`（与 `EngineReActLoop` 一致，对齐 Go 的 `fmt.Errorf` 返回）。
