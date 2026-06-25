# ProviderOpenAI 与 ProviderClaude 实现设计

| 项 | 值 |
|---|---|
| 日期 | 2026-06-25 |
| 状态 | 待实现 |
| 模块 | `core/provider/` |
| 关联设计 | `docs/specs/2026-06-22-QHarnesscc框架与schema设计.md` |
| 参考实现 | `E:\MyHarness\ai-sdk-cpp`（OpenAI/Anthropic request_builder/response_parser/http_request_handler） |
| OpenSSL 源码 | `E:\MyHarness\openssl-4.0.1` |

---

## 1. 背景与目标

QHarnesscc 的 `ProviderOpenAI`/`ProviderClaude` 当前为骨架（`generate` 体仅 `return GenerateResult{}`）。本设计把它们实现为**可对接真实 OpenAI / Anthropic HTTP API 的 Provider**，支撑 `EngineReActLoop` 的工具调用循环。

Go 参考项目 `go-tiny-claw` 仅有 Provider 接口（`internal/provider/interface.go`），**无 OpenAI/Claude 具体实现**——故这两个类为 C++ 端**新增**，需直接对接厂商 API 格式。

**总目标**：
- 用 vendored 的 `third/httplib.h`（0.48.0）+ nlohmann/json + 自编译 OpenSSL 4.0.1 实现真实联网调用。
- 保持 `generate(cancel, messages, tools) -> GenerateResult{message, error}` 现有签名不变，Provider 语义仍是「单次 LLM 调用」（多步 ReAct 由 `Engine` 编排，不在此层）。
- 离线可测：本地 Mock HTTP server 驱动 TDD。

---

## 2. 关键决策与权衡

### 2.1 路径选择：C —— 参考 ai-sdk-cpp 精简自写

评估过三条路径：

| 路径 | 结论 |
|---|---|
| A. 直接复用 ai-sdk-cpp（适配层） | 功能最全（白得流式/重试），但需升 C++20、解决双 nlohmann 冲突、引入 submodule 重依赖，且**跳过 Provider 层的自实现**，违背项目「移植/学习架构」定位。否决。 |
| B. 纯手写（无参考） | 最纯粹的练习，但最易踩 OpenAI/Anthropic API 字段坑。否决。 |
| **C. 参考 ai-sdk-cpp 精简自写** ✅ | 把 ai-sdk-cpp 经验证的 request_builder/response_parser 作**权威格式参考**，在 QHarnesscc 内自写精简实现。保持 C++17/轻量/Provider 语义纯净/学习价值。**采纳。** |

### 2.2 OpenSSL 来源：源码编译 4.0.1

用户已下载 `openssl-4.0.1` 源码。采用源码编译（静态库），便于部署与版本锁定。
- **风险**：httplib 0.48 对 OpenSSL **4.x** 兼容性未经验证（4.0 为 2026-06 新 major）。列为「实现期验证清单」首项，回退方案见 §12。

### 2.3 测试方式：本地 Mock HTTP TDD

用 `httplib::Server` 起本地 **HTTP** server 返回预设 JSON，Provider 走 `Client`(HTTP) 分支，**Mock 测试完全不触碰 SSL**，离线可重复，纳入 `qharness_core_tests`。

### 2.4 功能范围：非流式 + 工具调用（YAGNI）

本次实现：非流式同步、工具调用、Provider 级 `temperature`/`maxTokens` 配置。
**不做**：流式 SSE、自动重试、多步工具循环（属 Engine 职责）、embeddings。

---

## 3. 架构设计

### 3.1 组件总览

```
core/provider/
├── Provider.h                 （抽象基类，不变）
├── ProviderHttp.h / .cpp       【新增】内部共用 HTTP 工具（非 QH_API 导出）
├── ProviderOpenAI.h / .cpp     【改】实现 generate + setter
├── ProviderClaude.h / .cpp     【改】实现 generate + setter
└── MockProvider.* / MockTwoStageProvider.* （不变）
```

`ProviderHttp` 消除两个 Provider 的 HTTP 重复代码（DRY）；请求构造 / 响应解析因厂商格式不同，各自实现。

### 3.2 ProviderHttp（内部共用 HTTP 工具）

