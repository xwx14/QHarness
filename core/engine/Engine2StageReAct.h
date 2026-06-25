#ifndef QH_ENGINE_2STAGE_REACT_H
#define QH_ENGINE_2STAGE_REACT_H
#include "engine/Engine.h"
#include "qh_export.h"

namespace qh {
namespace engine {

// 双阶段 ReAct 引擎：Phase1 慢思考（剥夺工具，可开关）+ Phase2 行动（恢复工具）
// 忠实移植 Go loop.go 的 TwoStageReAct
class QH_API Engine2StageReAct : public Engine {
public:
    // 慢思考默认开启；workDir 之后追加 enableThinking（基类构造不含此参数）
    Engine2StageReAct(provider::Provider* provider, tool::ToolManager* toolManager,
                      std::string workDir, bool enableThinking = true);
    void setEnableThinking(bool enable);   // 运行时切换
    void run(const std::string& userPrompt) override;

private:
    bool _enableThinking;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_2STAGE_REACT_H
