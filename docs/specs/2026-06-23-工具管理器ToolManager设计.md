# 工具管理器（ToolRegistry / ToolManager）设计

> 日期：2026-06-23
> 模块：`core/tool`
> 对应 Go 参考：`go-tiny-claw/internal/tools/registry.go`、`internal/engine/loop.go`

## 1. 背景与目标

Go 参考项目中，`tools.Registry` 是一个**接口**，仅定义两个方法：

- `GetAvailableTools() []schema.ToolDefinition`：返回当前挂载的全部工具定义，供 provider 在每轮推理时感知可用工具。
- `Execute(ctx, call schema.ToolCall) schema.ToolResult`：按 `call.Name` 路由到具体工具执行，返回结果。

`engine/loop.go` 的 ReAct 循环每轮调用 `GetAvailableTools`，对模型返回的每个 `ToolCall` 调用 `Execute`，并把结果（Observation）回填进上下文。**Go 版没有提供 `Registry` 的真实实现**（`cmd/claw/main.go` 用 `mockRegistry` 凑合）。

C++ 现状（`QHarnesscc`）：

- 已有 `core/tool/Tool.h` 抽象基类：`virtual ToolDefinition definition() const = 0;` + `virtual ToolResult execute(const ToolCall& call) = 0;`。
- schema 层（`ToolDefinition` / `ToolCall` / `ToolResult`）已完备。
- **缺少「持有多个 Tool、按名注册/查找/分发」的具体实现**。

**本次目标**：在 `core/tool` 增加工具管理器，补上 Go `Registry` 语义的 C++ 具体实现，使 engine 能在 ReAct 循环中列出工具并分发执行。本次**只做管理器本身**，不实现具体工具（bash/edit/read 等留待后续）。

## 2. 核心决策（已与用户确认）

| 决策点 | 选择 | 理由 |
|---|---|---|
| 范围 | 抽象 `ToolRegistry` 接口 + 具体 `ToolManager` 实现 | 既保留抽象骨架（engine 依赖抽象），又给出真实可用实现 |
| 工具所有权 | **非拥有**：管理器持 `Tool*`，只路由，不管理生命周期 | 与 Go 版引用语义一致；工具按长生命周期单例管理，由调用方保证存活 |
| 找不到工具的语义 | 返回 `isError=true` 的 `ToolResult` | 与 Go 契约、schema 的 `isError`「错误自愈」设计一致；结果可喂回模型让其自愈，不中断 ReAct 循环 |
| 接口分层 | 方案 A：`ToolRegistry` 仅 Go 契约两方法，管理 API 仅在 `ToolManager` | engine 只需「列出+执行」，依赖最小接口（ISP 接口隔离 + DIP 依赖倒置） |

## 3. 架构与分层

| 文件 | 职责 | 形态 | 命名空间 |
|---|---|---|---|
| `core/tool/ToolRegistry.h` | 抽象接口，engine 依赖它 | header-only，纯虚 | `qh::tool` |
| `core/tool/ToolManager.h` | 具体管理类声明 | 头文件 | `qh::tool` |
| `core/tool/ToolManager.cpp` | 具体管理类实现 | 源文件 | `qh::tool` |

`ToolManager` 以 `public ToolRegistry` 继承抽象接口并实现其契约，额外扩展注册 / 注销 / 查找等管理 API。

include guard（遵循项目约定 `QH_<模块>_<文件>_H`）：`QH_TOOL_TOOLREGISTRY_H`、`QH_TOOL_TOOLMANAGER_H`，使用 `#ifndef/#define/#endif`。

## 4. 接口定义

### 4.1 ToolRegistry（抽象接口，对应 Go 的 `tools.Registry`）

