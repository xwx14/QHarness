# QHarnesscc 框架与 Schema 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立 Qt + CMake 双模块工程骨架，并完整实现 `go-tiny-claw/internal/schema/message.go` 数据结构的 C++ 版本（含 JSON 序列化/反序列化）与可编译运行的最小界面。

**Architecture:** 两个 CMake 目标——`core`（静态库 `qharness_core`，纯 C++17 + nlohmann，无 Qt 依赖）与 `app`（可执行 `qharness_app`，依赖 Qt5 + core）。Schema 类型用 nlohmann ADL 惯用法实现 `to_json/from_json`。抽象类以 header-only 骨架形式给出（基类纯虚、派生类内联 TODO 体）。测试用项目自带轻量框架（静态注册 + `QH_TEST` 宏），避免引入外部测试依赖。

**Tech Stack:** C++17、CMake ≥3.15、MSVC（VS 2017+）、Qt 5.14.2（msvc2017_64）、nlohmann/json 3.12.0（vendored 于 `third/nlohmann`）、cpp-httplib（vendored 于 `third/httplib.h`）。

## Global Constraints

- **语言/命名**：C++17，camelCase；命名空间 `qh::schema` / `qh::engine` / `qh::provider` / `qh::tool` / `qh::robot` / `qh::memory` / `qh::app` / `qh::test`。
- **源码编码**：UTF-8（含中文注释），MSVC 用 `/utf-8` 编译选项。
- **第三方头文件**：已 vendored 于 `third/`；根 CMake 用 `include_directories(${CMAKE_SOURCE_DIR}/third)` 暴露，源码中 `#include <nlohmann/json.hpp>`。
- **Qt 路径**：`E:\programProject\Qt\5.14.2\msvc2017_64`，通过 `-DCMAKE_PREFIX_PATH` 传入。
- **无 git 操作**：本项目非 git 仓库；计划中的"提交"步骤一律替换为**构建/测试验证检查点**（运行测试可执行并确认 PASS）。若用户后续要求建立 git，另行处理。
- **核心库 Qt 无关**：`qharness_core` 不链接 Qt；只有 `qharness_app` 链接 `Qt5::Core Qt5::Widgets`。依赖边界清晰。
- **include 路径约定**：`core/` 目录作为 core 库的公共 include 根（`target_include_directories(qharness_core PUBLIC core/)`）。所有内部头文件用根相对路径引用，如 `#include "schema/Message.h"`、`#include "engine/Engine.h"`。

## File Structure

| 文件 | 职责 |
|---|---|
| `CMakeLists.txt` | 根工程：C++17 设置、`include_directories(third)`、`find_package(Qt5)`、`enable_testing()`、聚合 core/app |
| `build_msvc.bat` | MSVC 一键构建脚本（全英文） |
| `.gitignore` | 忽略 build/ 等生成物 |
| `core/CMakeLists.txt` | 构建静态库 `qharness_core` + 测试可执行 `qharness_core_tests` |
| `core/schema/Role.h/.cpp` | `enum class Role` + 序列化宏 + 字符串辅助 |
| `core/schema/ToolCall.h/.cpp` | `struct ToolCall`（`arguments` 原始 JSON 文本 + `parseArguments()`） |
| `core/schema/ToolResult.h/.cpp` | `struct ToolResult` |
| `core/schema/ToolDefinition.h/.cpp` | `struct ToolDefinition`（`inputSchema` 为 json） |
| `core/schema/Message.h/.cpp` | `struct Message`（omitempty 语义） |
| `core/engine/Engine.h` | 抽象基类 |
| `core/engine/EngineReActLoop.h` | ReAct 循环骨架 |
| `core/provider/Provider.h` | 抽象基类 |
| `core/provider/ProviderOpenAI.h` | OpenAI 连接骨架 |
| `core/provider/ProviderClaude.h` | Claude 连接骨架 |
| `core/tool/Tool.h` | 抽象基类 |
| `core/robot/Robot.h` / `core/robot/RobotFeishu.h` | 抽象 + 飞书骨架 |
| `core/memory/Memory.h` / `core/memory/MemoryFile.h` | 抽象 + 文件骨架 |
| `core/tests/TestHarness.h` | 轻量测试框架（注册 + `QH_TEST/QH_CHECK` 宏） |
| `core/tests/TestMain.cpp` | 测试入口：迭代注册表 + 汇总 |
| `core/tests/test_*.cpp` | 各 schema 类型的测试 |
| `core/tests/skeleton_compile_check.cpp` | 骨架可编译/可实例化检查 |
| `app/CMakeLists.txt` | 构建可执行 `qharness_app` |
| `app/main.cpp` | QApplication 入口 |
| `app/MainWindow.h/.cpp` | 主窗口 + 日志/对话两个 QDockWidget |

---

### Task 1: CMake 工程骨架 + MSVC 编译脚本（Tracer Bullet）

**目标**：建立根 + 两模块 CMake 与 bat，用最小 `main`（仅创建 `QApplication`）证明 Qt + MSVC + CMake 工具链端到端可用。

**Files:**
- Create: `CMakeLists.txt`
- Create: `.gitignore`
- Create: `core/CMakeLists.txt`
- Create: `app/CMakeLists.txt`
- Create: `app/main.cpp`（最小占位，Task 8 替换）
- Create: `build_msvc.bat`