**职责**：baseUrl 解析 + 同步 POST + 超时/取消，返回统一 `HttpResponse`。不依赖 Qt，**不导出**（`QH_API` 仅 core 公共类用），头文件**不 include httplib**（`#include <httplib.h>` 只在 `.cpp`，符合 CLAUDE.md 坑点）。

```cpp
// core/provider/ProviderHttp.h
#ifndef QH_PROVIDER_HTTP_H
#define QH_PROVIDER_HTTP_H
#include <string>
#include <vector>
#include <utility>
#include "schema/CancellationToken.h"

namespace qh {
namespace provider {

struct HttpResponse {
    bool ok = false;         // 是否拿到 HTTP 响应（false=网络错误/取消/超时）
    int status = 0;          // HTTP 状态码（ok=true 时有效）
    std::string body;        // 响应体
    std::string error;       // ok=false 时的错误描述
};

class ProviderHttp {
public:
    // baseUrl 形如 "https://api.openai.com/v1"；path 形如 "/chat/completions"
    // headers 为 (key,value) 列表（避免头文件依赖 httplib::Headers）
    static HttpResponse post(
        const std::string& baseUrl,
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& headers,
        const std::string& body,
        const schema::CancellationToken& cancel);
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_HTTP_H
```

**实现要点**（参考 ai-sdk-cpp `http_request_handler.cpp`）：
1. **baseUrl 解析**：`https://` → `use_ssl=true`，`http://` → `use_ssl=false`；去协议前缀后，第一个 `/` 之前为 `host`，之后为 `base_path`；`full_path = base_path + path`。
2. **客户端选择**：`use_ssl ? httplib::SSLClient(host) : httplib::Client(host)`。
3. **超时**：`set_connection_timeout(30, 0)` / `set_read_timeout(120, 0)`；SSL 可关证书验证（`enable_server_certificate_verification(false)`，降低本地/自签环境门槛，生产可改回 true）。
4. **取消**：Post 前、Post 后各检查一次 `cancel.isCancelled()`；命中则 `ok=false, error="cancelled"`。httplib 同步 Post 不可中断，`read_timeout` 兜底卡死。
5. **结果映射**：`if(!res)` → `ok=false, error="网络错误"`；否则 `ok=true, status=res->status, body=res->body`。HTTP 错误码（非 200）不在此层判定，交由 Provider 解析错误体。

### 3.3 ProviderOpenAI / ProviderClaude

各自三个私有方法 + `generate`：

```cpp
GenerateResult generate(cancel, messages, tools) override {
    // 数据流见 §4
}
private:
    nlohmann::json buildXxxRequest(messages, tools);   // 内部 Message → 厂商请求 JSON
    schema::Message parseXxxResponse(const std::string& body);  // 厂商响应 → 内部 Message
    std::string parseXxxError(int status, const std::string& body);  // 提取 error.message
```

新增配置成员（setter，构造保持 `(apiKey, baseUrl, model)` 不变）：
```cpp
void setTemperature(double t);     // optional<double> _temperature（不设则请求不带该字段）
void setMaxTokens(int n);          // optional<int> _maxTokens
```

---

## 4. 数据流

```
ProviderOpenAI::generate(cancel, messages, tools):
  1. 若 cancel.isCancelled() → return {error:"cancelled"}
  2. body  = buildOpenaiRequest(messages, tools)            // nlohmann 组装
  3. headers = {{"Authorization","Bearer "+_apiKey}}
  4. info("→ POST /v1/chat/completions  model=" + _model)
  5. resp  = ProviderHttp::post(_baseUrl, "/chat/completions", headers, body, cancel)
  6. 若 cancel.isCancelled() → return {error:"cancelled"}
  7. 若 !resp.ok            → error(resp.error); return {error:resp.error}
  8. 若 resp.status != 200  → msg=parseOpenaiError(resp.status, resp.body);
                              error(msg); return {error:msg}
  9. msg   = parseOpenaiResponse(resp.body)                 // → Message(Assistant)
 10. 若 !msg._content.empty() → chat(msg._content)
     若 !msg._toolCalls.empty() → think("工具调用 N 个: ...")
 11. return {message:msg, error:""}

ProviderClaude::generate 同构，差异：
  - path = "/messages"
  - headers = {{"x-api-key",_apiKey}, {"anthropic-version","2023-06-01"}}
  - build/parse 用 Claude 格式（§5）
```

