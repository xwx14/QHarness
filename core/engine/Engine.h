#ifndef QH_ENGINE_H
#define QH_ENGINE_H
#include <string>
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace provider { class Provider; }   // 前向声明（指针成员，解耦）
namespace tool { class ToolManager; }

namespace engine {

// 运行过程类（抽象基类）；构造注入 Provider、ToolManager 与工作目录
class QH_API Engine : public schema::PostMessageInterface {
public:
    Engine(provider::Provider* provider, tool::ToolManager* toolManager, std::string workDir)
        : _provider(provider), _toolManager(toolManager), _workDir(std::move(workDir)) {}

    virtual ~Engine() = default;
    virtual void run(const std::string& userPrompt) = 0;

protected:
    provider::Provider* _provider = nullptr;     // 非拥有
    tool::ToolManager* _toolManager = nullptr;   // 非拥有
    std::string _workDir;
};

} // namespace engine
} // namespace qh
#endif // QH_ENGINE_H
