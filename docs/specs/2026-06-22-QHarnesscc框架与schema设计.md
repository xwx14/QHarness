# QHarnesscc 框架与 Schema 设计文档

- 日期：2026-06-22
- 语言/构建：C++17 / CMake / MSVC
- 依赖：Qt 5.14.2（msvc2017_64）、nlohmann/json、cpp-httplib
- 参考实现：`E:\MyHarness\go-tiny-claw`（Go 版 Harness，本工程为其 C++ 演进版）

## 1. 目标与范围

构建一个依赖 Qt 的可扩展大模型 Harness，初期分为两个模块：

1. **基础库模块（core）**：抽象基类 + 基础实现，输出静态库 `qharness_core`。
2. **界面模块（app）**：主窗口及控件（QDockWidget），输出可执行 `qharness_app`。

本次推进范围（用户确认）：

- 建立完整工程骨架：根 CMake + 两模块 CMake + MSVC 编译 bat（建立后实测修正）。
- 各抽象类以 **pure virtual 仅声明**的形式给出（无 .cpp 实现），忠实反映 Go 契约。
- **完整实现** `schema/message.go` 的数据结构，含 JSON 序列化/反序列化。
- 界面模块做到"空窗口可编译运行"，验证 Qt 链接正确。

YAGNI 原则：未在本步列出的能力（流式、错误自愈、注册中心等）一律留待后续步骤。

## 2. 工程结构

```
QHarnesscc/
├── CMakeLists.txt              根工程：find_package(Qt5)，聚合两模块
├── build_msvc.bat             MSVC 编译脚本（全英文，建立后测试）
├── third/                      （已存在）httplib.h + nlohmann/
├── core/                       基础库模块 → 静态库 qharness_core
│   ├── CMakeLists.txt
│   ├── schema/                 ★本步重点：message.go 数据结构
│   │   ├── Role.h
│   │   ├── ToolCall.h / .cpp
│   │   ├── ToolResult.h / .cpp
│   │   ├── ToolDefinition.h / .cpp
│   │   └── Message.h / .cpp
│   ├── engine/   Engine.h(抽象)  EngineReActLoop.h
│   ├── provider/ Provider.h  ProviderOpenAI.h  ProviderClaude.h
│   ├── tool/     Tool.h
│   ├── robot/    Robot.h  RobotFeishu.h
│   └── memory/   Memory.h  MemoryFile.h
└── app/                        界面模块 → 可执行 qharness_app
    ├── CMakeLists.txt
    ├── main.cpp
    └── MainWindow.h / .cpp     QDockWidget：日志窗口 + 对话窗口
```

约定：

- 命名空间：`qh::schema`、`qh::engine`、`qh::provider`、`qh::tool`、`qh::robot`、`qh::memory`（`qh` = QHarness 前缀）。
- 命名风格：camelCase（遵循全局 CLAUDE.md）。
- C++ 标准：C++17。
- 字符编码：源文件 UTF-8（含 BOM，MSVC 对中文注释更稳妥）。

## 3. Schema 数据结构设计（本步核心）

忠实移植 `go-tiny-claw/internal/schema/message.go`，逐类型对应：

| Go 类型 | C++ 类型 | 说明 |
|---|---|---|
| `Role`(string) | `enum class Role{System,User,Assistant}` | 枚举替代字符串字面量；配 `roleToString`/`roleFromString` 与 nlohmann ADL |
| `Message` | `struct Message` | `role`/`content`/`toolCalls`/`toolCallId` |
| `ToolCall` | `struct ToolCall` | `id`/`name`/`arguments`(原始JSON) + `parseArguments()` |
| `ToolResult` | `struct ToolResult` | `toolCallId`/`output`/`isError` |
| `ToolDefinition` | `struct ToolDefinition` | `name`/`description`/`inputSchema`(nlohmann::json) |

### 3.1 Arguments 的"两者结合"表达

Go 注释明确 `json.RawMessage` 的语义为"延迟解析，将解析责任交给具体的工具"。C++ 采取：

- 字段：`std::string arguments;`（存原始 JSON 文本，反序列化时保留原文，零成本）。
- 方法：`nlohmann::json parseArguments() const;`（按需解析为结构化对象，交给具体 Tool 使用）。
- 序列化：`to_json` 直接把字符串内容写回（nlohmann 会将其作为已转义的 JSON 值嵌入，需用 `json::parse` 或 `raw_json` 技巧保证 `arguments` 输出为 JSON 对象而非字符串，见 3.3）。

### 3.2 InputSchema 的表达

Go 的 `interface{}` 在 C++ 用 `nlohmann::json inputSchema;`。对应 JSON Schema（一个对象），天然适配。

### 3.3 JSON 序列化约定

采用 nlohmann **ADL 惯用法**：在 `qh::schema` 命名空间内为每个类型提供自由函数 `to_json(json&, const T&)` 与 `from_json(const json&, T&)`。好处：

- `nlohmann::json j = message;` 与 `message = j.get<Message>();` 直接可用。
- 与标准库容器（`std::vector<Message>`）自动适配。

要点：

- `Message.toolCalls` 使用 `omitempty` 语义——为空时序列化省略该字段（与 Go `omitempty` 一致）；`toolCallId` 为空亦省略。
- `ToolCall.arguments` 序列化时必须是原始 JSON 值（对象/数组），而非被二次转义的字符串。实现上：写时 `j["arguments"] = nlohmann::json::parse(tc.arguments, nullptr, false);`（容错：解析失败则保留字符串）；读时 `tc.arguments = raw.value().dump()`。
- `ToolDefinition.inputSchema` 直接作为 json 对象读写，无需特殊处理。

