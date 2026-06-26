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

void to_json(nlohmann::json& j, const LlmModel& m) {
    j = nlohmann::json{{"name", m._name}};
    if (m._temperature.has_value()) j["temperature"] = *m._temperature;
    if (m._maxTokens.has_value())   j["maxTokens"]   = *m._maxTokens;
}

void from_json(const nlohmann::json& j, LlmModel& m) {
    m._name = j.value("name", "");
    if (j.contains("temperature")) m._temperature = j["temperature"].get<double>();
    if (j.contains("maxTokens"))   m._maxTokens   = j["maxTokens"].get<int>();
}

void to_json(nlohmann::json& j, const LlmProvider& p) {
    j = nlohmann::json{
        {"name", p._name},
        {"providerType", p._providerType},
        {"baseUrl", p._baseUrl},
        {"apiKey", p._apiKey},
        {"models", p._models}};   // vector<LlmModel> 自动序列化
}

void from_json(const nlohmann::json& j, LlmProvider& p) {
    p._name         = j.value("name", "");
    p._providerType = j.value("providerType", ProviderType::OpenAI);
    p._baseUrl      = j.value("baseUrl", "");
    p._apiKey       = j.value("apiKey", "");
    if (j.contains("models")) j.at("models").get_to(p._models);
}

void to_json(nlohmann::json& j, const Settings& s) {
    j = nlohmann::json::object();
    if (!s._providers.empty())          j["providers"] = s._providers;
    if (!s._activeProviderName.empty()) j["activeProviderName"] = s._activeProviderName;
    if (!s._activeModelName.empty())    j["activeModelName"] = s._activeModelName;
    if (s._enableThinking)              j["enableThinking"] = s._enableThinking;
    if (!s._workDir.empty())            j["workDir"] = s._workDir;
    if (!s._enabledTools.empty())       j["enabledTools"] = s._enabledTools;
}

void from_json(const nlohmann::json& j, Settings& s) {
    if (j.contains("providers"))          j.at("providers").get_to(s._providers);
    if (j.contains("activeProviderName")) j.at("activeProviderName").get_to(s._activeProviderName);
    if (j.contains("activeModelName"))    j.at("activeModelName").get_to(s._activeModelName);
    if (j.contains("enableThinking"))     j.at("enableThinking").get_to(s._enableThinking);
    if (j.contains("workDir"))            j.at("workDir").get_to(s._workDir);
    if (j.contains("enabledTools"))       j.at("enabledTools").get_to(s._enabledTools);
}

const LlmProvider* findActiveProvider(const Settings& s) {
    if (s._activeProviderName.empty()) return nullptr;
    for (const auto& p : s._providers) {
        if (p._name == s._activeProviderName) return &p;
    }
    return nullptr;
}

const LlmModel* findModel(const LlmProvider& provider, const std::string& modelName) {
    if (modelName.empty()) return nullptr;
    for (const auto& m : provider._models) {
        if (m._name == modelName) return &m;
    }
    return nullptr;
}

} // namespace schema
} // namespace qh
