#ifndef QH_MEMORY_H
#define QH_MEMORY_H
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace memory {

// 记忆系统抽象基类；继承 PostMessageInterface 获得实时消息注入
class QH_API Memory : public schema::PostMessageInterface {
public:
    virtual ~Memory() = default;
    virtual void load() = 0;
    virtual void save() = 0;
};

} // namespace memory
} // namespace qh
#endif // QH_MEMORY_H
