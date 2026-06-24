# MockProvider 与 generate 接口改造 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 `Provider::generate` 接口完整对齐 Go `Generate(ctx, messages, tools) (*Message, error)`（错误返回值 + 取消/超时参数），并新增忠实移植 Go `mockProvider` 行为的 `MockProvider`。

**Architecture:** 新增轻量 `CancellationToken`（取消 + 可选 deadline，对齐 Go `context.Context` 核心语义，不含 Value map）置于 `qh::provider`；`generate` 返回 `GenerateResult{ Message; std::string error }`（error 空串=成功）；`MockProvider` 以 `_turn` 计数复现 Go mock 行为。改接口零破坏——`EngineReActLoop::run` 仍是空 TODO，当前无任何代码真正调用 `generate`。

**Tech Stack:** C++17、CMake、MSVC（VS17 生成器）、Qt 5.14.2（core 不依赖）、vendored nlohmann/json；测试用自带 `core/tests/TestHarness.h`（`QH_TEST`/`QH_CHECK`/`QH_CHECK_EQ`）。

## Global Constraints

- **语言/编译**：C++17；MSVC `/utf-8`（中文注释/字符串正常）；core 为 SHARED DLL，导出宏 `QH_API`，公共类整类 `class QH_API X` 导出。
- **命名**：类成员变量下划线前缀（`_turn`、`_cancelled`）；驼峰命名法用于函数/类名；命名空间 `qh::provider`。
- **包含防御**：`#ifndef/#define/#endif`，宏命名 `QH_<模块>_<文件>_H`（`CancellationToken.h`→`QH_PROVIDER_CANCEL_TOKEN_H`，`MockProvider.h`→`QH_PROVIDER_MOCK_H`）。
- **CMakeLists**：所有 `.h` 与 `.cpp` 都要列入 `core/CMakeLists.txt` 源列表（VS 工程可见）；新增测试 `.cpp` 列入 `qharness_core_tests` 源列表。
- **构建输出**：DLL、exe、tests 统一输出到 `build/out/{Release,Debug}`。
- **测试框架约定**：`QH_TEST(name){...}` 注册静态用例；`QH_CHECK(cond)` 布尔断言；`QH_CHECK_EQ(a,b)` 用 `==` 比较、失败时打印左右值——**枚举（如 `Role`）比较必须用 `QH_CHECK(x==y)`，不可用 `QH_CHECK_EQ`**（枚举无 `operator<<`，否则编译失败）；`size()` 比较用 `QH_CHECK_EQ(x, static_cast<size_t>(N))`。
- **不主动 git**：遵循项目 `CLAUDE.md`「未经用户主动要求，绝对不要执行 git 提交」。**本计划不含 `git commit` 步骤**；每个 Task 以「构建 + 全部测试通过」作为完成 gate，是否提交由用户在 Task 间决定。
- **运行环境坑**：`qharness_core_tests.exe` 必须在 `build/out/<config>/` 运行（依赖同目录 `qharness_core.dll`）。

## File Structure

**新增**：
- `core/provider/CancellationToken.h` / `.cpp` —— 轻量取消/超时令牌（`qh::provider`）。
- `core/provider/MockProvider.h` / `.cpp` —— 假模型，`_turn` 计数复现 Go mock。
- `core/tests/test_CancellationToken.cpp` —— CancellationToken 单元测试。
- `core/tests/test_MockProvider.cpp` —— MockProvider 单元测试。

**修改**：
- `core/provider/Provider.h` —— 加 `GenerateResult` 结构 + `generate` 新签名。
- `core/provider/ProviderOpenAI.h` / `.cpp` —— override 新签名（方法体仍 TODO，返回 `GenerateResult{}`）。
- `core/provider/ProviderClaude.h` / `.cpp` —— 同上。
- `core/tests/skeleton_compile_check.cpp` —— 新签名调用验证 + `MockProvider` 实例化。
- `core/CMakeLists.txt` —— 登记新增源/测试。
- `CLAUDE.md` —— 同步失败约定与新增类型。

---

### Task 1: CancellationToken（取消/超时令牌）

**Files:**
- Create: `core/provider/CancellationToken.h`
- Create: `core/provider/CancellationToken.cpp`
- Create: `core/tests/test_CancellationToken.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::provider::CancellationToken` —— `void cancel()`、`bool isCancelled() const`、`void setDeadline(std::chrono::steady_clock::time_point)`。Task 2 的 `generate` 新签名以 `const CancellationToken&` 为首参。

