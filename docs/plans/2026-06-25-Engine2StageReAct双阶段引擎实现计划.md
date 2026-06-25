# Engine2StageReAct 双阶段引擎 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新建 `Engine2StageReAct` 类，忠实移植 Go `loop.go` 的 TwoStageReAct 双阶段逻辑（Phase1 可开关慢思考 + Phase2 行动），与现有 `EngineReActLoop` 平级共存。

**Architecture:** `Engine2StageReAct` 直接继承 `Engine`，每轮 Turn 先按 `_enableThinking` 决定是否跑 Phase1（向 provider 传空 tools 剥夺工具、强制纯文本思考并追加进 history），再跑 Phase2（恢复全部 tools、生成行动/工具调用）。配套新建 `MockTwoStageProvider`（按 tools 空与否区分两阶段）支撑单测。

**Tech Stack:** C++17 / MSVC (`/utf-8`) / CMake / 项目自带 `TestHarness.h` 测试框架 / nlohmann json（既有）。

## Global Constraints

- C++17（`CMAKE_CXX_STANDARD 17 REQUIRED ON`），MSVC `/utf-8`（中文注释/字符串）。
- include guard 命名 `QH_<模块>_<文件>_H`；所有 `.h` 必须列入 CMakeLists 源列表（VS 工程可见）。
- 类成员变量下划线开头（`_enableThinking`）；变量/函数驼峰命名；注释中文。
- `core/` 纯 C++17，**不依赖 Qt**；`core/CMakeLists.txt` 不链接 Qt。
- core 为 SHARED DLL，测试 exe 必须在 `build/out/<config>/` 运行（与 `qharness_core.dll` 同目录）。
- 测试框架无法只跑单个用例，验证时跑全量 `qharness_core_tests.exe`，观察新用例的 FAIL/PASS。
- CMakeLists 改动后，`cmake --build` 会经 VS 生成器 ZERO_CHECK 自动重新配置，无需手动 `cmake -S . -B build`。
- **不主动 git 提交**（项目约定：未经用户明确要求不执行 commit）；每个任务以「构建 + 测试通过」为完成标志。

## File Structure

| 文件 | 责任 | 动作 |
|------|------|------|
| `core/provider/MockTwoStageProvider.h` | 双阶段 mock 声明（按 tools 空与否区分 Phase1/Phase2） | 新建 |
| `core/provider/MockTwoStageProvider.cpp` | mock 行为实现 | 新建 |
| `core/tests/test_MockTwoStageProvider.cpp` | mock 两阶段行为单测 | 新建 |
| `core/engine/Engine2StageReAct.h` | 双阶段引擎声明（继承 Engine，构造加 `enableThinking=true`，setter） | 新建 |
| `core/engine/Engine2StageReAct.cpp` | `run()` 双阶段循环实现 | 新建 |
| `core/tests/test_Engine2StageReAct.cpp` | 引擎 thinking on/off 两个用例 | 新建 |
| `core/CMakeLists.txt` | 注册新源/新测试 | 修改 |
| `core/tests/skeleton_compile_check.cpp` | 实例化新派生类做链接期检查 | 修改 |

---

## Task 1: MockTwoStageProvider 及其单测

**Files:**
- Create: `core/provider/MockTwoStageProvider.h`
- Create: `core/provider/MockTwoStageProvider.cpp`
- Create: `core/tests/test_MockTwoStageProvider.cpp`
- Modify: `core/CMakeLists.txt`
- Modify: `core/tests/skeleton_compile_check.cpp`

**Interfaces:**
- Consumes: `qh::provider::Provider`（基类，`generate(cancel, messages, tools) -> GenerateResult`）、`qh::schema::{CancellationToken, Message, ToolDefinition}`、测试宏 `QH_TEST/QH_CHECK`。
- Produces: `qh::provider::MockTwoStageProvider`（默认可构造，`generate` 按 tools 空与否返回思考/行动）—— Task 2 引擎测试依赖此类。

