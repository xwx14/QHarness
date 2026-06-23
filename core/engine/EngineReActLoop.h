#ifndef QH_ENGINE_REACT_LOOP_H
#define QH_ENGINE_REACT_LOOP_H
#include "engine/Engine.h"
#include "qh_export.h"
#include <string>

namespace qh {
namespace engine {

// 典型 ReAct 循环引擎：思考(Reasoning)→行动(Action)→观察(Observation)
class QH_API EngineReActLoop : public Engine {
public:
    void run(const std::string& userPrompt) override;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_REACT_LOOP_H
