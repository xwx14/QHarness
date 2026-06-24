#ifndef QH_PROVIDER_OPENAI_H
#define QH_PROVIDER_OPENAI_H
#include "provider/Provider.h"
#include "qh_export.h"
#include <string>

namespace qh {
namespace provider {

// OpenAI 格式大模型连接（骨架，HTTP 实现待后续步骤）
class QH_API ProviderOpenAI : public Provider {
public:
    ProviderOpenAI(std::string apiKey, std::string baseUrl, std::string model);

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
#endif // QH_PROVIDER_OPENAI_H
