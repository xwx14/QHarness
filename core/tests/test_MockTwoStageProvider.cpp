#include "TestHarness.h"
#include "provider/MockTwoStageProvider.h"
#include "schema/CancellationToken.h"
#include "schema/Message.h"
#include <vector>

// Phase1：tools 为空 → 纯文本思考，无 toolCalls
QH_TEST(mocktwostage_empty_tools_returns_thinking) {
    qh::provider::MockTwoStageProvider provider;
    qh::schema::CancellationToken token;
    std::vector<qh::schema::Message> msgs;
    std::vector<qh::schema::ToolDefinition> noTools;   // 空 = Phase1

    qh::provider::GenerateResult r = provider.generate(token, msgs, noTools);

    QH_CHECK(r.message._role == qh::schema::Role::Assistant);
    QH_CHECK(!r.message._content.empty());
    QH_CHECK(r.message._toolCalls.empty());
    QH_CHECK(r.error.empty());
}

// Phase2：tools 非空 → 第1次 bash 调用，第2次完成文本
QH_TEST(mocktwostage_nonempty_tools_action_turns) {
    qh::provider::MockTwoStageProvider provider;
    qh::schema::CancellationToken token;
    std::vector<qh::schema::Message> msgs;
    std::vector<qh::schema::ToolDefinition> tools(1);   // 非空 = Phase2

    qh::provider::GenerateResult r1 = provider.generate(token, msgs, tools);
    QH_CHECK(r1.message._toolCalls.size() == 1);
    QH_CHECK(r1.message._toolCalls[0]._name == "bash");

    qh::provider::GenerateResult r2 = provider.generate(token, msgs, tools);
    QH_CHECK(r2.message._toolCalls.empty());
    QH_CHECK(!r2.message._content.empty());
}
