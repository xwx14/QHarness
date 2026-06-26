# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

QHarnesscc 是依赖 Qt 的可扩展大模型（LLM）Harness，C++17 + CMake + MSVC。它是 Go 项目 E:\MyHarness\go-tiny-clawBai\go-tiny-claw的 C++ 演进版，目标是把 Go 版的 schema/Engine/Provider/Tool/Memory/Interaction 架构移植到 C++ 并加上 Qt 界面。

**当前完成度**：schema 数据结构与序列化（完整）、抽象类骨架（仅声明）、Qt 主窗口（最小可运行）。Engine/Provider/Tool/Memory/Interaction 的派生类方法体仍是 `/* TODO */` 占位，待后续步骤实现。

## 构建 / 测试 / 运行

工具链：cmake 4.1.1、Qt 5.14.2（msvc2017_64）、VS（用 VS17 生成器构建）。Qt 路径硬编码在 `build_msvc.bat`。

```bash
# 一键构建（自动探测 VS、配置、构建 Release、windeployqt 部署 Qt 运行时）
./build_msvc.bat

# 手动配置（首次或更换 Qt 路径时）
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=E:/programProject/Qt/5.14.2/msvc2017_64

# CMakeLists 改动后重新配置（复用缓存的生成器与 Qt 路径）
cmake -S . -B build

# 构建（Release / Debug）
cmake --build build --config Release
cmake --build build --config Debug

# 构建并运行核心测试（core 为 SHARED DLL，测试 exe 与 qharness_core.dll 同在 build/out/<config>）
cmake --build build --config Release --target qharness_core_tests
build/out/Release/qharness_core_tests.exe

# 运行 GUI（exe 与 Qt DLL 已部署到 build/out/<config>）
build/out/Release/qharness_app.exe
```

**测试框架**：`core/tests/TestHarness.h` 自带轻量框架，无外部依赖。宏 `QH_TEST(name){...}`、`QH_CHECK(cond)`、`QH_CHECK_EQ(a,b)`。测试用例通过静态注册自动收集，`core/tests/TestMain.cpp` 迭代运行**全部**注册用例——**目前无法只跑单个测试**，隔离需临时注释其他用例。新增测试：写 `core/tests/test_X.cpp`（用 `QH_TEST`），并在 `core/CMakeLists.txt` 的 `qharness_core_tests` 源列表中登记。

## 架构

两个 CMake 模块，依赖边界严格：

- **`core/` → 动态链接库 `qharness_core`（SHARED）**：纯 C++17，**不依赖 Qt**（只用 vendored 的 nlohmann/json）。`core/CMakeLists.txt` 不链接任何 Qt 目标。导出宏 `QH_API`（`core/qh_export.h`：编译 core 时由 `QH_BUILDING_DLL` 切换为 `dllexport`，消费方为 `dllimport`）；公共类整类 `class QH_API X` 导出。`qh_export.h` 集中抑制 `C4251`（导出含 STL 成员的类，同 CRT 下安全）。
- **`app/` → 可执行 `qharness_app`**：Qt5 Widgets，依赖 `qharness_core`。`MainWindow`（QMainWindow）含两个 `QDockWidget`（日志/对话）。

### Schema（`core/schema/Message.h` + `Message.cpp`）

5 个数据类型集中在一个文件，忠实 Go 的 `message.go` 单文件结构：

- `enum class Role { System, User, Assistant }`（+ `roleToString`/`roleFromString` + `NLOHMANN_JSON_SERIALIZE_ENUM` 宏，宏必须置于 `qh::schema` 命名空间内、枚举定义之后）
- `class ToolCall { id, name, arguments, parseArguments() }`、`class ToolResult`、`class ToolDefinition`、`class Message`

序列化用 nlohmann **ADL 惯用法**：自由函数 `to_json/from_json` 在 `qh::schema` 命名空间内（`std::vector<ToolCall>` 等容器由此自动获得序列化）。关键语义：
- `ToolCall::arguments` 是 `std::string`（原始 JSON 文本，对应 Go 的 `json.RawMessage`「延迟解析」），`parseArguments()` 按需解析。`to_json` 用容错 `json::parse(raw, nullptr, false)`（非法 JSON 退化为空对象，不抛异常），`from_json` 用 `dump()` 保留原文。
- `Message` 的 omitempty：空的 `toolCalls`/`toolCallId` 不输出该字段。
- 这些类型用 `class` + 首行 `public:`（数据成员须公开，nlohmann 的自由函数与外部代码需直接访问）。

### 抽象类骨架（声明/实现分离）

