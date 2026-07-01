#ifndef QH_TOOL_WRITEFILE_H
#define QH_TOOL_WRITEFILE_H
#include <string>
#include <optional>
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 写文件工具：新建或覆盖写入。提供相对工作区的 path 与 content（content 为 UTF-8，直接写入）。
// 路径穿越防护用基类 resolveInsideForWrite（不要求目标存在）；按需创建父目录。
class QH_API WriteFileTool : public Tool {
public:
    explicit WriteFileTool(std::string workDir);

    schema::ToolResult execute(const schema::ToolCall& call) override;

    // 同一文件路径 → 同一资源键（executeAll 据此把同文件操作串行化，防 lost update）
    std::optional<std::string> resourceKey(const schema::ToolCall& call) const override;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_WRITEFILE_H