- [ ] **Step 1: 写 mock 头文件声明**

创建 `core/provider/MockTwoStageProvider.h`：

```cpp
#ifndef QH_PROVIDER_MOCK_2STAGE_H
#define QH_PROVIDER_MOCK_2STAGE_H
#include "provider/Provider.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// 双阶段模拟大模型，用于测试 Engine2StageReAct 的 Phase1/Phase2 语义
// 按 tools 是否为空区分阶段（对齐 Engine2StageReAct 的两阶段调用约定）：
//   - tools 为空（Phase1 慢思考）→ 返回纯文本思考（Assistant，无 toolCalls）
//   - tools 非空（Phase2 行动）   → _actionTurn 计数：第1次 bash 工具调用、第2次完成文本
class QH_API MockTwoStageProvider : public Provider {
public:
    GenerateResult generate(
        const schema::CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

private:
    int _actionTurn{0};   // 仅 Phase2（tools 非空）累计
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_MOCK_2STAGE_H
```

- [ ] **Step 2: 写 mock 最小骨架（generate 返回空结果，使测试可链接、运行期 FAIL）**

创建 `core/provider/MockTwoStageProvider.cpp`：

```cpp
#include "provider/MockTwoStageProvider.h"

namespace qh {
namespace provider {

GenerateResult MockTwoStageProvider::generate(
    const schema::CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& /*tools*/) {
    GenerateResult result;   // 骨架：空实现，待 Step 6 填真实行为
    return result;
}

} // namespace provider
} // namespace qh
```

- [ ] **Step 3: 注册 CMake（core 源列表 + tests 列表）**

修改 `core/CMakeLists.txt`。

在 `qharness_core` 源列表中，`provider/MockProvider.h provider/MockProvider.cpp` 一行之后插入：

```
    provider/MockTwoStageProvider.h provider/MockTwoStageProvider.cpp
```

在 `qharness_core_tests` 源列表中，`tests/test_MockProvider.cpp` 一行之后插入：

```
    tests/test_MockTwoStageProvider.cpp
```

- [ ] **Step 4: 写 mock 行为单测**

创建 `core/tests/test_MockTwoStageProvider.cpp`：

```cpp
#include "TestHarness.h"
#include "provider/MockTwoStageProvider.h"
#include "schema/CancellationToken.h"
#include "schema/Message.h"
#include <vector>

// Phase1：tools 为空 → 纯文本思考，无 toolCalls
QH_TEST(mocktwostage_empty_tools_returns_thinking) {
    qh::provider::MockTwoStageProvider provider;
    qh::schema::CancellationToken token;
    std::vector<qh::schema::Message> msgs;
    std::vector<qh::schema::ToolDefinition> noTools;   // 空 = Phase1

    qh::provider::GenerateResult r = provider.generate(token, msgs, noTools);

    QH_CHECK(r.message._role == qh::schema::Role::Assistant);
    QH_CHECK(!r.message._content.empty());
    QH_CHECK(r.message._toolCalls.empty());
    QH_CHECK(r.error.empty());
}

// Phase2：tools 非空 → 第1次 bash 调用，第2次完成文本
QH_TEST(mocktwostage_nonempty_tools_action_turns) {
    qh::provider::MockTwoStageProvider provider;
    qh::schema::CancellationToken token;
    std::vector<qh::schema::Message> msgs;
    std::vector<qh::schema::ToolDefinition> tools(1);   // 非空 = Phase2

    qh::provider::GenerateResult r1 = provider.generate(token, msgs, tools);
    QH_CHECK(r1.message._toolCalls.size() == 1);
    QH_CHECK(r1.message._toolCalls[0]._name == "bash");

    qh::provider::GenerateResult r2 = provider.generate(token, msgs, tools);
    QH_CHECK(r2.message._toolCalls.empty());
    QH_CHECK(!r2.message._content.empty());
}
```

