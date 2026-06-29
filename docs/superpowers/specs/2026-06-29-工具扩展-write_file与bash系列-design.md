# 工具扩展：WriteFileTool 与 Bash 系列 实现设计

| 项 | 值 |
|---|---|
| 日期 | 2026-06-29 |
| 状态 | 待实现 |
| 模块 | `core/tool/`、`app/`、`.claude/skills/` |
| 关联 | `ReadFileTool`、`Tool` 基类、`PathCodec.h`/`ContentCodec.h`、`SettingsDialog`、`EngineThread` |

---

## 1. 背景与目标

引擎已能读文件（`ReadFileTool`）。本设计扩展工具集：① 建立「QHarness 工具创建」skill，把 `ReadFileTool` 的全套模式沉淀为可复用 SOP；② 用该 SOP 实现写文件工具 `WriteFileTool` 与命令执行 `Bash` 系列（`WinBashTool`/`LinuxBashTool`/`MacBashTool` + `BashTool` 切换）。

## 2. 设计决策（已与用户对齐）

- **skill 形态**：SOP 步骤 + 可复制代码模板。
- **BashTool 平台切换**：编译期 `#ifdef` 选一（单平台二进制只含一个实现）。
- **命令执行机制**：平台原生 API（Windows `CreateProcess` / POSIX `fork`+`exec`），超时可控、可杀进程。
- **设置勾选**：只暴露 `BashTool`（`Win/Linux/MacBashTool` 是内部实现，不单独勾选）。

## 3. 阶段一：skill「QHarness 工具创建」

- 位置：`.claude/skills/create-tool/SKILL.md`，用 `writing-skills` 建立。
- 内容（7 步 SOP，每步附代码骨架）：
  1. 继承 `Tool`，构造设 `_workDir` + `_definition`（`_name`/`_description`/`_inputSchema` JSON Schema）。
  2. `execute` 模板：`parseArguments` 取参 → `resolveWorkBase()`/`resolveInside()` 路径安全 → 业务逻辑 → 按需 `contentCodec`/`pathCodec` → 截断（`truncateUtf8Safe`）。
  3. `SettingsDialog::kToolNames` 登记工具名（设置可选）。
  4. `EngineThread` 按名 `std::make_unique<...>(workDir)` 创建 + `registerTool`（对话可用，所有权入 `_tools`）。
  5. `core/tests/test_X.cpp` 写 `QH_TEST` 用例。
  6. `core/CMakeLists.txt` 登记 `.h`（源列表）与 `test_X.cpp`（`qharness_core_tests`）。
  7. `run_tests.bat` 回归全绿 + 关键约定（路径穿越、编码、截断、工作目录限定）。

## 4. Tool 基类扩展

写文件目标可能不存在，现有 `Tool::resolveInside` 要求 `exists`（为读）。新增：

```cpp
// 解析 relPath 到工作目录 base 内的可写路径：穿越检查（位于 base 内）但不要求已存在。
// 父目录允许不存在（由调用方按需 create_directories）。越界/解析失败返回 nullopt。
std::optional<std::filesystem::path> resolveInsideForWrite(const std::filesystem::path& base,
                                                           const std::string& relPath) const;
```

实现：复用 `pathCodec::candidatePaths` + `weakly_canonical` + `relative` 穿越检查，但**不查 `exists`**，取首个通过穿越检查的候选（写文件语义下，编码候选无需"命中已存在文件"，取 GBK/首个合法即可；为确定性，取首个能 `weakly_canonical` 成功且不越界的候选）。

## 5. WriteFileTool（`write_file`）

- 参数：`path`（string）、`content`（string）。
- 流程：解析 → `resolveWorkBase()` → `resolveInsideForWrite(base, path)`（穿越检查，不要求存在）→ `create_directories(full.parent_path())`（按需建子目录）→ 以 **UTF-8**（`std::ofstream` binary）写 `content` → 已存在则覆盖。
- 编码：`content` 来自 AI（UTF-8），直接写 UTF-8（与 CLAUDE.md「与 AI 交互统一 UTF-8」一致）。
- 错误：参数缺失、穿越越界、父目录创建失败、写入失败 → `isError=true`。
- 不截断（写入内容由模型决定，不限制；超长由模型上下文约束）。

## 6. Bash 系列（`bash`）

