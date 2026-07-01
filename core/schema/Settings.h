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

// 单个模型（隶属于某供应商）：模型名 + 该模型自己的采样参数
class QH_API LlmModel {
public:
    std::string             _name;          // 模型名，如 "glm-4"
    std::optional<double>   _temperature;   // nullopt = 不下发，用 provider 默认
    std::optional<int>      _maxTokens;
};

QH_API void to_json(nlohmann::json& j, const LlmModel& m);
QH_API void from_json(const nlohmann::json& j, LlmModel& m);

// 供应商：一组 baseUrl + apiKey + providerType，下挂多个模型
class QH_API LlmProvider {
public:
    std::string             _name;                              // 供应商名，唯一标识
    ProviderType            _providerType = ProviderType::OpenAI;
    std::string             _baseUrl;
    std::string             _apiKey;
    std::vector<LlmModel>   _models;                            // 该供应商下多个模型
};

QH_API void to_json(nlohmann::json& j, const LlmProvider& p);
QH_API void from_json(const nlohmann::json& j, LlmProvider& p);

// 应用全局设置（多供应商 + 双激活 + 引擎开关 + 工作目录 + 启用工具）
class QH_API Settings {
public:
    std::vector<LlmProvider> _providers;
    std::string              _activeProviderName;   // 当前供应商；空 = 未激活
    std::string              _activeModelName;      // 当前模型（在 _activeProviderName 下查找）；空 = 未激活
    bool                     _enableThinking = false;
    std::string              _workDir;              // 空 = 运行时回退 currentPath
    std::vector<std::string> _enabledTools;
    int                      _maxToolConcurrency = 0;   // 工具并发上限，0 = 无上限
};

QH_API void to_json(nlohmann::json& j, const Settings& s);
QH_API void from_json(const nlohmann::json& j, Settings& s);

// 按 _activeProviderName 查找供应商；未设置/未命中返回 nullptr
QH_API const LlmProvider* findActiveProvider(const Settings& s);
// 在 provider._models 中按 name 查找模型；空名/未命中返回 nullptr
QH_API const LlmModel* findModel(const LlmProvider& provider, const std::string& modelName);

} // namespace schema
} // namespace qh
#endif // QH_SCHEMA_SETTINGS_H