**Interfaces:**
- Produces: 构建产物 `build/app/Release/qharness_app.exe`（最小版）、CMake 工具链验证通过。后续任务向 `core/CMakeLists.txt` 追加源文件。

- [ ] **Step 1: 写根 `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.15)
project(QHarnesscc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# 第三方头文件（httplib + nlohmann）全局可见
include_directories(${CMAKE_SOURCE_DIR}/third)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

enable_testing()

add_subdirectory(core)
add_subdirectory(app)
```

- [ ] **Step 2: 写 `.gitignore`**

```gitignore
build/
out/
*.user
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
*.lib
*.exp
*.pdb
*.ilk
```

- [ ] **Step 3: 写 `core/CMakeLists.txt`（最小空库）**

```cmake
add_library(qharness_core STATIC)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(qharness_core PUBLIC /utf-8 /W3)
endif()
```

- [ ] **Step 4: 写 `app/CMakeLists.txt`**

```cmake
add_executable(qharness_app WIN32
    main.cpp
)
target_link_libraries(qharness_app PRIVATE qharness_core Qt5::Core Qt5::Widgets)
if(MSVC)
    target_compile_options(qharness_app PRIVATE /utf-8)
endif()
```

- [ ] **Step 5: 写 `app/main.cpp`（占位）**

```cpp
#include <QApplication>

// 占位入口：Task 8 将替换为 MainWindow 实例化
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    return 0;
}
```

- [ ] **Step 6: 写 `build_msvc.bat`（全英文）**

```bat
@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem  QHarnesscc - MSVC one-click build script (English only)
rem ============================================================

set "QT_PREFIX=E:\programProject\Qt\5.14.2\msvc2017_64"
set "SOURCE_DIR=%~dp0"
set "BUILD_DIR=%SOURCE_DIR%build"

rem --- Locate Visual Studio via vswhere ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Install Visual Studio with C++ build tools.
    exit /b 1
)

set "VS_MAJOR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do set "VS_VERSION=%%i"
if defined VS_VERSION set "VS_MAJOR=!VS_VERSION:~0,2!"

set "CMAKE_GEN="
if "!VS_MAJOR!"=="17" set "CMAKE_GEN=Visual Studio 17 2022"
if "!VS_MAJOR!"=="16" set "CMAKE_GEN=Visual Studio 16 2019"
if "!VS_MAJOR!"=="15" set "CMAKE_GEN=Visual Studio 15 2017"

if not defined CMAKE_GEN (
    echo [ERROR] Could not determine Visual Studio generator. Detected version: !VS_VERSION!
    exit /b 1
)

echo [INFO] Qt prefix : %QT_PREFIX%
echo [INFO] Generator : !CMAKE_GEN!

rem --- Configure ---
cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "!CMAKE_GEN!" -A x64 -DCMAKE_PREFIX_PATH="%QT_PREFIX%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

rem --- Build ---
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [INFO] Build succeeded: %BUILD_DIR%app\Release\qharness_app.exe
endlocal
```

- [ ] **Step 7: 运行 bat 验证工具链**

Run: 在 `E:\MyHarness\QHarnesscc` 下执行 `build_msvc.bat`
Expected: 末行打印 `[INFO] Build succeeded: ...\qharness_app.exe`，无 errorlevel。

- [ ] **Step 8: 验证检查点（替代 git 提交）**

确认 `build/app/Release/qharness_app.exe` 已生成。若 vswhere 探测失败，根据报错调整 `CMAKE_GEN` 或在 bat 中显式指定 `-G "Visual Studio 17 2022"` 后重试。

---

### Task 2: Schema - Role

**Files:**
- Create: `core/schema/Role.h`
- Create: `core/schema/Role.cpp`
- Create: `core/tests/TestHarness.h`
- Create: `core/tests/TestMain.cpp`
- Create: `core/tests/test_Role.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::schema::Role`（`enum class`）、`roleToString`、`roleFromString`、nlohmann 对 Role 的自动序列化（`json j = Role::System;` → `"system"`）。

- [ ] **Step 1: 写 `core/tests/TestHarness.h`（轻量测试框架）**

```cpp
#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace qh {
namespace test {

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

inline int& failCount() { static int c = 0; return c; }
inline int& totalCount() { static int c = 0; return c; }

struct Registrar {
    Registrar(const char* name, void (*fn)()) {
        registry().push_back({name, fn});
    }
};

} // namespace test
} // namespace qh

#define QH_TEST(name)                                              \
    static void name();                                            \
    static ::qh::test::Registrar qh_reg_##name(#name, &name);      \
    static void name()

#define QH_CHECK(cond)                                                      \
    do {                                                                    \
        ::qh::test::totalCount()++;                                         \
        if (!(cond)) {                                                      \
            ::qh::test::failCount()++;                                      \
            std::cerr << "  FAIL: " << #cond << " (" << __FILE__ << ":"     \
                      << __LINE__ << ")\n";                                 \
        }                                                                   \
    } while (0)

#define QH_CHECK_EQ(a, b)                                                   \
    do {                                                                    \
        ::qh::test::totalCount()++;                                         \
        auto _qhA = (a);                                                    \
        auto _qhB = (b);                                                    \
        if (!(_qhA == _qhB)) {                                              \
            ::qh::test::failCount()++;                                      \
            std::cerr << "  FAIL: " << #a << " != " << #b << " ("           \
                      << __FILE__ << ":" << __LINE__ << ")\n"               \
                      << "    lhs=[" << _qhA << "]\n"                       \
                      << "    rhs=[" << _qhB << "]\n";                      \
        }                                                                   \
    } while (0)
```

