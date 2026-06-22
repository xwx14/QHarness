#pragma once
#include <string>

namespace qh {
namespace robot {

// 外部机器人连接抽象基类
class Robot {
public:
    virtual ~Robot() = default;
    virtual void send(const std::string& message) = 0;
};

} // namespace robot
} // namespace qh
