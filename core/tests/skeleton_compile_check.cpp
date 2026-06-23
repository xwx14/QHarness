#include "engine/Engine.h"
#include "engine/EngineReActLoop.h"
#include "provider/Provider.h"
#include "provider/ProviderOpenAI.h"
#include "provider/ProviderClaude.h"
#include "tool/Tool.h"
#include "interaction/Interaction.h"
#include "interaction/InteractionFeishu.h"
#include "memory/Memory.h"
#include "memory/MemoryFile.h"
#include "schema/Message.h"

namespace qh {
namespace checks {

// 强制实例化各派生骨架，验证接口可被实现、可被使用（编译期 + 链接期检查）
void touchSkeletons() {
    engine::EngineReActLoop e;
    e.run("hi");

    provider::ProviderOpenAI po("key", "https://api.example.com", "gpt-x");
    provider::ProviderClaude pc("key", "https://api.example.com", "claude-x");
    (void)po;
    (void)pc;

    memory::MemoryFile mf("./mem");
    mf.load();
    mf.save();

    interaction::InteractionFeishu rf("https://open.feishu.cn/hook/x");
    rf.send("hi");
}

} // namespace checks
} // namespace qh
