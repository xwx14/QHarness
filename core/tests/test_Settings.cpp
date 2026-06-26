#include "TestHarness.h"
#include "schema/Settings.h"
#include "config/SettingsStore.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

using nlohmann::json;

QH_TEST(settings_empty_omitempty) {
    qh::schema::Settings s;
    json j = s;
    QH_CHECK(j.is_object());
    QH_CHECK(!j.contains("providers"));
    QH_CHECK(!j.contains("activeProviderName"));
    QH_CHECK(!j.contains("activeModelName"));
    QH_CHECK(!j.contains("enableThinking"));
    QH_CHECK(!j.contains("workDir"));
    QH_CHECK(!j.contains("enabledTools"));
    auto back = j.get<qh::schema::Settings>();
    QH_CHECK(back._providers.empty());
    QH_CHECK(!back._enableThinking);
}

QH_TEST(provider_multi_models_roundtrip) {
    qh::schema::Settings s;
    s._enableThinking = true;
    s._workDir = "E:/work";
    s._enabledTools = {"bash"};
    qh::schema::LlmProvider p;
    p._name = "智谱";
    p._providerType = qh::schema::ProviderType::OpenAI;
    p._baseUrl = "https://api.example.com";
    p._apiKey = "sk-xxx";
    qh::schema::LlmModel m1; m1._name = "glm-4"; m1._temperature = 0.7; m1._maxTokens = 4096;
    qh::schema::LlmModel m2; m2._name = "glm-4-flash";   // 无 temp/maxTokens
    p._models.push_back(m1);
    p._models.push_back(m2);
    s._providers.push_back(p);
    s._activeProviderName = "智谱";
    s._activeModelName = "glm-4";

    json j = s;
    QH_CHECK(j.contains("providers"));
    QH_CHECK_EQ(j["providers"].size(), (size_t)1);
    QH_CHECK_EQ(j["providers"][0]["models"].size(), (size_t)2);
    QH_CHECK(j["providers"][0]["models"][0].contains("temperature"));
    QH_CHECK(!j["providers"][0]["models"][1].contains("temperature"));
    QH_CHECK_EQ(std::string(j["activeProviderName"]), std::string("智谱"));
    QH_CHECK_EQ(std::string(j["activeModelName"]), std::string("glm-4"));

    auto back = j.get<qh::schema::Settings>();
    QH_CHECK_EQ(back._providers.size(), (size_t)1);
    QH_CHECK_EQ(back._providers[0]._models.size(), (size_t)2);
    QH_CHECK_EQ(back._providers[0]._models[0]._name, std::string("glm-4"));
    QH_CHECK(back._providers[0]._models[0]._temperature.has_value());
    QH_CHECK(!back._providers[0]._models[1]._temperature.has_value());
    QH_CHECK_EQ(back._activeProviderName, std::string("智谱"));
    QH_CHECK_EQ(back._activeModelName, std::string("glm-4"));
}

QH_TEST(model_optional_omitempty) {
    qh::schema::LlmModel m;   // temp/maxTokens 均 nullopt
    m._name = "x";
    json j = m;
    QH_CHECK(!j.contains("temperature"));
    QH_CHECK(!j.contains("maxTokens"));
}

QH_TEST(findActiveProvider_hit_miss_empty) {
    qh::schema::Settings s;
    QH_CHECK(qh::schema::findActiveProvider(s) == nullptr);   // 空名
    qh::schema::LlmProvider p; p._name = "智谱";
    s._providers.push_back(p);
    s._activeProviderName = "智谱";
    QH_CHECK(qh::schema::findActiveProvider(s) != nullptr);
    QH_CHECK_EQ(qh::schema::findActiveProvider(s)->_name, std::string("智谱"));
    s._activeProviderName = "missing";
    QH_CHECK(qh::schema::findActiveProvider(s) == nullptr);   // 未命中
}

QH_TEST(findModel_hit_miss_empty) {
    qh::schema::LlmProvider p;
    p._name = "智谱";
    qh::schema::LlmModel m; m._name = "glm-4";
    p._models.push_back(m);
    QH_CHECK(qh::schema::findModel(p, "") == nullptr);          // 空名
    QH_CHECK(qh::schema::findModel(p, "glm-4") != nullptr);
    QH_CHECK_EQ(qh::schema::findModel(p, "glm-4")->_name, std::string("glm-4"));
    QH_CHECK(qh::schema::findModel(p, "missing") == nullptr);   // 未命中
}

// ========== SettingsStore 持久化 ==========

QH_TEST(store_save_load_providers_roundtrip) {
    const std::string path = "test_setting_tmp.json";
    {
        qh::schema::Settings s;
        qh::schema::LlmProvider p; p._name = "a"; p._baseUrl = "u"; p._apiKey = "k";
        qh::schema::LlmModel m; m._name = "m1";
        p._models.push_back(m);
        s._providers.push_back(p);
        s._activeProviderName = "a";
        s._activeModelName = "m1";
        qh::config::SettingsStore store(path);
        QH_CHECK(store.save(s));
        QH_CHECK(store.lastError().empty());
    }
    qh::config::SettingsStore store(path);
    auto loaded = store.load();
    QH_CHECK_EQ(loaded._providers.size(), (size_t)1);
    QH_CHECK_EQ(loaded._providers[0]._models.size(), (size_t)1);
    QH_CHECK_EQ(loaded._activeProviderName, std::string("a"));
    QH_CHECK_EQ(loaded._activeModelName, std::string("m1"));
    std::remove(path.c_str());
}

QH_TEST(store_load_missing_file_defaults) {
    qh::config::SettingsStore store("definitely_not_exist_xyz.json");
    auto s = store.load();
    QH_CHECK(s._providers.empty());
    QH_CHECK(store.lastError().empty());
}

QH_TEST(store_load_corrupt_json) {
    const std::string path = "test_setting_bad.json";
    { std::ofstream out(path); out << "{ this is not json"; }
    qh::config::SettingsStore store(path);
    auto s = store.load();
    QH_CHECK(s._providers.empty());
    QH_CHECK(!store.lastError().empty());
    std::remove(path.c_str());
}

QH_TEST(store_load_partial_provider_kept_not_dropped) {
    // provider 缺 apiKey 等字段 → 用默认值保留，不全量回退
    const std::string path = "test_setting_partial.json";
    { std::ofstream out(path); out << R"({"providers":[{"name":"partial","providerType":"OpenAI"}]})"; }
    qh::config::SettingsStore store(path);
    auto s = store.load();
    QH_CHECK_EQ(s._providers.size(), (size_t)1);
    QH_CHECK_EQ(s._providers[0]._name, std::string("partial"));
    QH_CHECK_EQ(s._providers[0]._apiKey, std::string(""));
    QH_CHECK(store.lastError().empty());
    std::remove(path.c_str());
}

QH_TEST(store_load_old_profiles_format_treated_empty) {
    // 旧格式（profiles）→ 新 from_json 读不到 providers → 空，不报错
    const std::string path = "test_setting_old.json";
    { std::ofstream out(path); out << R"({"profiles":[{"name":"x","model":"m"}]})"; }
    qh::config::SettingsStore store(path);
    auto s = store.load();
    QH_CHECK(s._providers.empty());
    QH_CHECK(store.lastError().empty());
    std::remove(path.c_str());
}
