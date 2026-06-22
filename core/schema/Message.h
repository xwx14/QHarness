#ifndef QH_SCHEMA_MESSAGE_H
#define QH_SCHEMA_MESSAGE_H
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 消息角色枚举：与大模型沟通的基石
enum class Role {
    System,    // 系统提示词
    User,      // 用户输入 / 工具执行结果(Observation)
    Assistant  // 模型输出：推理或工具调用
};

std::string roleToString(Role role);
Role roleFromString(const std::string& str);

// nlohmann 自动枚举序列化：必须置于 qh::schema 命名空间内、枚举定义之后，
// 才能正确生成 ADL 的 to_json/from_json
NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::System, "system"},
    {Role::User, "user"},
    {Role::Assistant, "assistant"},
})

// 模型请求调用的具体工具
struct ToolCall {
    std::string id;          // 工具调用唯一 ID
    std::string name;        // 工具名（如 "bash"）
    std::string arguments;   // 原始 JSON 文本（延迟解析，解析责任交给具体工具）

    // 按需将原始 arguments 解析为结构化对象
    nlohmann::json parseArguments() const;
};

void to_json(nlohmann::json& j, const ToolCall& tc);
void from_json(const nlohmann::json& j, ToolCall& tc);

// 工具本地执行完毕返回的物理结果
struct ToolResult {
    std::string toolCallId;
    std::string output;   // 控制台输出或报错堆栈
    bool isError = false; // 是否失败，供错误自愈使用
};

void to_json(nlohmann::json& j, const ToolResult& tr);
void from_json(const nlohmann::json& j, ToolResult& tr);

// 大模型可调用工具的元信息（供模型理解工具有何用途）
struct ToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json inputSchema; // 对应 JSON Schema
};

void to_json(nlohmann::json& j, const ToolDefinition& td);
void from_json(const nlohmann::json& j, ToolDefinition& td);

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
