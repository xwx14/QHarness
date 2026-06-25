#ifndef QH_PROVIDER_OPENAI_H
#define QH_PROVIDER_OPENAI_H
#include "provider/Provider.h"
#include "qh_export.h"
#include <nlohmann/json.hpp>
#include <optional>
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

    void setTemperature(double t);
    void setMaxTokens(int n);

private:
    nlohmann::json buildOpenaiRequest(const std::vector<schema::Message>& messages,
                                      const std::vector<schema::ToolDefinition>& tools) const;
    schema::Message parseOpenaiResponse(const std::string& body) const;
    static std::string parseOpenaiError(int status, const std::string& body);

    std::string _apiKey;
    std::string _baseUrl;
    std::string _model;
    std::optional<double> _temperature;
    std::optional<int> _maxTokens;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_OPENAI_H