---

## 5. schema::Message ↔ 厂商格式映射

`schema::Message` 为扁平结构：`_role / _content / _toolCalls / _toolCallId`。约定 **`_toolCallId` 非空 ⇒ 该消息是「工具结果消息」**（role 通常为 User，承载 ToolResult 输出）。`ToolCall._arguments` 是**原始 JSON 文本**（对齐 Go `json.RawMessage`）。

### 5.1 内部 Message → 厂商请求

| 内部 Message 形态 | OpenAI 请求 | Anthropic 请求 |
|---|---|---|
| `_role=System` | messages 首条 `{role:"system", content:_content}` | 顶层 `system` 字段（从 messages 抽出，不进 messages 数组） |
| `_role=User`, `_toolCallId` 空 | `{role:"user", content:_content}` | `{role:"user", content:_content}`（字符串） |
| `_role=User`, `_toolCallId` 非空（工具结果） | 独立 `{role:"tool", tool_call_id:_toolCallId, content:_content}` | `{role:"user", content:[{type:"tool_result", tool_use_id:_toolCallId, content:_content}]}` |
| `_role=Assistant` | `{role:"assistant", content:_content, tool_calls:[{id, type:"function", function:{name, arguments}}]}` | `{role:"assistant", content:[{type:"text", text:_content}?, {type:"tool_use", id, name, input}...]}` |

**工具定义**（`ToolDefinition{_name,_description,_inputSchema}`）：
- OpenAI：`tools:[{type:"function", function:{name, description, parameters:_inputSchema}}]`
- Anthropic：`tools:[{name, description, input_schema:_inputSchema}]`
- 仅当 `tools` 非空时附 `tools` 字段；本次固定 `tool_choice` 为 auto（OpenAI `"auto"`，Anthropic `{type:"auto"}`），不暴露切换。

**Assistant 工具调用的 arguments 字段**：
- OpenAI：`function.arguments` 必须是 **JSON 字符串** → `ToolCall._arguments` 直接作为字符串填入（它本就是原始文本）。
- Anthropic：`tool_use.input` 必须是 **JSON 对象** → 由 `_arguments` 解析（`parseArguments()`）后填入。

**Anthropic 必填项**：`max_tokens` 必填 → 默认 4096（`_maxTokens` 未设时），`setMaxTokens` 可覆盖。

### 5.2 厂商响应 → 内部 Message（role 固定 Assistant）

**OpenAI**：
- `choices[0].message.content`（可能为 null，如纯工具调用时）→ `_content`（null 视为空串）
- `choices[0].message.tool_calls[]`：`{id, function:{name, arguments(字符串)}}` → `_toolCalls`；`_arguments` 直接取字符串（契合 schema 原始文本语义）
- `choices[0].finish_reason`：`tool_calls`⇒有工具调用，`stop`⇒结束（仅用于日志，不改变 Message 结构）

**Anthropic**：
- `content[]` 数组遍历：`type:"text"` block 的 `text` 拼接 → `_content`；`type:"tool_use"` block 的 `{id, name, input(对象)}` → `_toolCalls`，`_arguments = input.dump()`（对象转原始文本）
- `stop_reason`：`tool_use`⇒有工具调用，`end_turn`⇒结束（仅日志）

### 5.3 `_arguments` 语义一致性

无论 OpenAI（字符串）还是 Anthropic（对象→dump），最终 `ToolCall._arguments` 都是**原始 JSON 文本**，与 `schema::Message.cpp` 的 `from_json`（`tc._arguments = j["arguments"].dump()`）及 `parseArguments()` 容错语义完全一致——Engine/Tool 侧无需感知厂商差异。

---

## 6. Provider 配置

