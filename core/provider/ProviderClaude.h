#pragma once
#include "provider/Provider.h"
#include <string>

namespace qh {
namespace provider {

// Claude 格式大模型连接（骨架）
class ProviderClaude : public Provider {
public:
    ProviderClaude(std::string apiKey, std::string baseUrl, std::string model)
        : apiKey_(std::move(apiKey)),
          baseUrl_(std::move(baseUrl)),
          model_(std::move(model)) {}

    schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override {
        // TODO: 基于 httplib 实现 Claude 格式请求与响应解析
        (void)messages;
        (void)tools;
        return schema::Message{};
    }

private:
    std::string apiKey_;
    std::string baseUrl_;
    std::string model_;
};

} // namespace provider
} // namespace qh
