#include "schema/Settings.h"

namespace qh {
namespace schema {

std::string providerTypeToString(ProviderType t) {
    switch (t) {
        case ProviderType::OpenAI: return "OpenAI";
        case ProviderType::Claude: return "Claude";
    }
    return "OpenAI";
}

ProviderType providerTypeFromString(const std::string& s) {
    if (s == "Claude") return ProviderType::Claude;
    return ProviderType::OpenAI;
}

void to_json(nlohmann::json& j, const LlmProfile& p) {
    j = nlohmann::json{
        {"name", p._name},
        {"providerType", p._providerType},
        {"baseUrl", p._baseUrl},
        {"apiKey", p._apiKey},
        {"model", p._model}};
    if (p._temperature.has_value()) j["temperature"] = *p._temperature;
    if (p._maxTokens.has_value())   j["maxTokens"]   = *p._maxTokens;
}

void from_json(const nlohmann::json& j, LlmProfile& p) {
    j.at("name").get_to(p._name);
    j.at("providerType").get_to(p._providerType);
    j.at("baseUrl").get_to(p._baseUrl);
    j.at("apiKey").get_to(p._apiKey);
    j.at("model").get_to(p._model);
    if (j.contains("temperature")) p._temperature = j["temperature"].get<double>();
    if (j.contains("maxTokens"))   p._maxTokens   = j["maxTokens"].get<int>();
}

void to_json(nlohmann::json& j, const Settings& s) {
    j = nlohmann::json::object();
    if (!s._profiles.empty())          j["profiles"] = s._profiles;            // vector<LlmProfile> 自动序列化
    if (!s._activeProfileName.empty()) j["activeProfileName"] = s._activeProfileName;
    if (s._enableThinking)             j["enableThinking"] = s._enableThinking;
    if (!s._workDir.empty())           j["workDir"] = s._workDir;
    if (!s._enabledTools.empty())      j["enabledTools"] = s._enabledTools;
}

void from_json(const nlohmann::json& j, Settings& s) {
    if (j.contains("profiles"))          j.at("profiles").get_to(s._profiles);
    if (j.contains("activeProfileName")) j.at("activeProfileName").get_to(s._activeProfileName);
    if (j.contains("enableThinking"))    j.at("enableThinking").get_to(s._enableThinking);
    if (j.contains("workDir"))           j.at("workDir").get_to(s._workDir);
    if (j.contains("enabledTools"))      j.at("enabledTools").get_to(s._enabledTools);
}

const LlmProfile* findActiveProfile(const Settings& s) {
    if (s._activeProfileName.empty()) return nullptr;
    for (const auto& p : s._profiles) {
        if (p._name == s._activeProfileName) return &p;
    }
    return nullptr;
}

} // namespace schema
} // namespace qh
