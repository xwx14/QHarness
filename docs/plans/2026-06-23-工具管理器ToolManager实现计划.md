# 工具管理器（ToolRegistry / ToolManager）实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 `core/tool` 实现工具管理器：抽象接口 `ToolRegistry`（engine 依赖）+ 具体类 `ToolManager`（非拥有持有 `Tool*`，按名注册/查找/注销/列举/分发执行），并用 TDD 覆盖全部行为。

**Architecture:** `ToolManager` 以 `public ToolRegistry` 继承抽象接口，内部用 `std::unordered_map<std::string, Tool*>` 存储工具的非拥有指针；`execute` 按 `call.name` 路由，命中则委托 `tool->execute(call)`，未命中返回 `isError=true` 的 `ToolResult`（与 schema「错误自愈」语义一致，不抛异常）。engine 将只依赖 `ToolRegistry` 两方法（依赖倒置）。

**Tech Stack:** C++17、STL（`unordered_map`/`vector`/`string`）、vendored nlohmann/json（仅测试桩的 `inputSchema` 用 `json::object()`）、项目自带 TestHarness（`QH_TEST`/`QH_CHECK`/`QH_CHECK_EQ`）。

## Global Constraints

（取自设计文档 `docs/specs/2026-06-23-工具管理器ToolManager设计.md` 与项目 CLAUDE.md，每个任务默认遵守）

- C++17（`CMAKE_CXX_STANDARD 17`，`/utf-8` 编译，MSVC + VS 生成器）。
- `core` 模块**不依赖 Qt**；`ToolManager` 实现仅用 STL，不直接用 nlohmann/json。
- include guard 用 `#ifndef/#define/#endif`（不用 `pragma once`），命名 `QH_TOOL_TOOLREGISTRY_H`、`QH_TOOL_TOOLMANAGER_H`。
- 命名空间 `qh::tool`；内部头用根相对路径 include（`core/` 是 `qharness_core` 的 include 根），如 `#include "tool/Tool.h"`、`#include "schema/Message.h"`。
- 所有新增 `.h` 列入 `core/CMakeLists.txt` 的 `qharness_core` 源列表（VS 工程可见）。
- 变量/函数驼峰命名；源码注释中文。
- 测试用 `QH_TEST(name){...}` 静态注册，`TestMain.cpp` 运行全部用例（无法隔离单跑）。
- git 提交信息用中文，提交前需用户确认（项目约定：未经要求不执行 git 操作）。

## File Structure

| 文件 | 动作 | 职责 |
|---|---|---|
| `core/tool/ToolRegistry.h` | 新建 | 抽象接口（header-only，纯虚 `getAvailableTools`/`execute`） |
| `core/tool/ToolManager.h` | 新建 | 具体类声明，继承 `ToolRegistry` |
| `core/tool/ToolManager.cpp` | 新建 | 具体类实现 |
| `core/tests/test_ToolManager.cpp` | 新建 | 单元测试（含 fake tool 桩） |
| `core/CMakeLists.txt` | 修改 | 登记新头/源到 `qharness_core`，测试到 `qharness_core_tests` |

构建/测试命令（仓库根 `E:/MyHarness/QHarnesscc`，bash）：

```bash
# CMakeLists 改动后重新配置（复用已缓存生成器与 Qt 路径）
cmake -S . -B build
# 构建测试目标
cmake --build build --config Release --target qharness_core_tests
# 运行（core_tests 不依赖 Qt，可直接运行）
build/core/Release/qharness_core_tests.exe
```

---

## Task 1: 抽象接口 + ToolManager 核心（注册/查找/存在/列举/分发）

**Files:**
- Create: `core/tool/ToolRegistry.h`
- Create: `core/tool/ToolManager.h`
- Create: `core/tool/ToolManager.cpp`
- Create: `core/tests/test_ToolManager.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Consumes: `qh::tool::Tool`（`definition()`/`execute(call)`，来自 `tool/Tool.h`）；`qh::schema::{ToolDefinition, ToolCall, ToolResult}`（来自 `schema/Message.h`）；`nlohmann::json`（来自 `third/`，仅测试用）。
- Produces:
  - `qh::tool::ToolRegistry`（抽象）：`virtual std::vector<schema::ToolDefinition> getAvailableTools() const = 0;`、`virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;`
  - `qh::tool::ToolManager : public ToolRegistry`：`bool registerTool(Tool& tool);`、`Tool* getTool(const std::string& name) const;`、`bool hasTool(const std::string& name) const;`、`getAvailableTools()`/`execute()` override。

- [ ] **Step 1: 创建抽象接口 `core/tool/ToolRegistry.h`**

```cpp
#ifndef QH_TOOL_TOOLREGISTRY_H
#define QH_TOOL_TOOLREGISTRY_H
#include <vector>
#include "schema/Message.h"