| 项 | OpenAI | Anthropic |
|---|---|---|
| 默认 baseUrl | `https://api.openai.com/v1` | `https://api.anthropic.com/v1` |
| endpoint path | `/chat/completions` | `/messages` |
| 认证 header | `Authorization: Bearer <key>` | `x-api-key: <key>` |
| 额外 header | — | `anthropic-version: 2023-06-01` |
| `_maxTokens` 未设 | 请求不带该字段 | 必填，默认 4096（映射 `max_tokens`） |
| `_temperature` 未设 | 请求不带 | 请求不带 |
| `Content-Type` | `application/json`（经 `Post` 的 content_type 参数传，不进 headers） | 同左 |

`baseUrl` 约定**含 `/v1`**（由调用方/Engine 构造时提供），endpoint path 仅含 `/chat/completions` 或 `/messages`，`ProviderHttp` 拼成 `/v1/chat/completions`。

---

## 7. 取消 / 超时 / 错误 / PostMessage

- **取消**：协作式（Post 前后检查 `cancel.isCancelled()`）+ `read_timeout=120s` 兜底。
- **超时**：connect 30s / read 120s，由 `ProviderHttp` 统一设置。
- **错误处理**（统一填 `GenerateResult.error` 非空，不抛异常）：
  - 取消 → `"cancelled"`
  - 网络错误（`!res`）→ `"网络错误: <httplib Error>`"
  - HTTP ≠ 200 → `parseXxxError` 提取 `error.message`（OpenAI/Anthropic 错误体均为 `{error:{message,...}}`），无则 `body` 前 200 字符 + status
  - JSON 解析失败 / 必要字段缺失 → `"响应解析失败: <detail>"`
- **PostMessage**（Provider 已继承 `PostMessageInterface`）：请求前 `info`、响应文本 `chat`、工具调用 `think`、错误 `error`。

---

## 8. OpenSSL 4.0.1 编译与 CMake 集成

### 8.1 编译（Perl + MSVC，静态库便于部署）

```bat
cd E:\MyHarness\openssl-4.0.1
perl Configure VC-WIN64A --prefix=C:\OpenSSL-4.0.1-x64 no-shared
nmake
nmake install
```
- `no-shared` 产出静态 `libcrypto.lib` / `libssl.lib`，静态链入 `qharness_core.dll`，无需额外部署 OpenSSL DLL。
- 需 Perl（Strawberry Perl）。`VC-WIN64A` 不强制 nasm。

### 8.2 CMake（`core/CMakeLists.txt` 改动）

```cmake
# OpenSSL（SSLClient 依赖）
set(OPENSSL_ROOT_DIR "C:/OpenSSL-4.0.1-x64" CACHE PATH "OpenSSL 安装根")
find_package(OpenSSL REQUIRED)

add_library(qharness_core SHARED
    ...现有源...
    provider/ProviderHttp.h   provider/ProviderHttp.cpp        # 新增
)

target_link_libraries(qharness_core PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(qharness_core PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
```
- `CPPHTTPLIB_OPENSSL_SUPPORT` 使 `httplib::SSLClient` 可用。
- httplib 仅在 `.cpp` 内 `#include <httplib.h>`（`ProviderHttp.cpp`、两个 Provider 的 `.cpp`）。

---

## 9. 测试设计（本地 Mock HTTP TDD）

### 9.1 Mock server 辅助

新增 `core/tests/MockHttpServer.h`：封装 `httplib::Server`，构造时选空闲端口、后台线程运行，注册路径 handler 返回预设 JSON，析构 `server.stop()`。提供简单接口：
```cpp
class MockHttpServer {
public:
    MockHttpServer();                                   // 选端口、起线程
    ~MockHttpServer();                                  // 停服
    std::string baseUrl() const;                        // "http://localhost:<port>/v1"
    void onPost(const std::string& path,                // 注册 path 的 POST 响应
                int status, const std::string& body);
};
```

### 9.2 测试用例

新增 `tests/test_ProviderOpenAI.cpp` / `tests/test_ProviderClaude.cpp`，登记到 `qharness_core_tests` 源列表。Provider 构造 `baseUrl = mock.baseUrl()`（纯 HTTP），**不触发 SSL 分支**，离线可跑。

