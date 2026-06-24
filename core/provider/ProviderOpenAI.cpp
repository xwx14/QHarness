#include "provider/ProviderOpenAI.h"

namespace qh {
namespace provider {

ProviderOpenAI::ProviderOpenAI(std::string apiKey, std::string baseUrl, std::string model)
    : _apiKey(std::move(apiKey)),
      _baseUrl(std::move(baseUrl)),
      _model(std::move(model)) {}

GenerateResult ProviderOpenAI::generate(
    const schema::CancellationToken& /*cancel*/,
    const std::vector<schema::Message>& /*messages*/,
    const std::vector<schema::ToolDefinition>& /*tools*/) {
    // TODO: 基于 httplib 实现 OpenAI 格式请求与响应解析
    return GenerateResult{};
}

} // namespace provider
} // namespace qh
