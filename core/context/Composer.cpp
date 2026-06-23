#include "context/Composer.h"

namespace qh {
namespace context {

Composer::Composer(std::string workDir, bool planMode)
    : _workDir(std::move(workDir)), _planMode(planMode) {}

schema::Message Composer::build() const {
    // TODO: 组装系统提示词（核心身份 + planMode 分支 + AGENTS.md + 技能加载），参考 Go composer.go
    (void)_workDir;
    (void)_planMode;
    return schema::Message{};
}

} // namespace context
} // namespace qh