- [ ] **Step 2: 写 `core/tests/TestMain.cpp`**

```cpp
#include "TestHarness.h"
#include <iostream>

int main() {
    std::cout << "=== QHarness Core Tests ===\n";
    for (auto& tc : qh::test::registry()) {
        std::cout << "[ " << tc.name << " ]\n";
        tc.fn();
    }
    std::cout << "\n" << qh::test::totalCount() << " checks, "
              << qh::test::failCount() << " failures\n";
    return qh::test::failCount() == 0 ? 0 : 1;
}
```

- [ ] **Step 3: 写失败测试 `core/tests/test_Role.cpp`**

```cpp
#include "TestHarness.h"
#include "schema/Role.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(role_string_roundtrip) {
    QH_CHECK_EQ(qh::schema::roleToString(qh::schema::Role::System), std::string("system"));
    QH_CHECK_EQ(qh::schema::roleToString(qh::schema::Role::User), std::string("user"));
    QH_CHECK_EQ(qh::schema::roleToString(qh::schema::Role::Assistant), std::string("assistant"));
    QH_CHECK(qh::schema::roleFromString("assistant") == qh::schema::Role::Assistant);
}

QH_TEST(role_json_serialization) {
    json j = qh::schema::Role::System;          // to_json
    QH_CHECK_EQ(j.get<std::string>(), std::string("system"));

    auto role = j.get<qh::schema::Role>();      // from_json
    QH_CHECK(role == qh::schema::Role::System);
}
```

- [ ] **Step 4: 更新 `core/CMakeLists.txt` 加入 Role 与测试**

将 `core/CMakeLists.txt` 整体替换为：

```cmake
add_library(qharness_core STATIC
    schema/Role.cpp
)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(qharness_core PUBLIC /utf-8 /W3)
endif()

# 测试可执行
add_executable(qharness_core_tests
    tests/TestMain.cpp
    tests/test_Role.cpp
)
target_link_libraries(qharness_core_tests PRIVATE qharness_core)
if(MSVC)
    target_compile_options(qharness_core_tests PRIVATE /utf-8)
endif()
add_test(NAME schema_tests COMMAND qharness_core_tests)
```

- [ ] **Step 5: 运行测试确认失败**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: FAIL（`schema/Role.h` 不存在，编译错误）。

- [ ] **Step 6: 写 `core/schema/Role.h`**

```cpp
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 消息角色枚举：与大模型沟通的基石
enum class Role {
    System,    // 系统提示词
    User,      // 用户输入 / 工具执行结果(Observation)
    Assistant  // 模型输出：推理或工具调用
};

std::string roleToString(Role role);
Role roleFromString(const std::string& str);

// nlohmann 自动枚举序列化：必须置于 qh::schema 命名空间内、枚举定义之后，
// 才能正确生成 ADL 的 to_json/from_json
NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::System, "system"},
    {Role::User, "user"},
    {Role::Assistant, "assistant"},
})

} // namespace schema
} // namespace qh
```

- [ ] **Step 7: 写 `core/schema/Role.cpp`**

```cpp
#include "Role.h"

namespace qh {
namespace schema {

std::string roleToString(Role role) {
    switch (role) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
    }
    return "user"; // 兜底
}

Role roleFromString(const std::string& str) {
    if (str == "system")    return Role::System;
    if (str == "assistant") return Role::Assistant;
    return Role::User;
}

} // namespace schema
} // namespace qh
```

- [ ] **Step 8: 运行测试确认通过**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: 末行 `... failures` 为 `0 failures`，退出码 0。

---

### Task 3: Schema - ToolCall

**Files:**
- Create: `core/schema/ToolCall.h`
- Create: `core/schema/ToolCall.cpp`
- Create: `core/tests/test_ToolCall.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::schema::ToolCall{ id, name, arguments }`，`arguments` 为 `std::string`（原始 JSON 文本），`nlohmann::json parseArguments() const` 按需解析；`to_json/from_json` 保证 `arguments` 字段在 JSON 中是原始对象（非二次转义字符串）。

- [ ] **Step 1: 写失败测试 `core/tests/test_ToolCall.cpp`**

