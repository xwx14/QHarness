#ifndef QH_CONFIG_SETTINGS_STORE_H
#define QH_CONFIG_SETTINGS_STORE_H
#include <string>
#include "schema/Settings.h"
#include "qh_export.h"

namespace qh {
namespace config {

// 配置持久化：纯 C++（std::ifstream/ofstream），不依赖 Qt。
// 文件路径由 app 层注入（app 用 QCoreApplication::applicationDirPath() 拼出 <exeDir>/config/setting.json）。
class QH_API SettingsStore {
public:
    explicit SettingsStore(std::string filePath);

    // 文件缺失 → 默认 Settings（正常首次运行，lastError 空）；
    // JSON 损坏/反序列化失败 → 默认 Settings + lastError 记错。
    schema::Settings load() const;

    // 序列化后直接写 _filePath（KISS，不做原子临时文件替换）；失败置 lastError 返回 false。
    bool save(const schema::Settings& s) const;

    std::string lastError() const;

private:
    std::string _filePath;
    mutable std::string _lastError;   // load/save 失败时写入，供 UI 提示
};

} // namespace config
} // namespace qh
#endif // QH_CONFIG_SETTINGS_STORE_H