## 4. 抽象类骨架（pure virtual 仅声明）

接口忠实反映 Go 契约，本步仅声明、不实现（后续步骤填充）。每个类一个头文件，无 .cpp。

- **`qh::engine::Engine`**（抽象）
  - `virtual ~Engine() = default;`
  - `virtual void run(const std::string& userPrompt) = 0;`
- **`qh::engine::EngineReActLoop`**（继承 Engine）
  - 声明标准 ReAct 循环（思考→行动→观察）的骨架，成员预留 `Provider*`、工具注册、`workDir`。本步仅声明 `run` 覆写与构造，方法体留空/TBD。
- **`qh::provider::Provider`**（抽象）
  - `virtual Message generate(const std::vector<Message>& messages, const std::vector<ToolDefinition>& tools) = 0;`
- **`qh::provider::ProviderOpenAI` / `ProviderClaude`**：继承 Provider，声明构造（apiKey/baseUrl/model），实现留 TBD。本步**不**包含 `httplib.h`（无 HTTP 代码则不引入巨型头文件，遵循 YAGNI），待实现步骤再纳入。
- **`qh::tool::Tool`**（抽象）
  - `virtual ToolDefinition definition() const = 0;`
  - `virtual ToolResult execute(const ToolCall& call) = 0;`
- **`qh::memory::Memory`**（抽象）：`virtual void load() = 0; virtual void save() = 0;`；**`MemoryFile`**：声明基于文件的实现骨架（构造接收 `basePath`）。
- **`qh::robot::Robot`**（抽象）：`virtual void send(const std::string& message) = 0;`；**`RobotFeishu`**：声明飞书机器人骨架。

说明：Provider 抽象本步暂用同步返回 `Message` + 异常表达错误（`throw std::runtime_error`），流式留待后续。

## 5. 界面模块（最小可运行）

- `MainWindow : public QMainWindow`，两个 `QDockWidget`：
  - **日志窗口**：`QPlainTextEdit`（只读），用于后续 Engine 日志输出。
  - **对话窗口**：`QLineEdit`/`QTextEdit` 输入 + `QListWidget`/`QTextBrowser` 消息列表。
- `main.cpp`：`QApplication` + 实例化 `MainWindow`。
- 本步目标：能编译运行出带两个停靠窗口的空主窗口，验证 Qt5::Widgets 链接正确。

## 6. CMake 组织

- **根 `CMakeLists.txt`**：
  - `cmake_minimum_required(VERSION 3.15)`；`project(QHarnesscc LANGUAGES CXX)`；`set(CMAKE_CXX_STANDARD 17)`，强制 `CXX_STANDARD_REQUIRED ON`。
  - `find_package(Qt5 COMPONENTS Core Widgets REQUIRED)`。
  - 将 `third/` 加入全局 include（`include_directories(${CMAKE_SOURCE_DIR}/third)`）。
  - `add_subdirectory(core)`、`add_subdirectory(app)`。
- **`core/CMakeLists.txt`**：`add_library(qharness_core STATIC <schema源文件>)`；`target_link_libraries(qharness_core PUBLIC Qt5::Core)`；抽象类头文件通过 `target_include_directories(... PUBLIC core)` 暴露。UTF-8 BOM：设置源文件 `COMPILE_FLAGS "/utf-8"`。
- **`app/CMakeLists.txt`**：`add_executable(qharness_app WIN32 main.cpp MainWindow.h MainWindow.cpp)`；`target_link_libraries(qharness_app PRIVATE qharness_core Qt5::Widgets)`。`WIN32` 让其作为 GUI 程序（无控制台黑窗）。

## 7. bat 编译脚本

全英文（遵循 CLAUDE.md）。策略：

1. 用 `vswhere` 探测已装 Visual Studio 版本，动态生成 `-G "Visual Studio NN YYYY"` 参数。
2. 通过 `-DCMAKE_PREFIX_PATH` 指定 Qt 路径 `E:\programProject\Qt\5.14.2\msvc2017_64`。
3. `cmake -B build -S .` 配置；`cmake --build build --config Release` 编译。
4. 建立后**实际运行**该脚本进行测试与修正（遵循 CLAUDE.md「建立后要进行测试修正」）。

## 8. 验证标准（本步完成判据）

1. `build_msvc.bat` 能成功配置并编译，生成 `qharness_core.lib` 与 `qharness_app.exe`。
2. Schema 各类型具备 `to_json`/`from_json`，且 `Message` 往返（含空 toolCalls 的 omitempty、`arguments` 原始 JSON 值）正确。
3. 运行 `qharness_app.exe` 弹出含日志/对话两个停靠窗口的主窗口。
4. 抽象类头文件可被 `#include` 且（如被继承）可实例化子类，无编译错误。

## 9. 后续步骤（不在本步范围）

- ProviderOpenAI/ProviderClaude 的真实 HTTP 请求与响应解析（基于 httplib.h）。
- Tool 基础实现（edit/read/bash）、工具注册分发（Registry）。
- EngineReActLoop 的完整 ReAct 循环实现。
- MemoryFile / RobotFeishu 实现。
- 界面与 Engine 的信号槽联动、日志输出接入。
