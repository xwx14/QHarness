#ifndef QH_PROVIDER_MOCK_2STAGE_H
#define QH_PROVIDER_MOCK_2STAGE_H
#include "provider/Provider.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// 双阶段模拟大模型，用于测试 Engine2StageReAct 的 Phase1/Phase2 语义
// 按 tools 是否为空区分阶段（对齐 Engine2StageReAct 的两阶段调用约定）：
//   - tools 为空（Phase1 慢思考）→ 返回纯文本思考（Assistant，无 toolCalls）
//   - tools 非空（Phase2 行动）   → _actionTurn 计数：第1次 bash 工具调用、第2次完成文本
class QH_API MockTwoStageProvider : public Provider {
public:
    GenerateResult generate(
        const schema::CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

private:
    int _actionTurn{0};   // 仅 Phase2（tools 非空）累计
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_MOCK_2STAGE_H