```cpp
#include "TestHarness.h"
#include "schema/ToolCall.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(toolcall_roundtrip_raw_json) {
    qh::schema::ToolCall tc;
    tc.id = "call_1";
    tc.name = "bash";
    tc.arguments = R"({"command":"ls -la"})";

    json j = tc;                                          // to_json
    QH_CHECK_EQ(std::string(j["id"]), std::string("call_1"));
    QH_CHECK(j["arguments"].is_object());                 // 原始JSON对象，非字符串
    QH_CHECK_EQ(std::string(j["arguments"]["command"]), std::string("ls -la"));

    auto back = j.get<qh::schema::ToolCall>();            // from_json
    QH_CHECK_EQ(back.id, std::string("call_1"));
    QH_CHECK_EQ(back.name, std::string("bash"));
    QH_CHECK_EQ(back.arguments, std::string(R"({"command":"ls -la"})"));
}

QH_TEST(toolcall_parse_arguments) {
    qh::schema::ToolCall tc;
    tc.arguments = R"({"x":1,"y":[2,3]})";
    auto parsed = tc.parseArguments();
    QH_CHECK_EQ(parsed["x"].get<int>(), 1);
    QH_CHECK_EQ(parsed["y"][0].get<int>(), 2);
}
```

- [ ] **Step 2: 更新 `core/CMakeLists.txt`（追加两行）**

在 `add_library` 的源列表追加 `schema/ToolCall.cpp`；在 `add_executable(qharness_core_tests ...)` 的源列表追加 `tests/test_ToolCall.cpp`。即：

```cmake
add_library(qharness_core STATIC
    schema/Role.cpp
    schema/ToolCall.cpp
)
...
add_executable(qharness_core_tests
    tests/TestMain.cpp
    tests/test_Role.cpp
    tests/test_ToolCall.cpp
)
```

- [ ] **Step 3: 运行测试确认失败**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: FAIL（`schema/ToolCall.h` 不存在）。

- [ ] **Step 4: 写 `core/schema/ToolCall.h`**

```cpp
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 模型请求调用的具体工具
struct ToolCall {
    std::string id;          // 工具调用唯一 ID
    std::string name;        // 工具名（如 "bash"）
    std::string arguments;   // 原始 JSON 文本（延迟解析，解析责任交给具体工具）

    // 按需将原始 arguments 解析为结构化对象
    nlohmann::json parseArguments() const;
};

void to_json(nlohmann::json& j, const ToolCall& tc);
void from_json(const nlohmann::json& j, ToolCall& tc);

} // namespace schema
} // namespace qh
```

- [ ] **Step 5: 写 `core/schema/ToolCall.cpp`**

```cpp
#include "ToolCall.h"

namespace qh {
namespace schema {

nlohmann::json ToolCall::parseArguments() const {
    if (arguments.empty()) {
        return nlohmann::json::object();
    }
    try {
        return nlohmann::json::parse(arguments);
    } catch (...) {
        return nlohmann::json::object(); // 解析失败兜底为空对象
    }
}

void to_json(nlohmann::json& j, const ToolCall& tc) {
    j = nlohmann::json{{"id", tc.id}, {"name", tc.name}};
    // arguments 作为原始 JSON 值（对象/数组），而非被二次转义的字符串
    const std::string raw = tc.arguments.empty() ? std::string("{}") : tc.arguments;
    j["arguments"] = nlohmann::json::parse(raw); // 解析失败会抛异常——由调用方保证参数合法
}

void from_json(const nlohmann::json& j, ToolCall& tc) {
    j.at("id").get_to(tc.id);
    j.at("name").get_to(tc.name);
    if (j.contains("arguments") && !j["arguments"].is_null()) {
        tc.arguments = j["arguments"].dump(); // 保留原始文本
    } else {
        tc.arguments = "{}";
    }
}

} // namespace schema
} // namespace qh
```

- [ ] **Step 6: 运行测试确认通过**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: 0 failures。

---

### Task 4: Schema - ToolResult

**Files:**
- Create: `core/schema/ToolResult.h`
- Create: `core/schema/ToolResult.cpp`
- Create: `core/tests/test_ToolResult.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::schema::ToolResult{ toolCallId, output, isError }`。

- [ ] **Step 1: 写失败测试 `core/tests/test_ToolResult.cpp`**

```cpp
#include "TestHarness.h"
#include "schema/ToolResult.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(toolresult_roundtrip) {
    qh::schema::ToolResult tr;
    tr.toolCallId = "call_1";
    tr.output = "total 0";
    tr.isError = false;

    json j = tr;
    QH_CHECK_EQ(std::string(j["tool_call_id"]), std::string("call_1"));
    QH_CHECK_EQ(std::string(j["output"]), std::string("total 0"));
    QH_CHECK(j["is_error"].get<bool>() == false);

    auto back = j.get<qh::schema::ToolResult>();
    QH_CHECK_EQ(back.toolCallId, std::string("call_1"));
    QH_CHECK(back.isError == false);
}
```

- [ ] **Step 2: 更新 `core/CMakeLists.txt`**（源列表追加 `schema/ToolResult.cpp` 与 `tests/test_ToolResult.cpp`）

- [ ] **Step 3: 运行测试确认失败**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: FAIL（头文件不存在）。

- [ ] **Step 4: 写 `core/schema/ToolResult.h`**

```cpp
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 工具本地执行完毕返回的物理结果
struct ToolResult {
    std::string toolCallId;
    std::string output;   // 控制台输出或报错堆栈
    bool isError = false; // 是否失败，供错误自愈使用
};

void to_json(nlohmann::json& j, const ToolResult& tr);
void from_json(const nlohmann::json& j, ToolResult& tr);

} // namespace schema
} // namespace qh
```

- [ ] **Step 5: 写 `core/schema/ToolResult.cpp`**

