#ifndef QH_ROBOT_FEISHU_H
#define QH_ROBOT_FEISHU_H
#include "robot/Robot.h"
#include <string>

namespace qh {
namespace robot {

// 飞书机器人连接（骨架）
class RobotFeishu : public Robot {
public:
    explicit RobotFeishu(std::string webhook) : webhook_(std::move(webhook)) {}

    void send(const std::string& message) override {
        // TODO: 基于 httplib 向飞书 webhook 推送
        (void)message;
    }

private:
    std::string webhook_;
};

} // namespace robot
} // namespace qh
#endif // QH_ROBOT_FEISHU_H