| 用例 | 验证点 |
|---|---|
| 纯文本响应 | mock 返回 `choices[0].message.content` / `content:[{text}]` → `_content` 正确 |
| 工具调用响应 | mock 返回 `tool_calls` / `tool_use` block → `_toolCalls` 的 id/name/arguments 正确 |
| 请求构造（双向） | mock handler 捕获实际收到的 body，断言 messages/tools 格式正确 |
| HTTP 401 / 500 | mock 返回错误体 → `GenerateResult.error` 非空且含 message |
| 取消 | 预先 `cancel.cancel()` → 返回 `error:"cancelled"`，不发请求 |
| Claude 工具结果回传 | 构造含工具结果消息的 messages → 请求体含 `tool_result` block |

`MockHttpServer.h` 亦登记入 `qharness_core_tests` 源列表（CLAUDE.md：所有 .h 列入）。

---

## 10. 文件清单与改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `core/provider/ProviderHttp.h` | 新增 | `HttpResponse` + `ProviderHttp::post` 声明 |
| `core/provider/ProviderHttp.cpp` | 新增 | baseUrl 解析 + httplib 同步 Post + 超时/取消 |
| `core/provider/ProviderOpenAI.h` | 改 | 加 `_temperature`/`_maxTokens` 成员 + setter 声明 |
| `core/provider/ProviderOpenAI.cpp` | 改 | 实现 `generate` + `buildOpenaiRequest`/`parseOpenaiResponse`/`parseOpenaiError` |
| `core/provider/ProviderClaude.h` | 改 | 同上 |
| `core/provider/ProviderClaude.cpp` | 改 | 同上（Claude 格式） |
| `core/CMakeLists.txt` | 改 | OpenSSL find_package/link/define；新增 ProviderHttp 源；新增测试源 |
| `core/tests/MockHttpServer.h` | 新增 | httplib::Server 封装 |
| `core/tests/test_ProviderOpenAI.cpp` | 新增 | Mock 驱动测试 |
| `core/tests/test_ProviderClaude.cpp` | 新增 | Mock 驱动测试 |

---

## 11. 非目标（YAGNI）

本次**不做**，留待后续按需：
- 流式 SSE（与现有同步 `generate` 签名冲突，需新接口）
- 自动重试 / 指数退避
- 多步工具调用循环（Engine 职责）
- embeddings、image generation
- `tool_choice` 切换（固定 auto）
- OpenAI reasoning（o 系列）/ Anthropic thinking 的原始思考链透传

---

## 12. 实现期验证清单（风险）

1. **【高风险】httplib 0.48 × OpenSSL 4.0.1 兼容性**：先编译 OpenSSL，写最小片段 `httplib::SSLClient` 验证编译/链接。若失败，回退顺序：① httplib 小补丁适配 4.x；② 临时降级 httplib；③ 改用 vcpkg OpenSSL 3.x。
2. **MSVC `/utf-8` + httplib**：确认 httplib 在 `/utf-8 /W3` 下无新警告/错误。
3. **Mock server 端口竞争**：`MockHttpServer` 动态选端口（bind 0），避免固定端口冲突。
4. **httplib 多线程**：`MockHttpServer` 用独立线程跑 `server.listen()`，测试主线程阻塞等待 server 就绪后再发起请求。
5. **`SSLClient` 证书验证**：默认关（`enable_server_certificate_verification(false)`）以适配多样环境；生产可经配置开启。

---

## 13. 参考资料

- 厂商 API：OpenAI Chat Completions `/v1/chat/completions`；Anthropic Messages `/v1/messages`。
- 格式映射蓝本：`ai-sdk-cpp/src/providers/openai/{openai_request_builder,openai_response_parser}.cpp`、`ai-sdk-cpp/src/providers/anthropic/{anthropic_request_builder,anthropic_response_parser}.cpp`、`ai-sdk-cpp/src/http/http_request_handler.cpp`。
- httplib 0.48：`third/httplib.h`（`Client`/`SSLClient`、`Post(path,headers,body,content_type)`、`set_connection_timeout`/`set_read_timeout`）。
- 项目 schema：`core/schema/Message.h`（`Message`/`ToolCall`/`ToolDefinition`）、`core/schema/CancellationToken.h`、`core/schema/PostMessage.h`。
- 抽象基类：`core/provider/Provider.h`（`generate` 签名、`GenerateResult`）。
