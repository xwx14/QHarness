#include "provider/ProviderClaude.h"

namespace qh {
namespace provider {

ProviderClaude::ProviderClaude(std::string apiKey, std::string baseUrl, std::string model)
    : apiKey_(std::move(apiKey)),
      baseUrl_(std::move(baseUrl)),
      model_(std::move(model)) {}

schema::Message ProviderClaude::generate(
    const std::vector<schema::Message>& messages,
    const std::vector<schema::ToolDefinition>& tools) {
    // TODO: 基于 httplib 实现 Claude 格式请求与响应解析
    (void)messages;
    (void)tools;
    return schema::Message{};
}

} // namespace provider
} // namespace qh