- [ ] **Step 1: 写失败测试** `core/tests/test_CancellationToken.cpp`

```cpp
#include "TestHarness.h"
#include "provider/CancellationToken.h"
#include <chrono>

QH_TEST(canceltoken_default_not_cancelled) {
    qh::provider::CancellationToken token;
    QH_CHECK(!token.isCancelled());
}

QH_TEST(canceltoken_cancel_marks_cancelled) {
    qh::provider::CancellationToken token;
    token.cancel();
    QH_CHECK(token.isCancelled());
}

QH_TEST(canceltoken_past_deadline_is_cancelled) {
    qh::provider::CancellationToken token;
    token.setDeadline(std::chrono::steady_clock::now() - std::chrono::seconds(1));
    QH_CHECK(token.isCancelled());
}

QH_TEST(canceltoken_future_deadline_not_cancelled) {
    qh::provider::CancellationToken token;
    token.setDeadline(std::chrono::steady_clock::now() + std::chrono::hours(1));
    QH_CHECK(!token.isCancelled());
}
```

- [ ] **Step 2: 改 `core/CMakeLists.txt`，登记新源与测试**

在 `qharness_core` 源列表的 `provider/Provider.h` 行之后插入一行：

```cmake
    provider/Provider.h
    provider/CancellationToken.h   provider/CancellationToken.cpp
    provider/ProviderOpenAI.h   provider/ProviderOpenAI.cpp
```

在 `qharness_core_tests` 源列表的 `tests/test_ToolManager.cpp` 行之后插入：

```cmake
    tests/test_ToolManager.cpp
    tests/test_CancellationToken.cpp
    tests/skeleton_compile_check.cpp
```

- [ ] **Step 3: 重新配置 CMake（CMakeLists 已改）**

Run: `cmake -S . -B build`
Expected: 配置成功（cmake 不校验源文件存在性，仅登记）。

- [ ] **Step 4: 构建测试目标，验证失败（红）**

Run: `cmake --build build --config Release --target qharness_core_tests`
Expected: 编译失败 —— `provider/CancellationToken.h`: No such file or directory（测试 include 了尚不存在的头）。

- [ ] **Step 5: 写实现 `core/provider/CancellationToken.h`**

```cpp
#ifndef QH_PROVIDER_CANCEL_TOKEN_H
#define QH_PROVIDER_CANCEL_TOKEN_H
#include "qh_export.h"
#include <atomic>
#include <chrono>
#include <optional>

namespace qh {
namespace provider {

// 轻量取消/超时令牌，对齐 Go context.Context 的取消与超时核心语义（不含 Value map）
// 纯 C++17、零外部依赖；provider 在耗时循环中协作式查询 isCancelled()
class QH_API CancellationToken {
public:
    CancellationToken() = default;

    // 请求取消（线程安全）
    void cancel();

    // 是否已被取消：显式 cancel() 为真，或已设 deadline 且当前时刻越过之
    bool isCancelled() const;

    // 设置超时截止时刻；不设置则仅响应显式 cancel()
    void setDeadline(std::chrono::steady_clock::time_point deadline);

private:
    std::atomic<bool> _cancelled{false};
    std::optional<std::chrono::steady_clock::time_point> _deadline;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_CANCEL_TOKEN_H
```

- [ ] **Step 6: 写实现 `core/provider/CancellationToken.cpp`**

```cpp
#include "provider/CancellationToken.h"

namespace qh {
namespace provider {

void CancellationToken::cancel() {
    _cancelled.store(true, std::memory_order_release);
}

bool CancellationToken::isCancelled() const {
    if (_cancelled.load(std::memory_order_acquire)) {
        return true;
    }
    if (_deadline.has_value() &&
        std::chrono::steady_clock::now() >= *_deadline) {
        return true;
    }
    return false;
}

void CancellationToken::setDeadline(std::chrono::steady_clock::time_point deadline) {
    _deadline = deadline;
}

} // namespace provider
} // namespace qh
```

- [ ] **Step 7: 构建 + 运行全部测试，验证通过（绿）**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全部用例通过，含 4 个 `canceltoken_*` 用例。

---

### Task 2: generate 接口改造（GenerateResult + 取消首参）

