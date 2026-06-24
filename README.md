# QHarness



使用C++和Qt实现的Harness。目标是构建一个**可插拔大模型、可扩展工具、带记忆与上下文工程**的 Agent 框架。

## ✨ 特性

- **分层架构**：schema（数据结构）+ 六层抽象（Interaction / Engine / Provider / Context / Memory / Tool），层间依赖边界清晰
- **core 与 app 分离**：core 是纯 C++17 动态链接库（不依赖 Qt），可独立测试；app 是 Qt5 Widgets 界面
- **schema 已完成**：5 个数据类型 + nlohmann ADL 序列化，JSON 键名与 Go 参考实现的 wire 格式一致
- **轻量自测**：自带零依赖测试框架 `TestHarness.h`
- **可插拔大模型**：Provider 抽象，已声明 OpenAI / Claude 适配器骨架

## 📐 架构

<img src="media\架构图.png" style="zoom:50%;" />

```
QHarnesscc/
├── core/                 # qharness_core (SHARED DLL, 纯 C++17, 不依赖 Qt)
│   ├── schema/           # 数据结构 Message.h/.cpp（已完成）
│   ├── engine/           # Engine 抽象 + EngineReActLoop 骨架
│   ├── provider/         # Provider 抽象 + OpenAI/Claude 骨架
│   ├── tool/             # Tool 抽象 + ToolManager（已实现）
│   ├── interaction/      # Interaction 抽象 + InteractionFeishu 骨架
│   ├── memory/           # Memory 抽象 + MemoryFile 骨架
│   ├── context/          # Composer 上下文组装器骨架
│   ├── tests/            # 轻量测试框架 + 用例
│   └── qh_export.h       # QH_API 导出宏
├── app/                  # qharness_app (Qt5 Widgets 可执行程序)
│   ├── MainWindow.*      # 主窗口（QMainWindow + 两个 QDockWidget）
│   ├── ChatDock.*        # 对话窗口（QTextBrowser + QPlainTextEdit + 发送按钮）
│   └── LogDock.*         # 日志窗口（QPlainTextEdit）
├── third/                # vendored 依赖（nlohmann/json、httplib.h）
├── docs/                 # 设计文档与开发笔记
├── CMakeLists.txt
└── build_msvc.bat        # 一键构建脚本
```

**命名空间**：`qh::schema`、`qh::engine`、`qh::provider`、`qh::tool`、`qh::interaction`、`qh::memory`、`qh::context`、`qh::app`。

core 编译为 SHARED DLL，公共类通过 `QH_API` 宏导出；app 链接 core 的导入库。

## 🔧 依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| CMake | 4.1.1 | 构建系统 |
| Qt | 5.14.2 (msvc2017_64) | app 界面层 |
| Visual Studio | 2022 (VS17, MSVC) | 编译器 |
| nlohmann/json | vendored (`third/`) | schema 序列化 |
| cpp-httplib | vendored (`third/httplib.h`) | HTTP 客户端（Provider/Interaction 实现时引入） |

## 🚀 构建

### 一键构建（推荐）

```bash
./build_msvc.bat
```

自动探测 Visual Studio、配置、构建 Release、并用 windeployqt 部署 Qt 运行时到 `build/out/Release/`。

### 手动构建

```bash
# 首次配置（指定 Qt 路径）
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_PREFIX_PATH=E:/programProject/Qt/5.14.2/msvc2017_64

# 构建 Release / Debug
cmake --build build --config Release
cmake --build build --config Debug
```

构建产物（可执行程序 + 全部 DLL + Qt 运行时）统一输出到 `build/out/{Release,Debug}/`。

## 🧪 测试

core 自带轻量测试框架（`core/tests/TestHarness.h`，零外部依赖）：

```bash
cmake --build build --config Release --target qharness_core_tests
./build/out/Release/qharness_core_tests.exe
```

> core 为 SHARED DLL，测试 exe 必须在 `build/out/<config>/` 运行（依赖同目录的 `qharness_core.dll`）。

当前 63 个断言全部通过，覆盖 schema 序列化与 ToolManager 全部行为（注册 / 查找 / 注销 / 列举 / 分发 / 未知工具）。

## ▶️ 运行

```bash
./build/out/Release/qharness_app.exe
```

> ⚠️ 必须在 `build/out/<config>/` 目录下运行（Qt DLL 与 `qharness_core.dll` 已部署至此；本机 PATH 上的 Anaconda Qt 会与项目 Qt 冲突）。

界面说明：

- **ChatDock**（顶部停靠）：上部 `QTextBrowser` 显示对话历史，下部 `QPlainTextEdit` 多行输入 + 右侧「发送」按钮；**Ctrl+Enter 快捷发送**，普通回车换行
- **LogDock**（底部停靠）：`QPlainTextEdit` 显示运行日志

> 当前为框架阶段，界面仅 UI 冒烟，尚未接入 core 的 Engine。

## 📌 项目状态

| 模块 | 状态 |
|------|------|
| schema（数据结构 + 序列化） | ✅ 完整实现 + 测试 |
| ToolManager（工具注册 / 分发） | ✅ 完整实现 + 测试 |
| Engine / Provider / Interaction / Memory / Composer | 🚧 骨架（接口已定义，方法体 TODO） |
| app 界面 | 🚧 UI 冒烟（未接 Engine） |

### 路线图

1. 实现 `ProviderOpenAI`，跑通首个大模型调用
2. 实现 `EngineReActLoop` 的 ReAct 循环
3. 实现 `Composer` 组装 + `MemoryFile` 存取
4. 注册基础工具（读写文件、执行命令等）
5. app 接入 Engine，打通完整对话

## 📚 参考

- 设计文档：`docs/specs/`
- 实现计划：`docs/plans/`
- 极客时间课程《从 0 开始构建 Agent Harness》

<img src="media\agentHarnessCourse.jpg" style="zoom:33%;" />

## 📄 协议

本项目采用 **GNU General Public License v3.0** 开源协议。详见 [LICENSE](LICENSE)。

```
QHarnesscc - 依赖 Qt 的可扩展大模型 Harness
Copyright (C) 2026  QHarnesscc Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
```

---

*QHarnesscc · C++17 + Qt5 · GPL-3.0*
