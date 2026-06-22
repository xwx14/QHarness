#ifndef QH_TOOL_H
#define QH_TOOL_H
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
#endif // QH_TOOL_H
