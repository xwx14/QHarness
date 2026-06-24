# MockProvider 新建 + generate 接口对齐 Go 设计

- 日期：2026-06-24
- 状态：设计已确认，待实现
- 参考实现：`E:\MyHarness\go-tiny-claw\cmd\claw\main.go`（mockProvider）、`E:\MyHarness\go-tiny-claw\internal\provider\interface.go`（LLMProvider）

## 背景

QHarnesscc 正把 Go 项目 `go-tiny-claw` 的 Provider 架构移植到 C++17。当前 C++ `Provider::generate` 签名为 `generate(messages, tools) -> Message`，失败抛 `std::runtime_error`、无取消语义；与 Go `Generate(ctx context.Context, messages, tools) (*Message, error)` 存在两点差异：

1. **错误处理**：C++ 用异常，Go 用返回值 `(*Message, error)`。
2. **取消/超时**：C++ 无，Go 由 `context.Context` 承载。

本次目标：

1. 将 `generate` 接口**完整对齐 Go**（错误返回值 + 取消/超时参数）。
2. 新增 `MockProvider`，忠实移植 Go `mockProvider` 行为，为后续引擎 ReAct 循环测试提供可用的假模型。

**关键时机判断**：`EngineReActLoop::run` 仍是空 `TODO`，**当前无任何代码真正调用 `generate`**——改接口零破坏，是最佳时机。

## 设计决策

| 决策点 | 选择 | 理由 |
|---|---|---|
| 对齐范围 | 完整对齐 Go（错误返回值 + 取消/超时） | 项目目标即忠实移植 Go；`run` 未实现，零破坏 |
| ctx 的 C++ 表达 | 轻量 `CancellationToken`（取消 + 可选 deadline） | 覆盖取消/超时核心语义；不抄 Go `ctx.Value`（YAGNI） |
| 错误返回形态 | `GenerateResult{ Message; std::string error }` | 结构整洁，`error` 空串=成功，贴近 Go `(*Message,error)` |
| 新类型命名空间 | `qh::provider` | 避免与现有 `qh::context::Composer`（消息上下文组装器）混淆 |

## 1. generate 接口改造

### 1.1 CancellationToken（新增）

文件：`core/provider/CancellationToken.h` + `core/provider/CancellationToken.cpp`。

轻量取消/超时令牌，对齐 Go `context.Context` 的「取消 + 超时」核心语义，**不含** Value map。纯 C++17、零外部依赖。provider 在耗时循环中协作式查询 `isCancelled()`。

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
class QH_API CancellationToken {
public:
    CancellationToken() = default;

    // 请求取消（线程安全）
    void cancel();

    // 是否已被取消（显式 cancel() 或 deadline 到期）
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

`isCancelled()` 实现：返回 `_cancelled` 为真**或** `_deadline` 已设且当前时刻越过之。

### 1.2 GenerateResult（并入 Provider.h）

置于 `Provider.h`，`Provider` 类之前。

```cpp
// generate 的返回结果，对齐 Go 的 (*Message, error)：error 空串表示成功
struct GenerateResult {
    schema::Message message;
    std::string error;   // 空=成功；非空=失败描述（HTTP/解析错误等）
};
```

### 1.3 Provider 新签名（Provider.h）

```cpp
class QH_API Provider {
public:
    virtual ~Provider() = default;
    // 失败时 error 非空（不再抛异常）；provider 应在耗时点协作式检查 cancel.isCancelled()
    virtual GenerateResult generate(
        const CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) = 0;
};
```

失败约定由「抛 `std::runtime_error`」改为「`error` 非空」。

### 1.4 派生类同步

`ProviderOpenAI`、`ProviderClaude` 的 `.h` override 声明与 `.cpp` 实现签名一并更新（方法体仍 `TODO` 占位，返回 `GenerateResult{}`）。

## 2. MockProvider（新增）

文件：`core/provider/MockProvider.h` + `core/provider/MockProvider.cpp`。忠实移植 Go `mockProvider`。

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

实现语义（`generate` 忽略 `cancel`/`messages`/`tools` 参数，`++_turn`）：

- `_turn == 1`：`_role=Assistant`、`_content="让我来看看当前目录下有什么文件。"`、`_toolCalls={ {_id="call_123", _name="bash", _arguments="{\"command\": \"ls -la\"}"} }`
- `_turn >= 2`：`_role=Assistant`、`_content="我看到了文件列表，里面包含 main.go，任务完成！"`、`_toolCalls={}`
- `error` 恒为空串（成功）

> 字段命名遵循 schema 既有约定：`ToolCall::_id/_name/_arguments`、`Message::_role/_content/_toolCalls`（均下划线前缀、public）。

## 3. 数据流 / 错误处理

```
Engine → provider.generate(cancel, messages, tools) → GenerateResult
```

- 成功：`error` 空，`message` 供引擎处理（含 `_toolCalls` 则执行工具、回填 `ToolResult`）。
- 失败：`error` 非空，引擎据以终止或重试。
- 取消：调用方置 `cancel.cancel()`，provider 检测后返回 `error="cancelled"`。

## 4. CMakeLists 登记

`core/CMakeLists.txt`：

- `qharness_core` 源列表新增 `provider/CancellationToken.h`/`.cpp`、`provider/MockProvider.h`/`.cpp`。
- `qharness_core_tests` 源列表新增 `tests/test_MockProvider.cpp`。

## 5. 测试（TDD）

`core/tests/test_MockProvider.cpp`，使用 `QH_TEST`/`QH_CHECK`/`QH_CHECK_EQ`：

- **第1次 generate**：`_role==Assistant`、`_content` 含预期文案、`_toolCalls.size()==1` 且 `_id=="call_123"`、`_name=="bash"`、`_arguments` 含 `ls -la`、`error` 空。
- **第2次 generate**（同一实例）：`_role==Assistant`、`_content=="我看到了文件列表，里面包含 main.go，任务完成！"`、`_toolCalls.empty()`、`error` 空。
- **CancellationToken 自测**：`cancel()` 后 `isCancelled()==true`；新建未取消、未设 deadline 时 `isCancelled()==false`。

## 6. skeleton_compile_check.cpp

新增 `MockProvider` 实例化 + 一次 `generate` 调用（验证新签名可编译、可链接）。

## 7. 文档同步

更新 `CLAUDE.md`：

- `Provider::generate` 失败约定由「抛 `std::runtime_error`」改为「`error` 非空」。
- 记录新增类型 `CancellationToken`、`GenerateResult` 与 `MockProvider`。

## 范围边界（YAGNI）

本次**不做**：

- `mockRegistry`（Go `main.go` 中存在，C++ 尚无）。
- `EngineReActLoop::run` 实现。
- 引擎 ReAct 循环集成测试——依赖 `run` 实现，待后续步骤；届时参考 Go `main.go` 的 `mockRegistry + engine.Run` 模式。

## 受影响文件清单

**新增**：

- `core/provider/CancellationToken.h` / `.cpp`
- `core/provider/MockProvider.h` / `.cpp`
- `core/tests/test_MockProvider.cpp`

**修改**：

- `core/provider/Provider.h`（新签名 + GenerateResult）
- `core/provider/ProviderOpenAI.h` / `.cpp`
- `core/provider/ProviderClaude.h` / `.cpp`
- `core/tests/skeleton_compile_check.cpp`
- `core/CMakeLists.txt`
- `CLAUDE.md`