> 重构任务（改抽象类签名），无新增测试，以现有全部测试回归 + `skeleton_compile_check` 新签名调用作为验证。

**Files:**
- Modify: `core/provider/Provider.h`
- Modify: `core/provider/ProviderOpenAI.h` / `.cpp`
- Modify: `core/provider/ProviderClaude.h` / `.cpp`
- Modify: `core/tests/skeleton_compile_check.cpp`

**Interfaces:**
- Consumes: `CancellationToken`（Task 1）。
- Produces: `qh::provider::GenerateResult{ schema::Message message; std::string error }`；`Provider::generate(const CancellationToken&, messages, tools) -> GenerateResult`。Task 3 的 `MockProvider` 继承此签名。

- [ ] **Step 1: 改 `core/provider/Provider.h`（加 include、GenerateResult、新签名）**

用以下内容**整体替换** `Provider.h`：

```cpp
#ifndef QH_PROVIDER_H
#define QH_PROVIDER_H
#include <string>
#include <vector>
#include "schema/Message.h"
#include "provider/CancellationToken.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// generate 的返回结果，对齐 Go 的 (*Message, error)：error 空串表示成功
struct GenerateResult {
    schema::Message message;
    std::string error;   // 空=成功；非空=失败描述（HTTP/解析错误等）
};

// 大模型连接抽象基类
class QH_API Provider {
public:
    virtual ~Provider() = default;
    // 失败时 error 非空（不再抛异常）；provider 应在耗时点协作式检查 cancel.isCancelled()
    virtual GenerateResult generate(
        const CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) = 0;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_H
```

- [ ] **Step 2: 改 `core/provider/ProviderOpenAI.h`（override 新签名）**

将 `generate` 声明块替换为：

```cpp
    GenerateResult generate(
        const CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;
```

- [ ] **Step 3: 改 `core/provider/ProviderOpenAI.cpp`（实现新签名）**

将 `generate` 实现替换为（构造函数不变）：

```cpp
GenerateResult ProviderOpenAI::generate(
    const CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& /*tools*/) {
    // TODO: 基于 httplib 实现 OpenAI 格式请求与响应解析
    return GenerateResult{};
}
```

- [ ] **Step 4: 改 `core/provider/ProviderClaude.h`（override 新签名）**

将 `generate` 声明块替换为：

```cpp
    GenerateResult generate(
        const CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;
```

- [ ] **Step 5: 改 `core/provider/ProviderClaude.cpp`（实现新签名）**

将 `generate` 实现替换为（构造函数不变）：

```cpp
GenerateResult ProviderClaude::generate(
    const CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& /*tools*/) {
    // TODO: 基于 httplib 实现 Claude 格式请求与响应解析
    return GenerateResult{};
}
```

- [ ] **Step 6: 改 `core/tests/skeleton_compile_check.cpp`（验证新签名可调用）**

将 provider 实例化块：

```cpp
    provider::ProviderOpenAI po("key", "https://api.example.com", "gpt-x");
    provider::ProviderClaude pc("key", "https://api.example.com", "claude-x");
    (void)po;
    (void)pc;
```

替换为：

```cpp
    provider::ProviderOpenAI po("key", "https://api.example.com", "gpt-x");
    provider::ProviderClaude pc("key", "https://api.example.com", "claude-x");
    {
        provider::CancellationToken token;
        std::vector<schema::Message> msgs;
        std::vector<schema::ToolDefinition> tools;
        (void)po.generate(token, msgs, tools);
        (void)pc.generate(token, msgs, tools);
    }
```

- [ ] **Step 7: 构建 + 运行全部测试，验证回归不破**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全部用例通过（接口重构不改变现有测试行为；`skeleton_compile_check` 通过即证明新签名可编译链接、可被调用）。

---

### Task 3: MockProvider（忠实移植 Go mockProvider）

**Files:**
- Create: `core/provider/MockProvider.h`
- Create: `core/provider/MockProvider.cpp`
- Create: `core/tests/test_MockProvider.cpp`
- Modify: `core/CMakeLists.txt`
- Modify: `core/tests/skeleton_compile_check.cpp`

**Interfaces:**
- Consumes: `Provider`/`GenerateResult`/`CancellationToken`（Task 1、2）；`schema::Message`/`ToolCall`（字段 `_role`/`_content`/`_toolCalls`/`_id`/`_name`/`_arguments`）。
- Produces: `qh::provider::MockProvider` —— 继承 `Provider`，`_turn` 计数：第 1 轮返回带 `bash` 工具调用的 Assistant 消息，第 2 轮起返回纯文本"任务完成"。