`engine/{Engine,EngineReActLoop}`、`provider/{Provider,ProviderOpenAI,ProviderClaude,MockProvider}`、`tool/{Tool,ToolManager}`、`interaction/{Interaction,InteractionFeishu}`、`memory/{Memory,MemoryFile}`、`context/{Composer}`。基类纯虚（`virtual ... = 0`）；派生类的构造函数与方法体在对应 `.cpp`（骨架阶段为 `/* TODO */` 占位），头文件仅声明。`Provider::generate` 新签名为 `generate(const schema::CancellationToken&, messages, tools) -> GenerateResult`，失败约定 `GenerateResult::error` 非空（不再抛异常）；`CancellationToken`（`schema/CancellationToken.h`，`qh::schema`，轻量取消/超时，对齐 Go `context.Context` 核心语义）作为首参。`MockProvider` 忠实移植 Go `mockProvider`（`_turn` 计数：第 1 轮返回 `bash` 工具调用、第 2 轮起纯文本"任务完成"），用于引擎循环测试。`core/tests/skeleton_compile_check.cpp` 强制实例化各派生类做链接期检查。

### Settings（`core/schema/Settings.h` + `Settings.cpp`、`core/config/SettingsStore.h` + `SettingsStore.cpp`）

应用配置（两级 LLM 结构）：`ProviderType`(OpenAI/Claude) enum、`LlmModel`(name/temperature/maxTokens)、`LlmProvider`(name/providerType/baseUrl/apiKey + 多个 LlmModel)、`Settings`(多 providers + activeProviderName + activeModelName + enableThinking + workDir + enabledTools)。ADL to_json/from_json（空值 omitempty，`from_json` 用 `value()` 缺字段容错）。`findActiveProvider`/`findModel` helper。`SettingsStore`（core/config，纯 C++，路径由 app 注入）load/save 到 `<exeDir>/config/setting.json`：文件缺失→默认、JSON 损坏→默认+lastError。app 层 `MainWindow` 持 `_settings`，`SettingsDialog`(单 QTabWidget) 编辑——LLM Tab 左侧供应商列表（单击设当前）+ 右侧供应商表单 + 模型 QTableWidget（模型名/temp/maxTokens 可编辑 + 当前列 radio 单选写 activeModelName）。`EngineThread` 构造收 `Settings` 值拷贝，按 `findActiveProvider`+`findModel` 定位创建 ProviderOpenAI/Claude（未配回退 mock）。值注入——worker 线程只读自己拷贝，与 UI 零共享。

### 命名空间

`qh::schema`、`qh::engine`、`qh::provider`、`qh::tool`、`qh::interaction`、`qh::memory`、`qh::context`、`qh::app`、`qh::test`。`core/` 目录是 `qharness_core` 的公共 include 根，内部头文件用根相对路径引用（如 `#include "schema/Message.h"`、`#include "provider/Provider.h"`）。

## 项目专属约定

- **include guard 命名**：`QH_<模块>_<文件>_H`（如 `QH_SCHEMA_MESSAGE_H`、`QH_PROVIDER_OPENAI_H`）。
- **头文件列入 CMakeLists**：所有 `.h` 都加入 `add_library`/`add_executable` 源列表（VS 工程可见），不止列 `.cpp`。
- **构建输出**：可执行程序与全部 DLL（含 `qharness_core.dll`——core 为 SHARED，以及 windeployqt 部署的 Qt 运行时）与 `qharness_core_tests.exe` 统一输出到 `build/out/{Release,Debug}`，由各目标的 `*_OUTPUT_DIRECTORY` 控制。
- **源码 UTF-8**：MSVC 用 `/utf-8` 编译选项（core/app 的 CMakeLists 已配），中文注释/字符串才正常。

## 坑点

- **Anaconda Qt 干扰**：本机 PATH 上有 Anaconda 自带 Qt，会与项目所用 Qt 5.14.2 冲突。`app/CMakeLists.txt` 的 windeployqt post-build 把正确 Qt 的 `bin` 置于 PATH 最前再部署，**只能在 `build/out/<config>/` 运行 `qharness_app.exe`**（Qt DLL 与 `qharness_core.dll` 均已部署至此）；`qharness_core_tests.exe` 同样须在 `build/out/<config>/` 运行（core 现为 SHARED，依赖同目录的 `qharness_core.dll`）。`build/core/`、`build/app/` 下不再有可运行 exe。
- **vendored 依赖**：`third/` 含 `nlohmann/json` 与 `httplib.h`（cpp-httplib）。根 CMakeLists 通过 `include_directories(${CMAKE_SOURCE_DIR}/third)` 暴露，源码中 `#include <nlohmann/json.hpp>`。httplib 尚未使用（Provider 实现时再引入，应 include 在派生类 .cpp 而非头文件）。
- **更换 Qt 路径**：改 `build_msvc.bat` 的 `QT_PREFIX` 与手动配置命令的 `-DCMAKE_PREFIX_PATH`。

## 参考文档

- `说明.txt`：项目最初需求清单。
- `docs/specs/2026-06-22-QHarnesscc框架与schema设计.md`：设计文档（架构决策、字段映射、序列化语义）。
- `docs/plans/2026-06-22-QHarnesscc框架与schema实现计划.md`：8 任务实现计划（含完整代码与 TDD 步骤）。
- Go 参考实现：`E:\MyHarness\go-tiny-claw`（`internal/schema/message.go`、`internal/engine`、`internal/provider` 等）。
