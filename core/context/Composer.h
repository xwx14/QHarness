#ifndef QH_CONTEXT_COMPOSER_H
#define QH_CONTEXT_COMPOSER_H
#include <string>
#include "schema/Message.h"
#include "qh_export.h"

namespace qh {
namespace context {

// 上下文组装器：组装系统提示词 Message（核心身份 + planMode 分支 + AGENTS.md + 技能）
// 对应 Go 的 PromptComposer（internal/context/composer.go）
class QH_API Composer {
public:
    // workDir：工作目录（读 AGENTS.md / 技能的根）；planMode：是否启用长程任务状态外部化规范
    Composer(std::string workDir, bool planMode);

    // 组装并返回系统提示词消息（骨架，待实现）
    schema::Message build() const;

private:
    std::string _workDir;
    bool _planMode;
};

} // namespace context
} // namespace qh
#endif // QH_CONTEXT_COMPOSER_H
