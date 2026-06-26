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
    QH_CHECK(back.profiles.empty());
    QH_CHECK(!back.enableThinking);
}

QH_TEST(settings_profile_roundtrip) {
    qh::schema::Settings s;
    s.enableThinking = true;
    s.workDir = "E:/work";
    s.enabledTools = {"bash"};
    qh::schema::LlmProfile p;
    p.name = "glm";
    p.providerType = qh::schema::ProviderType::OpenAI;
    p.baseUrl = "https://api.example.com";
    p.apiKey = "sk-xxx";
    p.model = "glm-4";
    p.temperature = 0.7;
    p.maxTokens = 4096;
    s.profiles.push_back(p);
    s.activeProfileName = "glm";

    json j = s;
    QH_CHECK(j.contains("profiles"));
    QH_CHECK_EQ(j["profiles"].size(), (size_t)1);
    QH_CHECK_EQ(std::string(j["profiles"][0]["name"]), std::string("glm"));
    QH_CHECK(j["profiles"][0]["providerType"] == qh::schema::ProviderType::OpenAI);
    QH_CHECK(j["profiles"][0].contains("temperature"));
    QH_CHECK_EQ(std::string(j["activeProfileName"]), std::string("glm"));
    QH_CHECK(j["enableThinking"] == true);

    auto back = j.get<qh::schema::Settings>();
    QH_CHECK_EQ(back.profiles.size(), (size_t)1);
    QH_CHECK_EQ(back.profiles[0].name, std::string("glm"));
    QH_CHECK(back.profiles[0].temperature.has_value());
    QH_CHECK(back.profiles[0].providerType == qh::schema::ProviderType::OpenAI);
    QH_CHECK_EQ(back.activeProfileName, std::string("glm"));
    QH_CHECK(back.enableThinking);
    QH_CHECK_EQ(back.enabledTools.size(), (size_t)1);
}

QH_TEST(settings_profile_optional_omitempty) {
    qh::schema::LlmProfile p;   // temperature/maxTokens 均 nullopt
    p.name = "x";
    p.model = "m";
    json j = p;
    QH_CHECK(!j.contains("temperature"));
    QH_CHECK(!j.contains("maxTokens"));
}

QH_TEST(findActiveProfile_hit_miss_empty) {
    qh::schema::Settings s;
    QH_CHECK(qh::schema::findActiveProfile(s) == nullptr);   // active 空
    qh::schema::LlmProfile p; p.name = "glm";
    s.profiles.push_back(p);
    s.activeProfileName = "glm";
    QH_CHECK(qh::schema::findActiveProfile(s) != nullptr);
    QH_CHECK_EQ(qh::schema::findActiveProfile(s)->name, std::string("glm"));
    s.activeProfileName = "missing";
    QH_CHECK(qh::schema::findActiveProfile(s) == nullptr);   // 未命中
}