```cpp
#ifndef QH_TOOL_TOOLREGISTRY_H
#define QH_TOOL_TOOLREGISTRY_H
#include <vector>
#include "schema/Message.h"

namespace qh {
namespace tool {

// 工具注册与分发执行的抽象接口（engine 依赖此抽象，而非具体 ToolManager）
class ToolRegistry {
public:
    virtual ~ToolRegistry() = default;

    // 返回当前挂载的全部工具定义（对应 Go GetAvailableTools）
    virtual std::vector<schema::ToolDefinition> getAvailableTools() const = 0;

    // 按 call.name 路由并执行工具（对应 Go Execute）
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_TOOLREGISTRY_H
```

### 4.2 ToolManager（具体实现）

```cpp
#ifndef QH_TOOL_TOOLMANAGER_H
#define QH_TOOL_TOOLMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>
#include "schema/Message.h"
#include "tool/Tool.h"
#include "tool/ToolRegistry.h"

namespace qh {
namespace tool {

// 工具管理器：持有多个工具的非拥有引用，负责注册/查找/分发执行
class ToolManager : public ToolRegistry {
public:
    // 注册工具：以 tool.definition().name 为键；重名返回 false（不覆盖、不抛错）
    bool registerTool(Tool& tool);

    // 注销工具：按名移除；不存在返回 false
    bool unregisterTool(const std::string& name);

    // 按名查找工具；找不到返回 nullptr
    Tool* getTool(const std::string& name) const;

    // 是否存在同名工具
    bool hasTool(const std::string& name) const;

    // 实现 ToolRegistry 契约
    std::vector<schema::ToolDefinition> getAvailableTools() const override;
    schema::ToolResult execute(const schema::ToolCall& call) override;

private:
    std::unordered_map<std::string, Tool*> tools_;  // 非拥有：调用方保证工具生命周期
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_TOOLMANAGER_H
```

## 5. 行为语义

- **注册取名**：`registerTool` 自动以 `tool.definition().name` 作为键，与 `execute` 按 `call.name` 路由一致，调用方无需重复传名。
- **重名注册**：返回 `false`，不覆盖、不抛异常（调用方显式感知，避免静默替换导致难以排查的行为变化）。
- **注销**：`unregisterTool` 按名移除，不存在返回 `false`。
- **查找**：`getTool` 返回 `Tool*`，未命中返回 `nullptr`；`hasTool` 为存在性判断。
- **execute 分发**：按 `call.name` 在 `tools_` 查找；命中则调用 `tool->execute(call)` 返回其结果；未命中返回错误结果（见下）。

## 6. 数据流（execute 分发）

```
engine ──call──▶ toolManager.execute(call)
                   │ 按 call.name 在 tools_ 查找
                   ├─ 命中 ─▶ tool->execute(call) ─▶ ToolResult（由具体工具决定成功/失败）
                   └─ 未命中 ─▶ ToolResult{
                                   toolCallId = call.id,
                                   output     = "未找到工具: <name>",
                                   isError    = true
                                }
```

## 7. 错误处理

- **找不到工具**：`execute` 返回 `isError=true` 的 `ToolResult`，`toolCallId` 回填为 `call.id`，`output` 写明缺失的工具名。engine 已有 `isError` 分支，会把该结果作为 Observation 喂回模型，触发错误自愈，**不中断 ReAct 循环、不抛异常**。
- **具体工具内部失败**：由各 `Tool::execute` 自行决定，通常也通过返回 `isError=true` 的 `ToolResult` 表达（与 schema 设计一致），ToolManager 透传，不做额外捕获。
- **不入参校验抛异常**：管理器层不抛 `std::runtime_error`（与 Provider 失败风格区分——Provider 是 IO 失败必须抛；工具管理器是可恢复的路由问题，用 `isError` 表达）。

## 8. 测试计划

新增 `core/tests/test_ToolManager.cpp`，使用项目自带框架（`QH_TEST` / `QH_CHECK` / `QH_CHECK_EQ`）。以一个实现 `Tool` 接口的 fake 工具（`definition()` 返回固定 `ToolDefinition`，`execute()` 回填固定 `ToolResult`）为桩，覆盖：