> 说明：`Role` 是 `enum class` 无 `operator<<`，故用 `QH_CHECK(==)` 而非 `QH_CHECK_EQ`（后者会用 `<<` 打印导致编译失败）。

- [ ] **Step 5: 构建 + 运行测试，验证 FAIL（red）**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe
```
Expected: 两个新用例 FAIL（骨架返回空 `GenerateResult`，`_content` 空、`_toolCalls` 空，断言不满足）。stderr 含 `FAIL: !r.message._content.empty()` 等，末行形如 `XXX checks, 2 failures`（failures ≥ 2）。

- [ ] **Step 6: 实现 mock 完整行为**

用以下内容**整体替换** `core/provider/MockTwoStageProvider.cpp`：

```cpp
#include "provider/MockTwoStageProvider.h"

namespace qh {
namespace provider {

GenerateResult MockTwoStageProvider::generate(
    const schema::CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& tools) {
    GenerateResult result;
    result.message._role = schema::Role::Assistant;

    if (tools.empty()) {
        // Phase 1 慢思考：剥夺工具，纯文本规划
        result.message._content = "让我先分析一下任务，规划执行步骤。";
    } else {
        // Phase 2 行动：第1次 bash 调用，第2次完成文本
        ++_actionTurn;
        if (_actionTurn == 1) {
            schema::ToolCall call;
            call._id = "call_2stage_1";
            call._name = "bash";
            call._arguments = "{\"command\": \"ls -la\"}";
            result.message._toolCalls.push_back(std::move(call));
        } else {
            result.message._content = "我看到了文件列表，任务完成！";
        }
    }
    return result;
}

} // namespace provider
} // namespace qh
```

- [ ] **Step 7: 构建 + 运行测试，验证 PASS（green）**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe
```
Expected: `mocktwostage_empty_tools_returns_thinking` 与 `mocktwostage_nonempty_tools_action_turns` 均通过，末行 `XXX checks, 0 failures`。

- [ ] **Step 8: 补 skeleton 链接期检查**

修改 `core/tests/skeleton_compile_check.cpp`：

在文件顶部 include 区，`#include "provider/MockProvider.h"` 之后加：

```cpp
#include "provider/MockTwoStageProvider.h"
```

在 `touchSkeletons()` 内、`provider::MockProvider mp;` 那段的大括号块中加入（与 `mp` 并列）：

```cpp
        provider::MockTwoStageProvider m2;
        (void)m2.generate(token, msgs, tools);
        m2.setPostMessage(nullPM);
```

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe
```
Expected: 末行 `XXX checks, 0 failures`（新增的 skeleton 实例化若签名错误会在编译/链接期暴露）。

**Task 1 完成标志：** mock 完整实现、单测通过、skeleton 链接通过。是否 git 提交由用户决定。

---

## Task 2: Engine2StageReAct 及其双阶段单测

**Files:**
- Create: `core/engine/Engine2StageReAct.h`
- Create: `core/engine/Engine2StageReAct.cpp`
- Create: `core/tests/test_Engine2StageReAct.cpp`
- Modify: `core/CMakeLists.txt`
- Modify: `core/tests/skeleton_compile_check.cpp`

**Interfaces:**
- Consumes: `qh::engine::Engine`（基类，protected 成员 `_provider`/`_toolManager`/`_workDir`，继承自 `PostMessageInterface` 的 `info/error/setPostMessage`）、`qh::provider::{Provider, GenerateResult}`、`qh::tool::ToolManager`（`getAvailableTools`/`execute`）、`qh::schema::{Message, CancellationToken, ToolResult}`、Task 1 产出的 `MockTwoStageProvider`、测试辅助 `FakePostMessage`。
- Produces: `qh::engine::Engine2StageReAct(provider, toolManager, workDir, enableThinking=true)` + `setEnableThinking(bool)` + `run(userPrompt)`。

- [ ] **Step 1: 写引擎头文件声明**

创建 `core/engine/Engine2StageReAct.h`：

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
    // 慢思考默认开启；workDir 之后追加 enableThinking（基类构造不含此参数）
    Engine2StageReAct(provider::Provider* provider, tool::ToolManager* toolManager,
                      std::string workDir, bool enableThinking = true);
    void setEnableThinking(bool enable);   // 运行时切换
    void run(const std::string& userPrompt) override;

private:
    bool _enableThinking;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_2STAGE_REACT_H
```

