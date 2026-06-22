#pragma once

namespace qh {
namespace memory {

// 记忆系统抽象基类
class Memory {
public:
    virtual ~Memory() = default;
    virtual void load() = 0;
    virtual void save() = 0;
};

} // namespace memory
} // namespace qh