- [ ] **Step 1: 写失败测试** `core/tests/test_MockProvider.cpp`

```cpp
#include "TestHarness.h"
#include "provider/MockProvider.h"
#include "provider/CancellationToken.h"
#include "schema/Message.h"
#include <string>
#include <vector>

QH_TEST(mockprovider_first_turn_returns_bash_toolcall) {
    qh::provider::MockProvider provider;
    qh::provider::CancellationToken token;
    std::vector<qh::schema::Message> messages;
    std::vector<qh::schema::ToolDefinition> tools;

    qh::provider::GenerateResult result = provider.generate(token, messages, tools);

    QH_CHECK(result.error.empty());
    QH_CHECK(result.message._role == qh::schema::Role::Assistant);
    QH_CHECK(result.message._content.find("文件") != std::string::npos);
    QH_CHECK_EQ(result.message._toolCalls.size(), static_cast<size_t>(1));
    QH_CHECK_EQ(result.message._toolCalls[0]._id, std::string("call_123"));
    QH_CHECK_EQ(result.message._toolCalls[0]._name, std::string("bash"));
    QH_CHECK(result.message._toolCalls[0]._arguments.find("ls -la") != std::string::npos);
}

QH_TEST(mockprovider_second_turn_returns_plain_text) {
    qh::provider::MockProvider provider;
    qh::provider::CancellationToken token;
    std::vector<qh::schema::Message> messages;
    std::vector<qh::schema::ToolDefinition> tools;

    (void)provider.generate(token, messages, tools); // 第 1 轮
    qh::provider::GenerateResult result = provider.generate(token, messages, tools); // 第 2 轮

    QH_CHECK(result.error.empty());
    QH_CHECK(result.message._role == qh::schema::Role::Assistant);
    QH_CHECK(result.message._content.find("任务完成") != std::string::npos);
    QH_CHECK(result.message._toolCalls.empty());
}
```

- [ ] **Step 2: 改 `core/CMakeLists.txt`，登记 MockProvider 源与测试**

在 `qharness_core` 源列表的 `provider/ProviderClaude.h   provider/ProviderClaude.cpp` 行之后插入：

```cmake
    provider/ProviderClaude.h   provider/ProviderClaude.cpp
    provider/MockProvider.h     provider/MockProvider.cpp
    tool/Tool.h
```

在 `qharness_core_tests` 源列表的 `tests/test_CancellationToken.cpp` 行之后插入：

```cmake
    tests/test_CancellationToken.cpp
    tests/test_MockProvider.cpp
    tests/skeleton_compile_check.cpp
```

- [ ] **Step 3: 改 `core/tests/skeleton_compile_check.cpp`（加 MockProvider 实例化）**

在文件顶部 include 块的 `#include "provider/ProviderClaude.h"` 之后加：

```cpp
#include "provider/ProviderClaude.h"
#include "provider/MockProvider.h"
```

将 Task 2 写入的 provider 调用块：

```cpp
    {
        provider::CancellationToken token;
        std::vector<schema::Message> msgs;
        std::vector<schema::ToolDefinition> tools;
        (void)po.generate(token, msgs, tools);
        (void)pc.generate(token, msgs, tools);
    }
```

替换为：

```cpp
    {
        provider::CancellationToken token;
        std::vector<schema::Message> msgs;
        std::vector<schema::ToolDefinition> tools;
        (void)po.generate(token, msgs, tools);
        (void)pc.generate(token, msgs, tools);
        provider::MockProvider mp;
        (void)mp.generate(token, msgs, tools);
    }
```

- [ ] **Step 4: 重新配置 CMake**

Run: `cmake -S . -B build`
Expected: 配置成功。

- [ ] **Step 5: 构建测试目标，验证失败（红）**

Run: `cmake --build build --config Release --target qharness_core_tests`
Expected: 编译失败 —— `provider/MockProvider.h`: No such file or directory。

- [ ] **Step 6: 写实现 `core/provider/MockProvider.h`**

