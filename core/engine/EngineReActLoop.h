#ifndef QH_ENGINE_REACT_LOOP_H
#define QH_ENGINE_REACT_LOOP_H
#include "engine/Engine.h"
#include <string>

namespace qh {
namespace engine {

// 典型 ReAct 循环引擎：思考(Reasoning)→行动(Action)→观察(Observation)
class EngineReActLoop : public Engine {
public:
    void run(const std::string& userPrompt) override {
        // TODO: 后续步骤实现完整 ReAct 循环
        (void)userPrompt;
    }
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_REACT_LOOP_H
