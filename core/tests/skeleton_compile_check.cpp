#include "engine/Engine.h"
#include "engine/EngineReActLoop.h"
#include "engine/Engine2StageReAct.h"
#include "provider/Provider.h"
#include "provider/ProviderOpenAI.h"
#include "provider/ProviderClaude.h"
#include "provider/MockProvider.h"
#include "provider/MockTwoStageProvider.h"
#include "tool/Tool.h"
#include "interaction/Interaction.h"
#include "interaction/InteractionFeishu.h"
#include "memory/Memory.h"
#include "memory/MemoryFile.h"
#include "context/Composer.h"
#include "tool/ToolManager.h"
#include "schema/Message.h"

namespace qh {
namespace checks {

// 强制实例化各派生骨架，验证接口可被实现、可被使用（编译期 + 链接期检查）
void touchSkeletons() {
    schema::PostMessage* nullPM = nullptr;
    // 仅实例化验证构造签名；run() 的运行验证由 test_EngineReActLoop 负责
    // （此处 _provider/_toolManager 为 nullptr，不可调 run，否则解引用空指针）
    engine::EngineReActLoop e(nullptr, nullptr, ".");
    engine::Engine2StageReAct e2(nullptr, nullptr, ".", true);
    e2.setEnableThinking(false);

    provider::ProviderOpenAI po("key", "https://api.example.com", "gpt-x");
    provider::ProviderClaude pc("key", "https://api.example.com", "claude-x");
    {
        schema::CancellationToken token;
        std::vector<schema::Message> msgs;
        std::vector<schema::ToolDefinition> tools;
        (void)po.generate(token, msgs, tools);
        (void)pc.generate(token, msgs, tools);
        provider::MockProvider mp;
        (void)mp.generate(token, msgs, tools);
        mp.setPostMessage(nullPM);
        provider::MockTwoStageProvider m2;
        (void)m2.generate(token, msgs, tools);
        m2.setPostMessage(nullPM);
    }

    memory::MemoryFile mf("./mem");
    mf.load();
    mf.save();

    interaction::InteractionFeishu rf("https://open.feishu.cn/hook/x");
    rf.send("hi");

    context::Composer cc("./", false);
    (void)cc.build();

    // 验证 PostMessageInterface 注入接口（各骨架类继承后可用 setPostMessage）
    e.setPostMessage(nullPM);
    e2.setPostMessage(nullPM);
    po.setPostMessage(nullPM);
    pc.setPostMessage(nullPM);
    mf.setPostMessage(nullPM);
    rf.setPostMessage(nullPM);
    cc.setPostMessage(nullPM);
    tool::ToolManager tm;
    tm.setPostMessage(nullPM);
}

} // namespace checks
} // namespace qh
