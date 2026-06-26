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
        {"name", p.name},
        {"providerType", p.providerType},
        {"baseUrl", p.baseUrl},
        {"apiKey", p.apiKey},
        {"model", p.model}};
    if (p.temperature.has_value()) j["temperature"] = *p.temperature;
    if (p.maxTokens.has_value())   j["maxTokens"]   = *p.maxTokens;
}

void from_json(const nlohmann::json& j, LlmProfile& p) {
    j.at("name").get_to(p.name);
    j.at("providerType").get_to(p.providerType);
    j.at("baseUrl").get_to(p.baseUrl);
    j.at("apiKey").get_to(p.apiKey);
    j.at("model").get_to(p.model);
    if (j.contains("temperature")) p.temperature = j["temperature"].get<double>();
    if (j.contains("maxTokens"))   p.maxTokens   = j["maxTokens"].get<int>();
}

void to_json(nlohmann::json& j, const Settings& s) {
    j = nlohmann::json::object();
    if (!s.profiles.empty())          j["profiles"] = s.profiles;            // vector<LlmProfile> 自动序列化
    if (!s.activeProfileName.empty()) j["activeProfileName"] = s.activeProfileName;
    if (s.enableThinking)             j["enableThinking"] = s.enableThinking;
    if (!s.workDir.empty())           j["workDir"] = s.workDir;
    if (!s.enabledTools.empty())      j["enabledTools"] = s.enabledTools;
}

void from_json(const nlohmann::json& j, Settings& s) {
    if (j.contains("profiles"))          j.at("profiles").get_to(s.profiles);
    if (j.contains("activeProfileName")) j.at("activeProfileName").get_to(s.activeProfileName);
    if (j.contains("enableThinking"))    j.at("enableThinking").get_to(s.enableThinking);
    if (j.contains("workDir"))           j.at("workDir").get_to(s.workDir);
    if (j.contains("enabledTools"))      j.at("enabledTools").get_to(s.enabledTools);
}

const LlmProfile* findActiveProfile(const Settings& s) {
    if (s.activeProfileName.empty()) return nullptr;
    for (const auto& p : s.profiles) {
        if (p.name == s.activeProfileName) return &p;
    }
    return nullptr;
}

} // namespace schema
} // namespace qh
