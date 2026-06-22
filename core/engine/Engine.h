#ifndef QH_ENGINE_H
#define QH_ENGINE_H
#include <string>

namespace qh {
namespace engine {

// 运行过程类（抽象基类）
class Engine {
public:
    virtual ~Engine() = default;
    virtual void run(const std::string& userPrompt) = 0;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_H
