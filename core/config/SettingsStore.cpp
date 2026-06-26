#include "config/SettingsStore.h"
#include <fstream>
#include <sstream>

namespace qh {
namespace config {

SettingsStore::SettingsStore(std::string filePath) : _filePath(std::move(filePath)) {}

schema::Settings SettingsStore::load() const {
    _lastError.clear();
    schema::Settings s;
    std::ifstream in(_filePath);
    if (!in.is_open()) {
        return s;   // 文件不存在 → 默认（正常首次运行）
    }
    std::stringstream ss;
    ss << in.rdbuf();
    auto j = nlohmann::json::parse(ss.str(), nullptr, false);
    if (j.is_discarded()) {
        _lastError = "配置文件 JSON 解析失败: " + _filePath;
        return s;   // 损坏 → 默认 + 记错
    }
    try {
        s = j.get<schema::Settings>();
    } catch (const std::exception& e) {
        _lastError = std::string("配置反序列化失败: ") + e.what();
        return schema::Settings{};
    }
    return s;
}

bool SettingsStore::save(const schema::Settings& s) const {
    _lastError.clear();
    std::ofstream out(_filePath, std::ios::trunc);
    if (!out.is_open()) {
        _lastError = "无法写入配置文件: " + _filePath;
        return false;
    }
    nlohmann::json j = s;
    out << j.dump(2);
    if (!out.good()) {
        _lastError = "写入配置文件失败: " + _filePath;
        return false;
    }
    return true;
}

std::string SettingsStore::lastError() const { return _lastError; }

} // namespace config
} // namespace qh