```cpp
#ifndef QH_PROVIDER_MOCK_H
#define QH_PROVIDER_MOCK_H
#include "provider/Provider.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// 模拟大模型，用于测试引擎 ReAct 循环（参考 Go mockProvider）
// 行为：第1轮返回带 bash 工具调用的 Assistant 消息；第2轮起返回纯文本"任务完成"
class QH_API MockProvider : public Provider {
public:
    GenerateResult generate(
        const CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

private:
    int _turn{0};
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_MOCK_H
```

- [ ] **Step 7: 写实现 `core/provider/MockProvider.cpp`**

```cpp
#include "provider/MockProvider.h"

namespace qh {
namespace provider {

GenerateResult MockProvider::generate(
    const CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& /*tools*/) {
    ++_turn;
    GenerateResult result;
    result.message._role = schema::Role::Assistant;

    if (_turn == 1) {
        result.message._content = "让我来看看当前目录下有什么文件。";
        schema::ToolCall call;
        call._id = "call_123";
        call._name = "bash";
        call._arguments = "{\"command\": \"ls -la\"}";
        result.message._toolCalls.push_back(std::move(call));
    } else {
        result.message._content = "我看到了文件列表，里面包含 main.go，任务完成！";
    }
    return result;
}

} // namespace provider
} // namespace qh
```

- [ ] **Step 8: 构建 + 运行全部测试，验证通过（绿）**

Run: `cmake --build build --config Release --target qharness_core_tests && build/out/Release/qharness_core_tests.exe`
Expected: 全部用例通过，含 2 个 `mockprovider_*` 用例。

---

### Task 4: 同步 CLAUDE.md

> 纯文档，无构建。

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: 更新 provider 派生类清单**

将：

```
`provider/{Provider,ProviderOpenAI,ProviderClaude}`
```

替换为：

```
`provider/{Provider,ProviderOpenAI,ProviderClaude,MockProvider}`
```

- [ ] **Step 2: 更新 generate 失败约定段**

将：

```
`Provider::generate` 失败约定抛 `std::runtime_error`。
```

替换为：

```
`Provider::generate` 新签名为 `generate(const CancellationToken&, messages, tools) -> GenerateResult`，失败约定 `GenerateResult::error` 非空（不再抛异常）；`CancellationToken`（`qh::provider`，轻量取消/超时，对齐 Go `context.Context` 核心语义）作为首参。`MockProvider` 忠实移植 Go `mockProvider`（`_turn` 计数：第 1 轮返回 `bash` 工具调用、第 2 轮起纯文本"任务完成"），用于引擎循环测试。
```

- [ ] **Step 3: 验证文档一致性**

人工确认 `CLAUDE.md` 中 provider 相关描述与新接口一致（无残留"抛异常"表述）。

---

## Self-Review

**1. Spec coverage**（对照设计文档 A–G）：
- 1.1 CancellationToken → Task 1 ✓
- 1.2 GenerateResult、1.3 Provider 新签名、1.4 派生类同步 → Task 2 ✓
- 2 MockProvider → Task 3 ✓
- 3 数据流/错误处理 → 由 Task 2 新签名 + Task 1 cancel 语义实现 ✓
- 4 CMakeLists 登记 → Task 1/3 各自 Step 2 ✓
- 5 测试（test_MockProvider + CancellationToken 自测）→ Task 1/3 ✓
- 6 skeleton_compile_check → Task 2 Step 6 + Task 3 Step 3 ✓
- 7 文档同步 → Task 4 ✓
- 范围边界（不做 mockRegistry/EngineReActLoop::run/集成测试）→ 计划未含，符合 YAGNI ✓

**2. Placeholder scan**：无 TBD/TODO（`ProviderOpenAI/Claude` 方法体的 `// TODO` 是保留的骨架占位，非计划占位）；每个代码步骤均含完整代码 ✓

**3. Type consistency**：
- `CancellationToken` 全程 `qh::provider::CancellationToken`，方法 `cancel()`/`isCancelled()`/`setDeadline()` 一致 ✓
- `GenerateResult` 字段 `message`/`error` 在 Provider.h、测试、MockProvider 中一致 ✓
- `generate` 签名四文件（Provider/OpenAI/Claude/Mock）参数顺序与类型一致 ✓
- schema 字段 `_role`/`_content`/`_toolCalls`/`_id`/`_name`/`_arguments` 与 `Message.h` 一致 ✓
- Role 比较统一用 `QH_CHECK(x==y)`（非 `QH_CHECK_EQ`）✓