```cpp
#include "ToolResult.h"

namespace qh {
namespace schema {

void to_json(nlohmann::json& j, const ToolResult& tr) {
    j = nlohmann::json{
        {"tool_call_id", tr.toolCallId},
        {"output", tr.output},
        {"is_error", tr.isError}};
}

void from_json(const nlohmann::json& j, ToolResult& tr) {
    j.at("tool_call_id").get_to(tr.toolCallId);
    j.at("output").get_to(tr.output);
    j.at("is_error").get_to(tr.isError);
}

} // namespace schema
} // namespace qh
```

- [ ] **Step 6: 运行测试确认通过**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: 0 failures。

---

### Task 5: Schema - ToolDefinition

**Files:**
- Create: `core/schema/ToolDefinition.h`
- Create: `core/schema/ToolDefinition.cpp`
- Create: `core/tests/test_ToolDefinition.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::schema::ToolDefinition{ name, description, inputSchema }`，`inputSchema` 为 `nlohmann::json`（对应 JSON Schema）。

- [ ] **Step 1: 写失败测试 `core/tests/test_ToolDefinition.cpp`**

```cpp
#include "TestHarness.h"
#include "schema/ToolDefinition.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(tooldefinition_roundtrip) {
    qh::schema::ToolDefinition td;
    td.name = "bash";
    td.description = "执行 shell 命令";
    td.inputSchema = json::parse(R"({"type":"object","properties":{"command":{"type":"string"}}})");

    json j = td;
    QH_CHECK_EQ(std::string(j["name"]), std::string("bash"));
    QH_CHECK_EQ(std::string(j["description"]), std::string("执行 shell 命令"));
    QH_CHECK(j["input_schema"]["type"].get<std::string>() == "object");

    auto back = j.get<qh::schema::ToolDefinition>();
    QH_CHECK_EQ(back.name, std::string("bash"));
    QH_CHECK(back.inputSchema["properties"]["command"]["type"].get<std::string>() == "string");
}
```

- [ ] **Step 2: 更新 `core/CMakeLists.txt`**（追加 `schema/ToolDefinition.cpp` 与 `tests/test_ToolDefinition.cpp`）

- [ ] **Step 3: 运行测试确认失败**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: FAIL。

- [ ] **Step 4: 写 `core/schema/ToolDefinition.h`**

```cpp
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 大模型可调用工具的元信息（供模型理解工具有何用途）
struct ToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json inputSchema; // 对应 JSON Schema
};

void to_json(nlohmann::json& j, const ToolDefinition& td);
void from_json(const nlohmann::json& j, ToolDefinition& td);

} // namespace schema
} // namespace qh
```

- [ ] **Step 5: 写 `core/schema/ToolDefinition.cpp`**

```cpp
#include "ToolDefinition.h"

namespace qh {
namespace schema {

void to_json(nlohmann::json& j, const ToolDefinition& td) {
    j = nlohmann::json{
        {"name", td.name},
        {"description", td.description},
        {"input_schema", td.inputSchema}};
}

void from_json(const nlohmann::json& j, ToolDefinition& td) {
    j.at("name").get_to(td.name);
    j.at("description").get_to(td.description);
    j.at("input_schema").get_to(td.inputSchema);
}

} // namespace schema
} // namespace qh
```

- [ ] **Step 6: 运行测试确认通过**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: 0 failures。

---

### Task 6: Schema - Message

**Files:**
- Create: `core/schema/Message.h`
- Create: `core/schema/Message.cpp`
- Create: `core/tests/test_Message.cpp`
- Modify: `core/CMakeLists.txt`

**Interfaces:**
- Produces: `qh::schema::Message{ role, content, toolCalls, toolCallId }`；`to_json` 对空 `toolCalls`/`toolCallId` 应用 omitempty（不输出该字段）；`toolCalls` 借助 `std::vector<ToolCall>` 的 nlohmann 自适应自动序列化。

- [ ] **Step 1: 写失败测试 `core/tests/test_Message.cpp`**

```cpp
#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(message_omitempty_plain) {
    qh::schema::Message m;
    m.role = qh::schema::Role::User;
    m.content = "hello";

    json j = m;
    QH_CHECK_EQ(std::string(j["role"]), std::string("user"));
    QH_CHECK_EQ(std::string(j["content"]), std::string("hello"));
    QH_CHECK(!j.contains("tool_calls"));   // omitempty
    QH_CHECK(!j.contains("tool_call_id")); // omitempty

    auto back = j.get<qh::schema::Message>();
    QH_CHECK(back.role == qh::schema::Role::User);
    QH_CHECK(back.toolCalls.empty());
}

QH_TEST(message_with_toolcalls) {
    qh::schema::Message m;
    m.role = qh::schema::Role::Assistant;
    m.content = "thinking...";
    qh::schema::ToolCall tc;
    tc.id = "call_1";
    tc.name = "bash";
    tc.arguments = R"({"command":"pwd"})";
    m.toolCalls.push_back(tc);

    json j = m;
    QH_CHECK(j.contains("tool_calls"));
    QH_CHECK_EQ(j["tool_calls"].size(), (size_t)1);
    QH_CHECK(j["tool_calls"][0]["arguments"].is_object());

    auto back = j.get<qh::schema::Message>();
    QH_CHECK_EQ(back.toolCalls.size(), (size_t)1);
    QH_CHECK_EQ(back.toolCalls[0].name, std::string("bash"));
}

QH_TEST(message_tool_response) {
    qh::schema::Message m;
    m.role = qh::schema::Role::User;
    m.content = "/home/user";
    m.toolCallId = "call_1";

    json j = m;
    QH_CHECK(j.contains("tool_call_id"));
    QH_CHECK_EQ(std::string(j["tool_call_id"]), std::string("call_1"));
}
```

