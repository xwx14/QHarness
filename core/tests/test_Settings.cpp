#include "TestHarness.h"
#include "schema/Settings.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(settings_empty_omitempty) {
    qh::schema::Settings s;
    json j = s;
    QH_CHECK(j.is_object());
    QH_CHECK(!j.contains("profiles"));
    QH_CHECK(!j.contains("activeProfileName"));
    QH_CHECK(!j.contains("enableThinking"));
    QH_CHECK(!j.contains("workDir"));
    QH_CHECK(!j.contains("enabledTools"));
    auto back = j.get<qh::schema::Settings>();
    QH_CHECK(back._profiles.empty());
    QH_CHECK(!back._enableThinking);
}

QH_TEST(settings_profile_roundtrip) {
    qh::schema::Settings s;
    s._enableThinking = true;
    s._workDir = "E:/work";
    s._enabledTools = {"bash"};
    qh::schema::LlmProfile p;
    p._name = "glm";
    p._providerType = qh::schema::ProviderType::OpenAI;
    p._baseUrl = "https://api.example.com";
    p._apiKey = "sk-xxx";
    p._model = "glm-4";
    p._temperature = 0.7;
    p._maxTokens = 4096;
    s._profiles.push_back(p);
    s._activeProfileName = "glm";

    json j = s;
    QH_CHECK(j.contains("profiles"));
    QH_CHECK_EQ(j["profiles"].size(), (size_t)1);
    QH_CHECK_EQ(std::string(j["profiles"][0]["name"]), std::string("glm"));
    QH_CHECK(j["profiles"][0]["providerType"] == qh::schema::ProviderType::OpenAI);
    QH_CHECK(j["profiles"][0].contains("temperature"));
    QH_CHECK_EQ(std::string(j["activeProfileName"]), std::string("glm"));
    QH_CHECK(j["enableThinking"] == true);

    auto back = j.get<qh::schema::Settings>();
    QH_CHECK_EQ(back._profiles.size(), (size_t)1);
    QH_CHECK_EQ(back._profiles[0]._name, std::string("glm"));
    QH_CHECK(back._profiles[0]._temperature.has_value());
    QH_CHECK(back._profiles[0]._providerType == qh::schema::ProviderType::OpenAI);
    QH_CHECK_EQ(back._activeProfileName, std::string("glm"));
    QH_CHECK(back._enableThinking);
    QH_CHECK_EQ(back._enabledTools.size(), (size_t)1);
}

QH_TEST(settings_profile_optional_omitempty) {
    qh::schema::LlmProfile p;   // _temperature/_maxTokens 均 nullopt
    p._name = "x";
    p._model = "m";
    json j = p;
    QH_CHECK(!j.contains("temperature"));
    QH_CHECK(!j.contains("maxTokens"));
}

QH_TEST(findActiveProfile_hit_miss_empty) {
    qh::schema::Settings s;
    QH_CHECK(qh::schema::findActiveProfile(s) == nullptr);   // _activeProfileName 空
    qh::schema::LlmProfile p; p._name = "glm";
    s._profiles.push_back(p);
    s._activeProfileName = "glm";
    QH_CHECK(qh::schema::findActiveProfile(s) != nullptr);
    QH_CHECK_EQ(qh::schema::findActiveProfile(s)->_name, std::string("glm"));
    s._activeProfileName = "missing";
    QH_CHECK(qh::schema::findActiveProfile(s) == nullptr);   // 未命中
}