namespace qh {
namespace tool {

// 工具注册与分发执行的抽象接口（对应 Go 的 tools.Registry）
// engine 依赖此抽象而非具体 ToolManager（依赖倒置）
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

- [ ] **Step 2: 创建 `core/tool/ToolManager.h`（核心声明，本任务不含 `unregisterTool`）**

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

- [ ] **Step 3: 创建 `core/tool/ToolManager.cpp`（最小桩，使工程可编译；测试将失败 = RED）**

```cpp
#include "tool/ToolManager.h"

namespace qh {
namespace tool {

// 桩实现：刻意返回失败/空值，驱动测试进入 RED 状态
bool ToolManager::registerTool(Tool& /*tool*/) { return false; }
Tool* ToolManager::getTool(const std::string& /*name*/) const { return nullptr; }
bool ToolManager::hasTool(const std::string& /*name*/) const { return false; }
std::vector<schema::ToolDefinition> ToolManager::getAvailableTools() const { return {}; }
schema::ToolResult ToolManager::execute(const schema::ToolCall& call) {
    schema::ToolResult r;
    r.toolCallId = call.id;
    r.isError = true;
    return r;
}

} // namespace tool
} // namespace qh
```

- [ ] **Step 4: 修改 `core/CMakeLists.txt`，登记新文件**

在 `qharness_core` 源列表的 `tool/Tool.h` 之后新增 `ToolRegistry.h` 与 `ToolManager.h/.cpp`：

```cmake
    tool/Tool.h
    tool/ToolRegistry.h
    tool/ToolManager.h        tool/ToolManager.cpp
    robot/Robot.h
```

在 `qharness_core_tests` 源列表的 `tests/test_Message.cpp` 之后新增测试文件：

```cmake
    tests/test_Message.cpp
    tests/test_ToolManager.cpp
    tests/skeleton_compile_check.cpp
```

- [ ] **Step 5: 创建 `core/tests/test_ToolManager.cpp`（Task 1 测试用例）**

```cpp
#include "TestHarness.h"
#include "tool/Tool.h"
#include "tool/ToolManager.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>
#include <string>

using nlohmann::json;

namespace {

// 测试用桩工具：definition() 返回固定定义，execute() 回填固定输出
class FakeTool : public qh::tool::Tool {
public:
    FakeTool(std::string name, std::string output = "ok")
        : name_(std::move(name)), output_(std::move(output)) {}

    qh::schema::ToolDefinition definition() const override {
        qh::schema::ToolDefinition d;
        d.name = name_;
        d.description = "fake tool for test";
        d.inputSchema = json::object();
        return d;
    }

    qh::schema::ToolResult execute(const qh::schema::ToolCall& call) override {
        qh::schema::ToolResult r;
        r.toolCallId = call.id;
        r.output = output_;
        r.isError = false;
        return r;
    }

private:
    std::string name_;
    std::string output_;
};

// 构造一个带 id/name 的 ToolCall，参数给空 JSON 对象
qh::schema::ToolCall makeCall(const std::string& id, const std::string& name) {
    qh::schema::ToolCall c;
    c.id = id;
    c.name = name;
    c.arguments = "{}";
    return c;
}

} // namespace

QH_TEST(toolmanager_register_and_lookup) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");

    QH_CHECK(tm.registerTool(bash));
    QH_CHECK(tm.hasTool("bash"));
    QH_CHECK(tm.getTool("bash") != nullptr);
    QH_CHECK_EQ(std::string(tm.getTool("bash")->definition().name), std::string("bash"));
}

QH_TEST(toolmanager_register_duplicate_not_replaced) {
    qh::tool::ToolManager tm;
    FakeTool a("dup", "a-out");
    FakeTool b("dup", "b-out");

    QH_CHECK(tm.registerTool(a));
    QH_CHECK(!tm.registerTool(b));  // 重名返回 false，不覆盖

    // 执行应走 a（未被 b 替换）
    auto r = tm.execute(makeCall("c1", "dup"));
    QH_CHECK_EQ(r.output, std::string("a-out"));
}

QH_TEST(toolmanager_list_definitions) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");
    FakeTool edit("edit");
    tm.registerTool(bash);
    tm.registerTool(edit);

    auto defs = tm.getAvailableTools();
    QH_CHECK_EQ(defs.size(), static_cast<size_t>(2));

    // 集合语义：包含 bash 和 edit（unordered_map 顺序不保证）
    bool hasBash = false, hasEdit = false;
    for (const auto& d : defs) {
        if (d.name == "bash") hasBash = true;
        if (d.name == "edit") hasEdit = true;
    }
    QH_CHECK(hasBash);
    QH_CHECK(hasEdit);
}

QH_TEST(toolmanager_execute_dispatches_to_tool) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash", "ls-output");
    tm.registerTool(bash);

    auto r = tm.execute(makeCall("call_1", "bash"));
    QH_CHECK_EQ(r.toolCallId, std::string("call_1"));
    QH_CHECK_EQ(r.output, std::string("ls-output"));
    QH_CHECK(!r.isError);
}

QH_TEST(toolmanager_execute_unknown_returns_error) {
    qh::tool::ToolManager tm;
    auto r = tm.execute(makeCall("call_2", "nope"));

    QH_CHECK(r.isError);
    QH_CHECK_EQ(r.toolCallId, std::string("call_2"));
    QH_CHECK(r.output.find("nope") != std::string::npos);  // 错误信息含工具名
}

```

- [ ] **Step 6: 重新配置并构建测试目标**

Run:
```bash
cmake -S . -B build && cmake --build build --config Release --target qharness_core_tests
```
Expected: 构建成功（无编译/链接错误）。

- [ ] **Step 7: 运行测试，确认 RED**

Run:
```bash
build/core/Release/qharness_core_tests.exe; echo "exit=$?"
```
Expected: 输出含 `FAIL:` 行（如 `FAIL: tm.registerTool(bash)`、`FAIL: defs.size() ... != 2`、`FAIL: r.output ... != "ls-output"` 等，因桩返回 false/nullptr/空），末行形如 `N checks, M failures`（`M > 0`），`exit=1`。
> 说明：`toolmanager_execute_unknown_returns_error` 在桩阶段即通过（桩 `execute` 恰返回 isError），属正常——它不阻塞 RED。

- [ ] **Step 8: 用真实实现替换 `core/tool/ToolManager.cpp`（GREEN）**

```cpp
#include "tool/ToolManager.h"

namespace qh {
namespace tool {

bool ToolManager::registerTool(Tool& tool) {
    const std::string name = tool.definition().name;
    if (tools_.count(name) > 0) {
        return false;  // 重名：不覆盖
    }
    tools_.emplace(name, &tool);
    return true;
}

Tool* ToolManager::getTool(const std::string& name) const {
    auto it = tools_.find(name);
    return it == tools_.end() ? nullptr : it->second;
}

bool ToolManager::hasTool(const std::string& name) const {
    return tools_.count(name) > 0;
}

std::vector<schema::ToolDefinition> ToolManager::getAvailableTools() const {
    std::vector<schema::ToolDefinition> defs;
    defs.reserve(tools_.size());
    for (const auto& kv : tools_) {
        defs.push_back(kv.second->definition());
    }
    return defs;
}

schema::ToolResult ToolManager::execute(const schema::ToolCall& call) {
    Tool* tool = getTool(call.name);
    if (tool == nullptr) {
        schema::ToolResult result;
        result.toolCallId = call.id;
        result.output = "未找到工具: " + call.name;
        result.isError = true;
        return result;
    }
    return tool->execute(call);
}

} // namespace tool
} // namespace qh
```

- [ ] **Step 9: 重新构建测试目标**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests
```
Expected: 构建成功。

- [ ] **Step 10: 运行测试，确认 GREEN**

Run:
```bash
build/core/Release/qharness_core_tests.exe; echo "exit=$?"
```
Expected: 无 `FAIL:` 行；末行 `N checks, 0 failures`；`exit=0`。（`N` = 既有 schema/skeleton 检查数 + 本任务新增的 5 个用例的全部断言数。）

- [ ] **Step 11: 提交（需用户确认后执行）**

```bash
git add core/tool/ToolRegistry.h core/tool/ToolManager.h core/tool/ToolManager.cpp core/tests/test_ToolManager.cpp core/CMakeLists.txt
git commit -m "实现工具管理器核心(ToolRegistry抽象接口+ToolManager注册/查找/列举/分发,含单元测试)"
```

---

## Task 2: 注销接口 `unregisterTool`

**Files:**
- Modify: `core/tool/ToolManager.h`（public 区新增声明）
- Modify: `core/tool/ToolManager.cpp`（桩 → 真实实现）
- Modify: `core/tests/test_ToolManager.cpp`（末尾追加测试）

**Interfaces:**
- Consumes: Task 1 产出的 `ToolManager`（`tools_` 映射、`hasTool`/`getAvailableTools`/`getTool`）。
- Produces: `qh::tool::ToolManager::unregisterTool` —— `bool unregisterTool(const std::string& name);`（按名移除，存在返回 true，不存在返回 false）。

- [ ] **Step 1: 在 `ToolManager.h` 声明 `unregisterTool`，在 `.cpp` 加桩**

`core/tool/ToolManager.h` —— 在 `hasTool` 声明之后插入：

```cpp
    // 是否存在同名工具
    bool hasTool(const std::string& name) const;

