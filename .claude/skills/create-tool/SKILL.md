---
name: create-tool
description: Use when 在 QHarnesscc 项目中新增一个引擎工具（继承 qh::tool::Tool、实现 execute 供 ReAct 循环调用）——需要按项目约定完成工具定义、设置勾选、引擎注册、路径穿越防护、编码处理与回归测试。
---

# QHarness 工具创建

## Overview

工具 = 继承 `core/tool/Tool` 的子类，被 `ToolManager` 按名注册、由 ReAct 引擎在模型发起 `tool_call` 时调用。新增工具要走完 **7 步固定流程**，漏任何一步（CMake 登记、设置勾选、引擎注册、路径防护、测试）都会让工具不可用或留下安全漏洞。

最完整的范例：`core/tool/ReadFileTool.{h,cpp}`。

## When to Use

- 新增一个引擎可调用的工具（读/写文件、执行命令、……）。
- **不要用于**：改 `ToolManager` 分发、改引擎循环（那是引擎层，非工具）。

## 7 步 SOP

### 1. Tool 子类（`core/tool/XxxTool.h`）

```cpp
#ifndef QH_TOOL_XXX_H
#define QH_TOOL_XXX_H
#include <string>
#include "tool/Tool.h"
#include "qh_export.h"
namespace qh { namespace tool {
class QH_API XxxTool : public Tool {
public:
    explicit XxxTool(std::string workDir);
    schema::ToolResult execute(const schema::ToolCall& call) override;
};
}} // namespace
#endif
```

### 2. 构造设 `_workDir` + `_definition`

```cpp
XxxTool::XxxTool(std::string workDir) {
    _workDir = std::move(workDir);                  // 基类 Tool::_workDir（UTF-8）
    _definition._name = "xxx";
    _definition._description = "做什么；请提供相对工作区的参数。";
    _definition._inputSchema["properties"]["param"]["type"] = "string";
    _definition._inputSchema["required"] = { "param" };
}
```

### 3. `execute` 模板（解析→路径安全→业务→截断）

```cpp
schema::ToolResult XxxTool::execute(const schema::ToolCall& call) {
    schema::ToolResult result; result._toolCallId = call._id;
    const auto args = call.parseArguments();        // 容错解析；JSON/nlohmann 要求 UTF-8
    if (!args.is_object() || !args.contains("param")) {
        result._isError = true; result._output = "参数解析失败"; return result;
    }
    const std::string relPath = args["param"].get<std::string>();
    const fs::path base = resolveWorkBase();        // 基类：_workDir 编码转换+规范化
    auto hit = resolveInside(base, relPath);        // 基类：穿越检查+存在性（读文件用）
    if (!hit) { result._isError = true; result._output = "失败"; return result; }
    // ↓ 业务逻辑；读文件用 contentCodec::toUtf8 转码、truncateUtf8Safe 截断
    return result;
}
```

### 4. `SettingsDialog` 登记（设置可选）

```cpp
// app/SettingsDialog.cpp —— 工具清单单一数据源，驱动列表构建/回显/收集
static const char* const kToolNames[] = { "bash", "read_file", "write_file", "xxx" };
```

### 5. `EngineThread` 注册（对话可用）

```cpp
// app/EngineThread.cpp 工具创建循环
std::unique_ptr<tool::Tool> t;
if      (name == "xxx")      t = std::make_unique<tool::XxxTool>(workDir);
// ... 其他工具
if (t) {
    _toolManager->registerTool(*t);          // 注册（非拥有引用）
    _tools.push_back(std::move(t));          // 所有权入 EngineThread::_tools
}
```

### 6. 测试 + CMakeLists 登记

```cpp
// core/tests/test_XxxTool.cpp
QH_TEST(xxxtool_basic) {
    qh::tool::XxxTool t(workDirPath);
    qh::schema::ToolCall call; call._id="c"; call._name="xxx";
    call._arguments = "{\"param\":\"...\"}";
    QH_CHECK(!t.execute(call)._isError);
}
```

```cmake
# core/CMakeLists.txt
#   qharness_core 源列表加：tool/XxxTool.h  tool/XxxTool.cpp
#   qharness_core_tests 加：tests/test_XxxTool.cpp
```

### 7. `run_tests.bat` 全绿

`./run_tests.bat` 必须 `[PASS] ... 0 failures`。

## Quick Reference

| 步 | 文件 | 关键点 |
|---|---|---|
| 1 | `core/tool/XxxTool.h` | 继承 Tool，QH_API 导出，include guard `QH_TOOL_XXX_H` |
| 2 | 构造 | 设 `_workDir` + `_definition`(name/desc/inputSchema) |
| 3 | `execute` | parseArguments → resolveWorkBase → resolveInside → 业务 |
| 4 | `app/SettingsDialog.cpp` | `kToolNames[]` 加名字 |
| 5 | `app/EngineThread.cpp` | `make_unique<XxxTool>(workDir)` + registerTool |
| 6 | `test_XxxTool.cpp` + CMakeLists | 登记 .h/.cpp + test |
| 7 | `run_tests.bat` | 0 failures |

## 关键约定

- **路径穿越**：读用 `resolveInside`（穿越检查+存在性）；写用 `resolveInsideForWrite`（穿越检查，不要求存在）。**绝不**直接拼 `_workDir + path`。
- **编码**：路径名 `PathCodec`（Windows GBK/UTF-8 候选）；文件内容 `ContentCodec::toUtf8`（BOM→UTF-8 校验→GBK→原样兜底）。
- **截断**：输出超 `kMaxLen`(8000) 用 `truncateUtf8Safe`（按 UTF-8 字符边界，不切断多字节字符）。
- **工作目录**：`_workDir` 注入（UTF-8），工具只在其内操作；bash 系列执行命令也限定到该目录。
- **读文件用 binary 模式**（`std::ios::binary`），保留原始字节给 `toUtf8`，文本模式会破坏 GBK/UTF-16。

## Common Mistakes

| 漏掉 | 后果 |
|---|---|
| CMakeLists 不登记 `.h`/`.cpp` | VS 工程看不到 / 不编译 |
| 忘 `kToolNames` | 设置对话框勾不到该工具 |
| 忘 `EngineThread` 分支 | 勾选了也不创建，对话中不可用 |
| 直接拼路径不走 `resolveInside` | **路径穿越漏洞**（`../../../etc/passwd`） |
| 文本模式读文件 | GBK/UTF-16 字节被 CRLF/Ctrl-Z 破坏 |
| 忘 `run_tests.bat` | 回归无保障 |

## 参考

- `core/tool/ReadFileTool.{h,cpp}`：读文件工具，最完整范例（binary 读 + toUtf8 + 截断）。
- `core/tool/Tool.{h,cpp}`：基类（`_workDir`/`resolveWorkBase`/`resolveInside`/`resolveInsideForWrite`）。
- `core/tool/PathCodec.h`（路径编码）/ `ContentCodec.h`（内容编码）。
- `CLAUDE.md`「回归测试」约定。
