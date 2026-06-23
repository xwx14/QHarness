#ifndef QH_MEMORY_H
#define QH_MEMORY_H
#include "qh_export.h"

namespace qh {
namespace memory {

// 记忆系统抽象基类
class QH_API Memory {
public:
    virtual ~Memory() = default;
    virtual void load() = 0;
    virtual void save() = 0;
};

} // namespace memory
} // namespace qh
#endif // QH_MEMORY_H
