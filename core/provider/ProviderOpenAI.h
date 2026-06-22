#pragma once
#include "provider/Provider.h"
#include <string>

namespace qh {
namespace provider {

// OpenAI 格式大模型连接（骨架，HTTP 实现待后续步骤）
class ProviderOpenAI : public Provider {
public:
    ProviderOpenAI(std::string apiKey, std::string baseUrl, std::string model)
        : apiKey_(std::move(apiKey)),
          baseUrl_(std::move(baseUrl)),
          model_(std::move(model)) {}

    schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) override {
        // TODO: 基于 httplib 实现 OpenAI 格式请求与响应解析
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
