#ifndef QH_TOOL_EDITFILE_H
#define QH_TOOL_EDITFILE_H
#include <string>
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 编辑文件工具（参考 Go EditFileTool）：对现有文件做局部字符串替换，
// 比整体重写更安全/快速。提供 path 与 old_text/new_text（均 UTF-8）。
// 多级模糊匹配（精确→换行归一化→trim 首尾空白→逐行去缩进）以容忍 AI 给出的
// 文本与文件实际内容的细微差异（CRLF/首尾空白/缩进）。
// 路径穿越防护用基类 resolveInside（要求文件已存在）；读出经 toUtf8 统一到 UTF-8
// 空间匹配（与 ReadFileTool 一致），写回 UTF-8（与 WriteFileTool 一致）。
class QH_API EditFileTool : public Tool {
public:
    explicit EditFileTool(std::string workDir);

    schema::ToolResult execute(const schema::ToolCall& call) override;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_EDITFILE_H
