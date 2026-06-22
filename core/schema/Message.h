#ifndef QH_SCHEMA_MESSAGE_H
#define QH_SCHEMA_MESSAGE_H
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
#endif // QH_SCHEMA_MESSAGE_H
