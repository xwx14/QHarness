#include "robot/RobotFeishu.h"

namespace qh {
namespace robot {

RobotFeishu::RobotFeishu(std::string webhook) : _webhook(std::move(webhook)) {}

void RobotFeishu::send(const std::string& message) {
    // TODO: 基于 httplib 向飞书 webhook 推送
    (void)message;
}

} // namespace robot
} // namespace qh
