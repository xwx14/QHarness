#ifndef QH_PROVIDER_MOCK_H
#define QH_PROVIDER_MOCK_H
#include "provider/Provider.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// 模拟大模型，用于测试引擎 ReAct 循环（参考 Go mockProvider）
// 行为：第1轮返回带 bash 工具调用的 Assistant 消息；第2轮起返回纯文本"任务完成"
class QH_API MockProvider : public Provider {
public:
    GenerateResult generate(
        const schema::CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

private:
    int _turn{0};
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_MOCK_H