- [ ] **Step 2: 写引擎最小骨架（构造/setter 落实，run 仅启动日志，使测试可链接、运行期 FAIL）**

创建 `core/engine/Engine2StageReAct.cpp`：

```cpp
#include "engine/Engine2StageReAct.h"
#include <utility>   // std::move

namespace qh {
namespace engine {

Engine2StageReAct::Engine2StageReAct(provider::Provider* provider,
                                     tool::ToolManager* toolManager,
                                     std::string workDir, bool enableThinking)
    : Engine(provider, toolManager, std::move(workDir)), _enableThinking(enableThinking) {}

void Engine2StageReAct::setEnableThinking(bool enable) {
    _enableThinking = enable;
}

void Engine2StageReAct::run(const std::string& /*userPrompt*/) {
    info("引擎启动，锁定工作区: " + _workDir);   // 骨架：仅启动日志，待 Step 6 填完整循环
}

} // namespace engine
} // namespace qh
```

- [ ] **Step 3: 注册 CMake（core 源列表 + tests 列表）**

修改 `core/CMakeLists.txt`。

在 `qharness_core` 源列表中，`engine/EngineReActLoop.h engine/EngineReActLoop.cpp` 一行之后插入：

```
    engine/Engine2StageReAct.h engine/Engine2StageReAct.cpp
```

在 `qharness_core_tests` 源列表中，`tests/test_EngineReActLoop.cpp` 一行之后插入：

```
    tests/test_Engine2StageReAct.cpp
```

- [ ] **Step 4: 写引擎双阶段单测**

创建 `core/tests/test_Engine2StageReAct.cpp`：

```cpp
#include "TestHarness.h"
#include "engine/Engine2StageReAct.h"
#include "provider/MockTwoStageProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include "schema/PostMessage.h"
#include "schema/Message.h"
#include <string>
#include <vector>

namespace {

// 收集 PostMessage 消息的 fake（复用 test_EngineReActLoop 的模式）
class FakePostMessage : public qh::schema::PostMessage {
public:
    void post(qh::schema::Level level, const std::string& msg) override {
        _levels.push_back(level);
        _messages.push_back(msg);
    }
    std::vector<qh::schema::Level> _levels;
    std::vector<std::string> _messages;
};

bool hasInfoContaining(const FakePostMessage& pm, const std::string& sub) {
    for (size_t i = 0; i < pm._messages.size(); ++i) {
        if (pm._levels[i] == qh::schema::Level::Info &&
            pm._messages[i].find(sub) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool hasAnyError(const FakePostMessage& pm) {
    for (size_t i = 0; i < pm._levels.size(); ++i) {
        if (pm._levels[i] == qh::schema::Level::Error) return true;
    }
    return false;
}

} // namespace

// enableThinking 默认 true：应同时出现思考 trace、工具执行、任务完成
QH_TEST(engine2stage_thinking_on_runs_two_phases) {
    qh::provider::MockTwoStageProvider provider;
    qh::tool::MockBashTool bash;
    qh::tool::ToolManager tm;
    tm.registerTool(bash);

    qh::engine::Engine2StageReAct engine(&provider, &tm, "./");   // enableThinking 默认 true
    FakePostMessage pm;
    engine.setPostMessage(&pm);

    engine.run("帮我检查当前目录的文件");

    QH_CHECK(hasInfoContaining(pm, "内部思考 Trace"));   // Phase1 产出
    QH_CHECK(hasInfoContaining(pm, "执行工具"));          // Phase2 工具调用
    QH_CHECK(hasInfoContaining(pm, "任务完成"));          // 循环正常退出
    QH_CHECK(!hasAnyError(pm));
}

// enableThinking=false：跳过 Phase1，无思考 trace，仍完成 ReAct 循环
QH_TEST(engine2stage_thinking_off_behaves_like_react) {
    qh::provider::MockTwoStageProvider provider;
    qh::tool::MockBashTool bash;
    qh::tool::ToolManager tm;
    tm.registerTool(bash);

    qh::engine::Engine2StageReAct engine(&provider, &tm, "./", false);   // 显式关闭
    FakePostMessage pm;
    engine.setPostMessage(&pm);

    engine.run("帮我检查当前目录的文件");

    QH_CHECK(!hasInfoContaining(pm, "内部思考 Trace"));   // 关闭则无 Phase1
    QH_CHECK(hasInfoContaining(pm, "执行工具"));
    QH_CHECK(hasInfoContaining(pm, "任务完成"));
    QH_CHECK(!hasAnyError(pm));
}
```

