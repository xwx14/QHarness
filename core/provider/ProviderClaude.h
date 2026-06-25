#ifndef QH_PROVIDER_CLAUDE_H
#define QH_PROVIDER_CLAUDE_H
#include "provider/Provider.h"
#include "qh_export.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace qh {
namespace provider {

// Claude 格式大模型连接（Anthropic Messages API）
class QH_API ProviderClaude : public Provider {
public:
    ProviderClaude(std::string apiKey, std::string baseUrl, std::string model);

    GenerateResult generate(
        const schema::CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override;

    void setTemperature(double t);
    void setMaxTokens(int n);

private:
    nlohmann::json buildClaudeRequest(const std::vector<schema::Message>& messages,
                                      const std::vector<schema::ToolDefinition>& tools) const;
    schema::Message parseClaudeResponse(const std::string& body) const;
    static std::string parseClaudeError(int status, const std::string& body);

    std::string _apiKey;
    std::string _baseUrl;
    std::string _model;
    std::optional<double> _temperature;
    std::optional<int> _maxTokens;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_CLAUDE_H
