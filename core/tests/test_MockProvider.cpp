#include "TestHarness.h"
#include "provider/MockProvider.h"
#include "schema/CancellationToken.h"
#include "schema/Message.h"
#include <string>
#include <vector>

QH_TEST(mockprovider_first_turn_returns_bash_toolcall) {
    qh::provider::MockProvider provider;
    qh::schema::CancellationToken token;
    std::vector<qh::schema::Message> messages;
    std::vector<qh::schema::ToolDefinition> tools;

    qh::provider::GenerateResult result = provider.generate(token, messages, tools);

    QH_CHECK(result.error.empty());
    QH_CHECK(result.message._role == qh::schema::Role::Assistant);
    QH_CHECK(result.message._content.find("文件") != std::string::npos);
    QH_CHECK_EQ(result.message._toolCalls.size(), static_cast<size_t>(1));
    QH_CHECK_EQ(result.message._toolCalls[0]._id, std::string("call_123"));
    QH_CHECK_EQ(result.message._toolCalls[0]._name, std::string("bash"));
    QH_CHECK(result.message._toolCalls[0]._arguments.find("ls -la") != std::string::npos);
}

QH_TEST(mockprovider_second_turn_returns_plain_text) {
    qh::provider::MockProvider provider;
    qh::schema::CancellationToken token;
    std::vector<qh::schema::Message> messages;
    std::vector<qh::schema::ToolDefinition> tools;

    (void)provider.generate(token, messages, tools); // 第 1 轮
    qh::provider::GenerateResult result = provider.generate(token, messages, tools); // 第 2 轮

    QH_CHECK(result.error.empty());
    QH_CHECK(result.message._role == qh::schema::Role::Assistant);
    QH_CHECK(result.message._content.find("任务完成") != std::string::npos);
    QH_CHECK(result.message._toolCalls.empty());
}