- [ ] **Step 5: 构建 + 运行测试，验证 FAIL（red）**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe
```
Expected: 两个引擎用例 FAIL（骨架 `run` 仅输出启动日志，无 "内部思考 Trace"/"执行工具"/"任务完成"）。stderr 含对应 `FAIL:` 行，末行 `XXX checks, 8 failures`（failures ≥ 1，实际两个用例共 8 条断言均不满足）。

- [ ] **Step 6: 实现 run 完整双阶段循环**

用以下内容**整体替换** `core/engine/Engine2StageReAct.cpp`：

```cpp
#include "engine/Engine2StageReAct.h"
#include "provider/Provider.h"
#include "tool/ToolManager.h"
#include "schema/Message.h"
#include "schema/CancellationToken.h"
#include <string>
#include <utility>
#include <vector>

namespace qh {
namespace engine {

Engine2StageReAct::Engine2StageReAct(provider::Provider* provider,
                                     tool::ToolManager* toolManager,
                                     std::string workDir, bool enableThinking)
    : Engine(provider, toolManager, std::move(workDir)), _enableThinking(enableThinking) {}

void Engine2StageReAct::setEnableThinking(bool enable) {
    _enableThinking = enable;
}

void Engine2StageReAct::run(const std::string& userPrompt) {
    info("引擎启动，锁定工作区: " + _workDir);
    info(_enableThinking ? "慢思考模式 (Thinking Phase): 开启"
                         : "慢思考模式 (Thinking Phase): 关闭");

    // 初始化会话上下文（system + user）
    std::vector<schema::Message> history = {
        {schema::Role::System, "You are go-tiny-claw, an expert coding assistant. You have full access to tools in the workspace."},
        {schema::Role::User, userPrompt},
    };

    for (int turn = 1; ; ++turn) {
        info("========== [Turn " + std::to_string(turn) + "] 开始 ==========");
        schema::CancellationToken token;
        std::vector<schema::ToolDefinition> tools = _toolManager->getAvailableTools();

        // ===== Phase 1: 慢思考（仅当 _enableThinking）——剥夺工具，强制纯文本规划 =====
        if (_enableThinking) {
            info("[Phase 1] 剥夺工具访问权，强制进入慢思考与规划阶段...");
            provider::GenerateResult thinkResult = _provider->generate(token, history, {});
            if (!thinkResult.error.empty()) {
                error("Thinking 阶段生成失败: " + thinkResult.error);
                return;
            }
            if (!thinkResult.message._content.empty()) {
                info("内部思考 Trace: " + thinkResult.message._content);
                history.push_back(thinkResult.message);
            }
        }

        // ===== Phase 2: 行动——恢复工具挂载，顺着规划执行 =====
        info("[Phase 2] 恢复工具挂载，等待模型采取行动...");
        provider::GenerateResult actionResult = _provider->generate(token, history, tools);
        if (!actionResult.error.empty()) {
            error("Action 阶段生成失败: " + actionResult.error);
            return;
        }
        history.push_back(actionResult.message);
        if (!actionResult.message._content.empty()) {
            info("模型: " + actionResult.message._content);
        }

        // 退出条件：模型未请求工具，任务完成
        if (actionResult.message._toolCalls.empty()) {
            info("模型未请求调用工具，任务完成。");
            break;
        }

        // 执行工具 + 观察（Observation）回填为 User 消息，携带 ToolCallID 维系推理链
        info("模型请求调用 " + std::to_string(actionResult.message._toolCalls.size()) + " 个工具...");
        for (const auto& call : actionResult.message._toolCalls) {
            info("  -> 执行工具: " + call._name + ", 参数: " + call._arguments);
            schema::ToolResult toolResult = _toolManager->execute(call);
            if (toolResult._isError) {
                error("  -> 工具执行报错: " + toolResult._output);
            } else {
                info("  -> 工具执行成功 (返回 " + std::to_string(toolResult._output.size()) + " 字节)");
            }
            schema::Message observation;
            observation._role = schema::Role::User;
            observation._content = toolResult._output;
            observation._toolCallId = call._id;
            history.push_back(observation);
        }
    }
}

} // namespace engine
} // namespace qh
```

- [ ] **Step 7: 构建 + 运行测试，验证 PASS（green）**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe
```
Expected: `engine2stage_thinking_on_runs_two_phases` 与 `engine2stage_thinking_off_behaves_like_react` 均通过，末行 `XXX checks, 0 failures`。

