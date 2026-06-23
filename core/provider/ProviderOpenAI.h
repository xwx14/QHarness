#ifndef QH_PROVIDER_OPENAI_H
#define QH_PROVIDER_OPENAI_H
#include "provider/Provider.h"
#include <string>

namespace qh {
namespace provider {

// OpenAI 格式大模型连接（骨架，HTTP 实现待后续步骤）
class ProviderOpenAI : public Provider {
public:
    ProviderOpenAI(std::string apiKey, std::string baseUrl, std::string model);

    schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

private:
    std::string apiKey_;
    std::string baseUrl_;
    std::string model_;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_OPENAI_H