### 6.1 通用契约
- 参数：`command`（string）、可选 `timeout_ms`（默认 30000）。
- 行为：在 `_workDir` 下执行命令，捕获 stdout+stderr，返回（截断到 `kMaxLen` 的）输出 + 退出码；超时则杀进程并返回超时提示。
- 输出编码：Windows cmd 输出按本地代码页（GBK）→ `contentCodec::toUtf8`；Linux/Mac UTF-8 直通。
- `_definition._name = "bash"`（三平台工具统一名，`BashTool` 别名继承）。

### 6.2 WinBashTool（Windows，`CreateProcess`）
- `CreateProcess` 启动 `cmd.exe /c <command>`，`lpCurrentDirectory = workDir`，管道捕获 stdout/stderr。
- 超时：`WaitForSingleObject` + timeout，超时 `TerminateProcess`。
- 退出码：`GetExitCodeProcess`。
- 输出：读取管道 → `toUtf8`（GBK→UTF-8）→ 截断。

### 6.3 LinuxBashTool / MacBashTool（POSIX `fork`+`exec`）
- `pipe` + `fork`，子进程 `chdir(workDir)` + `execl("/bin/sh", "sh", "-c", command)`（Mac 同）。
- 父进程 `waitpid`；超时用 `alarm`/`SIGALRM` 或轮询 + `kill` 杀子进程。
- 退出码：`WEXITSTATUS`。
- 输出：读管道（UTF-8 直通）→ 截断。

### 6.4 BashTool（编译期别名）
```cpp
// core/tool/BashTool.h
#ifdef _WIN32
    #include "tool/WinBashTool.h"
    using BashTool = WinBashTool;
#elif defined(__APPLE__)
    #include "tool/MacBashTool.h"
    using BashTool = MacBashTool;
#elif defined(__linux__)
    #include "tool/LinuxBashTool.h"
    using BashTool = LinuxBashTool;
#endif
```

### 6.5 CMake 跨平台
`core/CMakeLists.txt` 按 `WIN32`/`APPLE`/`UNIX` 条件只编对应平台 `.cpp`（+ 三平台 `.h` 均登记）。当前环境仅 Windows 编译验证；Linux/Mac 靠代码审查。

## 7. 集成

- `SettingsDialog::kToolNames = { "bash", "read_file", "write_file" }`（用户面对这三个；BashTool 自动按平台选实现）。
- `EngineThread` 工具创建分支：
  ```cpp
  if (name == "bash")        t = std::make_unique<tool::BashTool>(workDir);
  else if (name == "read_file")  t = std::make_unique<tool::ReadFileTool>(workDir);
  else if (name == "write_file") t = std::make_unique<tool::WriteFileTool>(workDir);
  ```

## 8. 测试（TDD）

- **WriteFileTool**：新建文件、覆盖已存在、建子目录写、穿越拒绝（`../`）、参数缺失、内容正确（读回校验）。
- **WinBashTool**（Windows）：`echo hello` 输出正确 + 退出码 0；失败命令（`exit 7`）退出码 7；超时（`ping`/长命令）被杀返回超时提示；工作目录正确（`cd` 输出当前目录）；中文输出（`echo 测试`，GBK→UTF-8）。
- Linux/Mac BashTool：仅代码审查（环境无 Linux/Mac）。
- 每工具 `test_X.cpp` + CMakeLists 登记；`run_tests.bat` 全绿。

## 9. 边界与约束

- **安全**：命令执行本质信任模型（Harness 就是让 AI 动手）；工作目录限定（`CreateProcess`/`chdir`）+ 超时杀进程 + 输出截断是基本护栏，不做命令白名单（YAGNI，与 ReadFileTool 的穿越防护层级不同——bash 本就是任意命令）。
- **超时默认 30s**，可被 `timeout_ms` 参数覆盖。
- **编码**：Windows cmd 输出转 UTF-8（GBK/UTF-16 检测同 ContentCodec）；Linux/Mac 默认 UTF-8。
- **`resolveInsideForWrite`**：写文件不要求目标存在，但穿越检查照做（防 `write_file` 写到工作目录外）。
- **skill** 是 Claude Code skill（`.claude/skills/`），非项目代码；`docs/技术文档/` 不入库，skill 同理本地维护（或按团队约定入库，待定）。