- [ ] **Step 2: 更新 `core/CMakeLists.txt`**（追加 `schema/Message.cpp` 与 `tests/test_Message.cpp`）

- [ ] **Step 3: 运行测试确认失败**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: FAIL。

- [ ] **Step 4: 写 `core/schema/Message.h`**

```cpp
#pragma once
#include <string>
#include <vector>
#include "Role.h"
#include "ToolCall.h"
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 上下文中传递的单条消息
struct Message {
    Role role = Role::User;
    std::string content;                  // 纯文本内容

    std::vector<ToolCall> toolCalls;      // 模型请求的工具调用（支持多个）
    std::string toolCallId;               // 若为对工具调用的响应，填写关联 ID（空=非工具响应）
};

void to_json(nlohmann::json& j, const Message& m);
void from_json(const nlohmann::json& j, Message& m);

} // namespace schema
} // namespace qh
```

- [ ] **Step 5: 写 `core/schema/Message.cpp`**

```cpp
#include "Message.h"

namespace qh {
namespace schema {

void to_json(nlohmann::json& j, const Message& m) {
    j = nlohmann::json{{"role", m.role}, {"content", m.content}};
    if (!m.toolCalls.empty()) {
        j["tool_calls"] = m.toolCalls; // vector<ToolCall> 自动序列化
    }
    if (!m.toolCallId.empty()) {
        j["tool_call_id"] = m.toolCallId;
    }
}

void from_json(const nlohmann::json& j, Message& m) {
    j.at("role").get_to(m.role);
    j.at("content").get_to(m.content);
    if (j.contains("tool_calls")) {
        j.at("tool_calls").get_to(m.toolCalls);
    }
    if (j.contains("tool_call_id")) {
        j.at("tool_call_id").get_to(m.toolCallId);
    }
}

} // namespace schema
} // namespace qh
```

- [ ] **Step 6: 运行测试确认通过**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: 0 failures（此时全部 schema 测试通过）。

---

### Task 7: 抽象类骨架 + 可编译性检查

**目标**：创建全部抽象基类与派生骨架（header-only），并增加 `skeleton_compile_check.cpp` 验证头文件可被包含、派生类可实例化。

**Files:**
- Create: `core/engine/Engine.h`
- Create: `core/engine/EngineReActLoop.h`
- Create: `core/provider/Provider.h`
- Create: `core/provider/ProviderOpenAI.h`
- Create: `core/provider/ProviderClaude.h`
- Create: `core/tool/Tool.h`
- Create: `core/robot/Robot.h`
- Create: `core/robot/RobotFeishu.h`
- Create: `core/memory/Memory.h`
- Create: `core/memory/MemoryFile.h`
- Create: `core/tests/skeleton_compile_check.cpp`
- Modify: `core/CMakeLists.txt`、`core/tests/TestMain.cpp`

**Interfaces:**
- Consumes: `schema/Message.h`、`schema/ToolDefinition.h`、`schema/ToolCall.h`、`schema/ToolResult.h`
- Produces: 全部抽象基类（`Engine`/`Provider`/`Tool`/`Robot`/`Memory`）及派生骨架。

- [ ] **Step 1: 写 `core/engine/Engine.h`**

```cpp
#pragma once
#include <string>

namespace qh {
namespace engine {

// 运行过程类（抽象基类）
class Engine {
public:
    virtual ~Engine() = default;
    virtual void run(const std::string& userPrompt) = 0;
};

} // namespace engine
} // namespace qh
```

- [ ] **Step 2: 写 `core/engine/EngineReActLoop.h`**

```cpp
#pragma once
#include "engine/Engine.h"
#include <string>

namespace qh {
namespace engine {

// 典型 ReAct 循环引擎：思考(Reasoning)→行动(Action)→观察(Observation)
class EngineReActLoop : public Engine {
public:
    void run(const std::string& userPrompt) override {
        // TODO: 后续步骤实现完整 ReAct 循环
        (void)userPrompt;
    }
};

} // namespace engine
} // namespace qh
```

- [ ] **Step 3: 写 `core/provider/Provider.h`**

```cpp
#pragma once
#include <vector>
#include "schema/Message.h"
#include "schema/ToolDefinition.h"

namespace qh {
namespace provider {

// 大模型连接抽象基类
class Provider {
public:
    virtual ~Provider() = default;
    virtual schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) = 0;
};

} // namespace provider
} // namespace qh
```

- [ ] **Step 4: 写 `core/provider/ProviderOpenAI.h`**

