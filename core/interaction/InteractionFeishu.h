#ifndef QH_INTERACTION_FEISHU_H
#define QH_INTERACTION_FEISHU_H
#include "interaction/Interaction.h"
#include "qh_export.h"
#include <string>

namespace qh {
namespace interaction {

// 飞书交互连接（骨架）
class QH_API InteractionFeishu : public Interaction {
public:
    explicit InteractionFeishu(std::string webhook);

    void send(const std::string& message) override;

private:
    std::string _webhook;
};

} // namespace interaction
} // namespace qh
#endif // QH_INTERACTION_FEISHU_H
