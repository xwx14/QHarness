#ifndef QH_TOOL_H
#define QH_TOOL_H
#include "schema/Message.h"
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 工具抽象基类（edit/read/bash 等工具的基类）；继承 PostMessageInterface 获得实时消息注入
class QH_API Tool : public schema::PostMessageInterface {
public:
    virtual ~Tool() = default;
    virtual schema::ToolDefinition definition() const = 0;
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_H