```cpp
#pragma once
#include "provider/Provider.h"
#include <string>

namespace qh {
namespace provider {

// OpenAI 格式大模型连接（骨架，HTTP 实现待后续步骤）
class ProviderOpenAI : public Provider {
public:
    ProviderOpenAI(std::string apiKey, std::string baseUrl, std::string model)
        : apiKey_(std::move(apiKey)),
          baseUrl_(std::move(baseUrl)),
          model_(std::move(model)) {}

    schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override {
        // TODO: 基于 httplib 实现 OpenAI 格式请求与响应解析
        (void)messages;
        (void)tools;
        return schema::Message{};
    }

private:
    std::string apiKey_;
    std::string baseUrl_;
    std::string model_;
};

} // namespace provider
} // namespace qh
```

- [ ] **Step 5: 写 `core/provider/ProviderClaude.h`**

```cpp
#pragma once
#include "provider/Provider.h"
#include <string>

namespace qh {
namespace provider {

// Claude 格式大模型连接（骨架）
class ProviderClaude : public Provider {
public:
    ProviderClaude(std::string apiKey, std::string baseUrl, std::string model)
        : apiKey_(std::move(apiKey)),
          baseUrl_(std::move(baseUrl)),
          model_(std::move(model)) {}

    schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override {
        // TODO: 基于 httplib 实现 Claude 格式请求与响应解析
        (void)messages;
        (void)tools;
        return schema::Message{};
    }

private:
    std::string apiKey_;
    std::string baseUrl_;
    std::string model_;
};

} // namespace provider
} // namespace qh
```

- [ ] **Step 6: 写 `core/tool/Tool.h`**

```cpp
#pragma once
#include "schema/ToolDefinition.h"
#include "schema/ToolCall.h"
#include "schema/ToolResult.h"

namespace qh {
namespace tool {

// 工具抽象基类（edit/read/bash 等工具的基类）
class Tool {
public:
    virtual ~Tool() = default;
    virtual schema::ToolDefinition definition() const = 0;
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;
};

} // namespace tool
} // namespace qh
```

- [ ] **Step 7: 写 `core/robot/Robot.h` 与 `core/robot/RobotFeishu.h`**

`core/robot/Robot.h`：

```cpp
#pragma once
#include <string>

namespace qh {
namespace robot {

// 外部机器人连接抽象基类
class Robot {
public:
    virtual ~Robot() = default;
    virtual void send(const std::string& message) = 0;
};

} // namespace robot
} // namespace qh
```

`core/robot/RobotFeishu.h`：

```cpp
#pragma once
#include "robot/Robot.h"
#include <string>

namespace qh {
namespace robot {

// 飞书机器人连接（骨架）
class RobotFeishu : public Robot {
public:
    explicit RobotFeishu(std::string webhook) : webhook_(std::move(webhook)) {}

    void send(const std::string& message) override {
        // TODO: 基于 httplib 向飞书 webhook 推送
        (void)message;
    }

private:
    std::string webhook_;
};

} // namespace robot
} // namespace qh
```

- [ ] **Step 8: 写 `core/memory/Memory.h` 与 `core/memory/MemoryFile.h`**

`core/memory/Memory.h`：

```cpp
#pragma once

namespace qh {
namespace memory {

// 记忆系统抽象基类
class Memory {
public:
    virtual ~Memory() = default;
    virtual void load() = 0;
    virtual void save() = 0;
};

} // namespace memory
} // namespace qh
```

`core/memory/MemoryFile.h`：

```cpp
#pragma once
#include "memory/Memory.h"
#include <string>

namespace qh {
namespace memory {

// 基于文件的记忆系统（骨架）
class MemoryFile : public Memory {
public:
    explicit MemoryFile(std::string basePath) : basePath_(std::move(basePath)) {}

    void load() override {
        // TODO: 从 basePath 读取记忆
    }
    void save() override {
        // TODO: 将记忆写入 basePath
    }

private:
    std::string basePath_;
};

} // namespace memory
} // namespace qh
```

- [ ] **Step 9: 写 `core/tests/skeleton_compile_check.cpp`**

```cpp
#include "engine/Engine.h"
#include "engine/EngineReActLoop.h"
#include "provider/Provider.h"
#include "provider/ProviderOpenAI.h"
#include "provider/ProviderClaude.h"
#include "tool/Tool.h"
#include "robot/Robot.h"
#include "robot/RobotFeishu.h"
#include "memory/Memory.h"
#include "memory/MemoryFile.h"
#include "schema/Message.h"
#include "schema/ToolDefinition.h"

namespace qh {
namespace checks {

// 强制实例化各派生骨架，验证接口可被实现、可被使用（编译期 + 链接期检查）
void touchSkeletons() {
    engine::EngineReActLoop e;
    e.run("hi");

    provider::ProviderOpenAI po("key", "https://api.example.com", "gpt-x");
    provider::ProviderClaude pc("key", "https://api.example.com", "claude-x");
    (void)po;
    (void)pc;

    memory::MemoryFile mf("./mem");
    mf.load();
    mf.save();

    robot::RobotFeishu rf("https://open.feishu.cn/hook/x");
    rf.send("hi");
}

} // namespace checks
} // namespace qh
```

- [ ] **Step 10: 更新 `core/tests/TestMain.cpp`（加入 skeleton 调用）**

在 `main()` 的注册表循环之后、汇总之前插入：

