#include "interaction/InteractionFeishu.h"

namespace qh {
namespace interaction {

InteractionFeishu::InteractionFeishu(std::string webhook) : _webhook(std::move(webhook)) {}

void InteractionFeishu::send(const std::string& message) {
    // TODO: 基于 httplib 向飞书 webhook 推送
    (void)message;
}

} // namespace interaction
} // namespace qh
