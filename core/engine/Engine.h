#ifndef QH_ENGINE_H
#define QH_ENGINE_H
#include <string>
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace engine {

// 运行过程类（抽象基类）；继承 PostMessageInterface 获得实时消息注入能力
class QH_API Engine : public schema::PostMessageInterface {
public:
    virtual ~Engine() = default;
    virtual void run(const std::string& userPrompt) = 0;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_H