```cpp
#include "TestHarness.h"
#include <iostream>

namespace qh { namespace checks { void touchSkeletons(); } } // 新增声明

int main() {
    std::cout << "=== QHarness Core Tests ===\n";
    for (auto& tc : qh::test::registry()) {
        std::cout << "[ " << tc.name << " ]\n";
        tc.fn();
    }
    qh::checks::touchSkeletons();
    std::cout << "Skeletons compile & link OK\n";
    std::cout << "\n" << qh::test::totalCount() << " checks, "
              << qh::test::failCount() << " failures\n";
    return qh::test::failCount() == 0 ? 0 : 1;
}
```

- [ ] **Step 11: 更新 `core/CMakeLists.txt`**（在测试可执行源列表追加 `tests/skeleton_compile_check.cpp`）

- [ ] **Step 12: 运行测试确认通过**

Run: `cmake --build build --config Release --target qharness_core_tests && build\core\Release\qharness_core_tests.exe`
Expected: 末行 `Skeletons compile & link OK`，0 failures。

---

### Task 8: 界面模块 - MainWindow + 日志/对话停靠窗口

**Files:**
- Modify: `app/main.cpp`（替换占位为 MainWindow 实例化）
- Create: `app/MainWindow.h`
- Create: `app/MainWindow.cpp`

**Interfaces:**
- Consumes: `QMainWindow`、`QDockWidget`、`QPlainTextEdit`、`QTextBrowser`、`QLineEdit`。
- Produces: `qh::app::MainWindow`，含日志/对话两个停靠窗口，可运行出 GUI。

- [ ] **Step 1: 写 `app/MainWindow.h`**

```cpp
#pragma once
#include <QMainWindow>

class QDockWidget;
class QPlainTextEdit;
class QTextBrowser;
class QLineEdit;

namespace qh {
namespace app {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onChatSend();

private:
    void appendLog(const QString& text);

    QDockWidget* logDock_;
    QDockWidget* chatDock_;
    QPlainTextEdit* logView_;
    QTextBrowser* chatView_;
    QLineEdit* chatInput_;
};

} // namespace app
} // namespace qh
```

- [ ] **Step 2: 写 `app/MainWindow.cpp`**

```cpp
#include "MainWindow.h"
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace qh {
namespace app {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("QHarnesscc");
    resize(900, 600);

    // 日志子窗口
    logDock_ = new QDockWidget(QStringLiteral("日志"), this);
    logView_ = new QPlainTextEdit(logDock_);
    logView_->setReadOnly(true);
    logDock_->setWidget(logView_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);

    // 对话子窗口（中央）
    chatDock_ = new QDockWidget(QStringLiteral("对话"), this);
    auto* center = new QWidget(chatDock_);
    auto* layout = new QVBoxLayout(center);
    chatView_ = new QTextBrowser(center);
    chatInput_ = new QLineEdit(center);
    chatInput_->setPlaceholderText(QStringLiteral("输入消息后回车发送..."));
    layout->addWidget(chatView_);
    layout->addWidget(chatInput_);
    chatDock_->setWidget(center);
    addDockWidget(Qt::TopDockWidgetArea, chatDock_);

    connect(chatInput_, &QLineEdit::returnPressed, this, &MainWindow::onChatSend);

    appendLog(QStringLiteral("[UI] 主窗口与日志/对话窗口已就绪。"));
}

void MainWindow::appendLog(const QString& text) {
    logView_->appendPlainText(text);
}

void MainWindow::onChatSend() {
    QString text = chatInput_->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    chatView_->append(QStringLiteral("<b>你:</b> ") + text.toHtmlEscaped());
    chatInput_->clear();
    appendLog(QStringLiteral("[Chat] 用户发送: ") + text);
    // TODO: 后续接入 Engine 进行 ReAct 推理
}

} // namespace app
} // namespace qh
```

- [ ] **Step 3: 替换 `app/main.cpp`**

```cpp
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    qh::app::MainWindow window;
    window.show();
    return app.exec();
}
```

- [ ] **Step 4: 更新 `app/CMakeLists.txt`**（加入 MainWindow，AUTOMOC 处理 Q_OBJECT）

```cmake
set(CMAKE_AUTOMOC ON)
add_executable(qharness_app WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
)
target_link_libraries(qharness_app PRIVATE qharness_core Qt5::Core Qt5::Widgets)
if(MSVC)
    target_compile_options(qharness_app PRIVATE /utf-8)
endif()
```

- [ ] **Step 5: 全量构建验证**

Run: `build_msvc.bat`（或 `cmake --build build --config Release`）
Expected: 构建成功，生成 `build/app/Release/qharness_app.exe`。

- [ ] **Step 6: 运行测试 + 启动 GUI 验证**

Run: `build\core\Release\qharness_core_tests.exe` → 0 failures；再运行 `build\app\Release\qharness_app.exe`
Expected: 弹出主窗口，含"日志"（底部）与"对话"（顶部）两个停靠窗口，日志区显示 `[UI] 主窗口与日志/对话窗口已就绪。`。

- [ ] **Step 7: 验证检查点**

确认全部 8 个 Task 的判据满足（见设计文档第 8 节）：核心测试通过、抽象骨架可编译、GUI 可运行出双窗口。本步完成。
