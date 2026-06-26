#ifndef QH_SCHEMA_SETTINGS_H
#define QH_SCHEMA_SETTINGS_H
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "qh_export.h"

namespace qh {
namespace schema {

// 大模型服务类型（决定请求/响应格式：OpenAI Chat 还是 Anthropic Messages）
enum class ProviderType { OpenAI, Claude };

QH_API std::string providerTypeToString(ProviderType t);
QH_API ProviderType providerTypeFromString(const std::string& s);

// nlohmann 自动枚举序列化：必须置于 qh::schema 内、枚举定义之后
NLOHMANN_JSON_SERIALIZE_ENUM(ProviderType, {
    {ProviderType::OpenAI, "OpenAI"},
    {ProviderType::Claude, "Claude"},
})

// 单套 LLM 连接配置（一个 profile = 一组 baseUrl/apiKey/model 等）
class QH_API LlmProfile {
public:
    std::string  name;                                   // profile 唯一标识（用户命名）
    ProviderType providerType = ProviderType::OpenAI;
    std::string  baseUrl;
    std::string  apiKey;
    std::string  model;
    std::optional<double> temperature;                   // nullopt = 不下发，用 provider 默认
    std::optional<int>    maxTokens;
};

QH_API void to_json(nlohmann::json& j, const LlmProfile& p);
QH_API void from_json(const nlohmann::json& j, LlmProfile& p);

// 应用全局设置（多 profile + 引擎开关 + 工作目录 + 启用工具）
class QH_API Settings {
public:
    std::vector<LlmProfile>  profiles;
    std::string              activeProfileName;           // 按 name 引用激活项；空 = 未激活
    bool                     enableThinking = false;      // 两阶段引擎的慢思考 Phase1 开关
    std::string              workDir;                     // 空 = 运行时回退 currentPath
    std::vector<std::string> enabledTools;                // 启用的工具名子集（如 "bash"）
};

QH_API void to_json(nlohmann::json& j, const Settings& s);
QH_API void from_json(const nlohmann::json& j, Settings& s);

// 按 activeProfileName 查找激活 profile；未设置或未命中返回 nullptr
QH_API const LlmProfile* findActiveProfile(const Settings& s);

} // namespace schema
} // namespace qh
#endif // QH_SCHEMA_SETTINGS_H
