#ifndef QH_INTERACTION_H
#define QH_INTERACTION_H
#include <string>
#include "qh_export.h"

namespace qh {
namespace interaction {

// 外部交互连接抽象基类（飞书等外部渠道）
class QH_API Interaction {
public:
    virtual ~Interaction() = default;
    virtual void send(const std::string& message) = 0;
};

} // namespace interaction
} // namespace qh
#endif // QH_INTERACTION_H