- [ ] **Step 8: 补 skeleton 链接期检查（验证构造签名 + setter）**

修改 `core/tests/skeleton_compile_check.cpp`：

顶部 include 区，`#include "engine/EngineReActLoop.h"` 之后加：

```cpp
#include "engine/Engine2StageReAct.h"
```

在 `touchSkeletons()` 内、`engine::EngineReActLoop e(nullptr, nullptr, ".");` 之后加（注意：此处 provider/toolManager 为 nullptr，**不可调 run**，仅验证构造与 setter）：

```cpp
    engine::Engine2StageReAct e2(nullptr, nullptr, ".", true);
    e2.setEnableThinking(false);
```

并在文件末尾 `setPostMessage` 验证段（`e.setPostMessage(nullPM);` 附近）加：

```cpp
    e2.setPostMessage(nullPM);
```

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe
```
Expected: 末行 `XXX checks, 0 failures`（构造/setter 签名错误会在编译期暴露）。

**Task 2 完成标志：** 引擎完整实现、两个双阶段用例通过、skeleton 链接通过。是否 git 提交由用户决定。

---

## 整体验证（全部任务完成后）

- [ ] **全量构建 + 全量测试**

Run:
```bash
cmake --build build --config Release && build/out/Release/qharness_core_tests.exe
```
Expected: `qharness_app.exe` / `qharness_core.dll` / `qharness_core_tests.exe` 均生成；测试末行 `XXX checks, 0 failures`（含原有 104 + 新增 mock 2 用例 + 引擎 2 用例的全部 check）。

## Self-Review 结论

- **Spec 覆盖**：继承结构（Task 2 头文件）、enableThinking 构造默认 true + setter（Task 2 Step 1/6）、Phase1 剥夺工具传空 tools（Task 2 Step 6）、Phase2 恢复工具（同）、thinking trace 追加 history（同）、MockTwoStageProvider 按 tools 区分（Task 1）、两个测试用例（Task 2 Step 4）、CMake 注册（两任务 Step 3）、不改动 MainWindow（计划全程未触及 app/）—— spec 各节均有对应任务。
- **占位符扫描**：所有代码块为完整可用代码，无 TBD/TODO/"类似上文"。
- **类型一致**：`Engine2StageReAct(provider, toolManager, workDir, enableThinking=true)` 在声明、skeleton、测试中签名一致；`setEnableThinking(bool)` 名称统一；`MockTwoStageProvider` 在 Task 1 产出、Task 2 消费，类名一致。