1. **注册与查找**：`registerTool` 后 `hasTool` 为真；`getTool` 返回非空且 `definition().name` 正确。
2. **重名保护**：对同名工具再次 `registerTool` 返回 `false`，且原工具未被替换。
3. **注销**：`unregisterTool` 后 `hasTool` 为假、`getTool` 返回 `nullptr`；注销不存在的名返回 `false`。
4. **列举定义**：`getAvailableTools` 数量与已注册工具数一致，且包含各工具的 `definition`。
5. **execute 分发**：对已注册工具的 `call`，结果来自对应工具（`toolCallId` 正确回填、内容匹配）。
6. **execute 找不到**：未注册名的 `call`，返回 `isError==true`、`toolCallId==call.id`、`output` 含工具名。

测试文件登记到 `core/CMakeLists.txt` 的 `qharness_core_tests` 源列表。

> 注：`TestMain.cpp` 会运行全部注册用例，暂无法隔离单跑；调试时可临时注释其它用例（项目既有约定）。

## 9. 构建集成（CMake）

在 `core/CMakeLists.txt`：

- `qharness_core` 静态库源列表新增（沿用现有「.h 与 .cpp 同行列出」格式）：
  ```
  tool/ToolRegistry.h
  tool/ToolManager.h      tool/ToolManager.cpp
  ```
  （紧随现有 `tool/Tool.h` 之后）
- `qharness_core_tests` 测试源列表新增：
  ```
  tests/test_ToolManager.cpp
  ```

`core` 模块仍**不依赖 Qt**，仅用 vendored 的 nlohmann/json（ToolManager 实现仅用 STL 容器与 schema 类型，连 json 都不直接需要）。

## 10. 约定与坑点

- **非线程安全**：单线程 ReAct 循环内使用（与 Go 版 `loop.go` 顺序执行 `toolCalls` 一致），不加锁（YAGNI）。如未来 engine 并发执行工具，需另行评估加锁。
- **非拥有生命周期**：`ToolManager` 析构**不**析构工具；调用方（后续 main / Robot 装配处）必须保证工具生命周期长于管理器，否则 `tools_` 内指针悬空。
- **列举顺序不保证**：`getAvailableTools` 基于 `std::unordered_map` 遍历，返回的工具定义顺序与注册顺序、字母序均无关。功能上模型不依赖顺序，无影响；但测试断言需用**集合语义**比较（数量相等 + 逐项包含），而非有序逐元素比较。若后续需要稳定顺序，可改用 `std::map` 或附加 `std::vector` 记录插入顺序。
- **头文件入 CMakeLists**：`ToolRegistry.h`、`ToolManager.h` 必须列入 `qharness_core` 源列表，使其在 VS 工程中可见（项目约定）。
- **include 路径**：`core/` 是 `qharness_core` 的公共 include 根，内部头用根相对路径引用（`#include "tool/Tool.h"`、`#include "tool/ToolRegistry.h"`、`#include "schema/Message.h"`）。
- **源码 UTF-8**：CMake 已对 core 配 `/utf-8`，中文注释/字符串正常。

## 11. 非目标（YAGNI，本次不做）

- 不实现具体工具（bash / edit / read 等）。
- 不引入 `context` / 取消 / 超时机制（Go 的 `context.Context` 等价物）；`execute` 签名保持只接 `ToolCall`，与现有 `Tool::execute` 一致。
- 不做线程安全 / 并发执行。
- 不做工具分组、权限、启用/禁用等高级管理特性。
- 不做工具动态加载 / 插件化。

## 12. 后续衔接（不在本次范围）

- engine（`EngineReActLoop`）将持有 `ToolRegistry*`（或引用），在 ReAct 循环中调用 `getAvailableTools` 与 `execute`——届时 engine 依赖 `ToolRegistry` 抽象。
- 装配处（main / Robot）创建具体 `Tool` 实例并 `registerTool` 进 `ToolManager`，再把 `ToolManager` 交给 engine。
