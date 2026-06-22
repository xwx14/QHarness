#ifndef QH_SCHEMA_ROLE_H
#define QH_SCHEMA_ROLE_H
#include <string>
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

} // namespace schema
} // namespace qh
#endif // QH_SCHEMA_ROLE_H
