# PostMessage 新增 Chat 与 Think 消息级别 设计

- 日期：2026-06-25
- 状态：已批准，待实现

## 1. 背景与目标

当前 `PostMessage` 仅有 `Info/Warn/Error` 三级，全部输出到日志窗口（`LogDock`）。本改动：

- **Chat 级别**：模型对用户的回复内容，输出到**对话窗口**（`ChatDock`）而非日志。
- **Think 级别**：模型内部思考 trace 的独立级别，**当前仍输出到日志窗口**，未来通过配置选项决定其去向（对话窗口 / 单独窗口 / 静默）。

## 2. 设计决策

### 2.1 级别扩展
`enum class Level { Info, Warn, Error, Chat, Think };`（`Chat`、`Think` 追加末尾，不破坏既有枚举值/序列化）。

### 2.2 显示格式（决策 A，采用推荐）
- **Chat** 路由到对话窗口，显示 `<b>AI:</b> ` + content，与现有用户消息 `<b>你:</b> text` 对齐。引擎只传纯 content，显示前缀由 `QPostMessage` 统一添加。
- **Info/Warn/Error/Think** 输出到日志窗口，显示 `levelToString(level) + " " + msg`（现状不变）。

### 2.3 Think 级别（决策 B，新增独立级别）
- 思考 trace 使用 `Think` 级别（不再是 `Info`）。
- **当前**路由到日志窗口（与 `Info` 同），显示 `[THINK] msg`。
- **未来扩展**：`QPostMessage` 的「级别 → view」路由集中在一处分支，后续加配置选项时只需改该路由。本次**不**实现配置（YAGNI）。

## 3. 改动点

### 3.1 `core/schema/PostMessage.h`
- `Level` 枚举追加 `Chat, Think`。
- `PostMessageInterface` 加便利方法（与 `info/warn/error` 并列）：
  ```cpp
  void chat(const std::string& msg)  { post(Level::Chat, msg); }
  void think(const std::string& msg) { post(Level::Think, msg); }
  ```

### 3.2 `core/schema/PostMessage.cpp`
`levelToString` 追加：
```cpp
case Level::Chat:  return "[CHAT]";
case Level::Think: return "[THINK]";
```

### 3.3 `app/QPostMessage.{h,cpp}`
- 构造签名改为：`QPostMessage(QPlainTextEdit* logView, QTextBrowser* chatView, QObject* parent = nullptr)`
- 成员：`QPlainTextEdit* _logView = nullptr;` `QTextBrowser* _chatView = nullptr;`
- `post` 内 `messagePosted` 的 lambda 按 level 路由（集中分支，便于未来加配置）：
  ```cpp
  if (level == qh::schema::Level::Chat) {
      if (_chatView) _chatView->append("<b>AI:</b> " + msg);
  } else {
      if (_logView) _logView->appendPlainText(
          QString::fromStdString(qh::schema::levelToString(level)) + " " + msg);
  }
  ```
- `.h` 前向声明 `class QTextBrowser;`，`.cpp` include `<QTextBrowser>`。

### 3.4 `app/MainWindow.cpp`
调整构造顺序：先建 `_logDock`、`_chatDock`，再 `_postMessage = new QPostMessage(_logDock->view(), _chatDock->view(), this);`（当前 `_postMessage` 在 `_chatDock` 之前创建，需调换）。

### 3.5 引擎侧（模型输出改级别）
- `core/engine/EngineReActLoop.cpp`：`info("模型: " + result.message._content)` → `chat(result.message._content)`
- `core/engine/Engine2StageReAct.cpp`：
  - Phase1 思考 `info("内部思考 Trace: " + thinkResult.message._content)` → `think(thinkResult.message._content)`
  - Phase2 模型回复 `info("模型: " + actionResult.message._content)` → `chat(actionResult.message._content)`

## 4. 测试设计

### 4.1 `test_PostMessage`
追加用例：`levelToString(Level::Chat) == "[CHAT]"`、`levelToString(Level::Think) == "[THINK]"`。

### 4.2 FakePostMessage 通用辅助（`test_EngineReActLoop` / `test_Engine2StageReAct`）
```cpp
bool hasLevelContaining(const FakePostMessage& pm, qh::schema::Level level, const std::string& sub) {
    for (size_t i = 0; i < pm._messages.size(); ++i) {
        if (pm._levels[i] == level && pm._messages[i].find(sub) != std::string::npos) return true;
    }
    return false;
}
```

### 4.3 `test_EngineReActLoop`
- 保留既有 Info 断言（"执行工具"、"任务完成"、`!hasAnyError`）。
- 加：模型回复走 Chat：`hasLevelContaining(pm, Level::Chat, "看到了文件")`（MockProvider 第2轮完成文本 `"我看到了文件列表，里面包含 main.go，任务完成！"`）。

### 4.4 `test_Engine2StageReAct`
- `thinking_on`：`hasLevelContaining(pm, Level::Think, "分析")`（思考 trace）、`hasLevelContaining(pm, Level::Chat, "看到了文件")`（模型回复）、保留"执行工具"Info、`!hasAnyError`。
- `thinking_off`：`!hasLevelContaining(pm, Level::Think, "分析")`（无思考）、`hasLevelContaining(pm, Level::Chat, "看到了文件")`、保留 Info 断言、`!hasAnyError`。

> MockTwoStageProvider 的思考文本为 `"让我先分析一下任务，规划执行步骤。"`，Phase2 完成文本为 `"我看到了文件列表，任务完成！"`。

## 5. 不改动范围
- `QPostMessage` 为 app 层（Qt），无单测；靠编译 + GUI 手动验证（对话窗口显示 AI 回复、日志窗口显示思考/日志）。
- **不**实现 Think 的配置选项（未来单独做）。

## 6. 错误处理
无新错误路径；沿用现有 post 转发（view 为空则不处理）。