    // 注销工具：按名移除；不存在返回 false
    bool unregisterTool(const std::string& name);
```

`core/tool/ToolManager.cpp` —— 在 `hasTool` 实现之后加桩（刻意返回 false，驱动 RED）：

```cpp
bool ToolManager::hasTool(const std::string& name) const {
    return tools_.count(name) > 0;
}

bool ToolManager::unregisterTool(const std::string& /*name*/) {
    return false;  // 桩
}
```

- [ ] **Step 2: 在 `test_ToolManager.cpp` 末尾追加注销测试**

```cpp
QH_TEST(toolmanager_unregister) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");
    tm.registerTool(bash);

    QH_CHECK(tm.unregisterTool("bash"));          // 注销已存在：true
    QH_CHECK(!tm.hasTool("bash"));                // 注销后查不到
    QH_CHECK(tm.getTool("bash") == nullptr);
    QH_CHECK_EQ(tm.getAvailableTools().size(), static_cast<size_t>(0));

    QH_CHECK(!tm.unregisterTool("bash"));         // 再次注销：false
    QH_CHECK(!tm.unregisterTool("never"));        // 注销不存在的工具：false
}
```

- [ ] **Step 3: 构建并运行，确认 RED**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/core/Release/qharness_core_tests.exe; echo "exit=$?"
```
Expected: `[ toolmanager_unregister ]` 下出现 `FAIL: tm.unregisterTool("bash")`（桩返回 false），末行 `M failures`（`M > 0`），`exit=1`。

- [ ] **Step 4: 用真实实现替换 `ToolManager.cpp` 中的 `unregisterTool` 桩（GREEN）**

```cpp
bool ToolManager::unregisterTool(const std::string& name) {
    return tools_.erase(name) > 0;
}
```

- [ ] **Step 5: 构建并运行，确认 GREEN**

Run:
```bash
cmake --build build --config Release --target qharness_core_tests && build/core/Release/qharness_core_tests.exe; echo "exit=$?"
```
Expected: 无 `FAIL:` 行；末行 `N checks, 0 failures`；`exit=0`。

- [ ] **Step 6: 提交（需用户确认后执行）**

```bash
git add core/tool/ToolManager.h core/tool/ToolManager.cpp core/tests/test_ToolManager.cpp
git commit -m "工具管理器补充注销接口(unregisterTool,含单元测试)"
```

---

## 验收标准

- 全部新增/修改文件就位，`qharness_core` 与 `qharness_core_tests` 编译链接通过。
- `build/core/Release/qharness_core_tests.exe` 退出码 0，`0 failures`（含既有 schema/skeleton 用例不回归）。
- `ToolManager` 实现 `ToolRegistry` 全部契约；engine 后续只需依赖 `ToolRegistry` 抽象。
