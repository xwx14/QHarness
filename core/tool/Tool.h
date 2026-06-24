#ifndef QH_TOOL_H
#define QH_TOOL_H
#include "schema/Message.h"
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace tool {

// 工具抽象基类（edit/read/bash 等工具的基类）；继承 PostMessageInterface 获得实时消息注入
// ToolDefinition 作为成员，构造时构建（派生经 Tool(def) 注入），definition() 返回成员引用
class QH_API Tool : public schema::PostMessageInterface {
public:
    Tool() = default;
    explicit Tool(schema::ToolDefinition def) : _definition(std::move(def)) {}
    virtual ~Tool() = default;

    const schema::ToolDefinition& definition() const { return _definition; }
    virtual schema::ToolResult execute(const schema::ToolCall& call) = 0;

protected:
    schema::ToolDefinition _definition;
};

} // namespace tool
} // namespace qh
#endif // QH_TOOL_H
