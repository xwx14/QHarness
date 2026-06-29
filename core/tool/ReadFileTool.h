#ifndef QH_TOOL_READFILE_H
#define QH_TOOL_READFILE_H
#include <string>
#include "tool/Tool.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 读文件工具（参考 Go ReadFileTool）：definition 名称注册为 "read_file"，
// 限定只能读取注入的工作目录及其子目录（路径穿越防护），并对超长内容做字节截断保护。
class QH_API ReadFileTool : public Tool {
public:
    // 注入工作目录：工具仅允许在此目录及其子目录下读取
    explicit ReadFileTool(std::string workDir);

    schema::ToolResult execute(const schema::ToolCall& call) override;

    // 超长内容截断阈值（字节）：防止读取超大文件撑爆上下文
    static const std::size_t kMaxLen = 8000;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_READFILE_H
