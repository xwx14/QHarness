#ifndef QH_PROVIDER_CLAUDE_H
#define QH_PROVIDER_CLAUDE_H
#include "provider/Provider.h"
#include "qh_export.h"
#include <string>

namespace qh {
namespace provider {

// Claude 格式大模型连接（骨架）
class QH_API ProviderClaude : public Provider {
public:
    ProviderClaude(std::string apiKey, std::string baseUrl, std::string model);

    GenerateResult generate(
        const schema::CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

private:
    std::string _apiKey;
    std::string _baseUrl;
    std::string _model;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_CLAUDE_H
