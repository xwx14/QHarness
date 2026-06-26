#include "EngineThread.h"
#include <QDir>
#include "engine/Engine.h"
#include "engine/EngineReActLoop.h"
#include "engine/Engine2StageReAct.h"
#include "provider/Provider.h"
#include "provider/ProviderOpenAI.h"
#include "provider/ProviderClaude.h"
#include "provider/MockProvider.h"
#include "provider/MockTwoStageProvider.h"
#include "tool/MockBashTool.h"
#include "tool/ToolManager.h"
#include <utility>

namespace qh {
namespace app {

EngineThread::EngineThread(QPostMessage* postMessage, std::string prompt,
                           schema::Settings settings, EngineKind kind, QObject* parent)
    : QThread(parent), _postMessage(postMessage),
      _prompt(std::move(prompt)), _settings(std::move(settings)) {

    // 1) Provider：按激活 profile 创建 OpenAI/Claude；未配/缺字段 → 回退 mock
    const schema::LlmProfile* prof = schema::findActiveProfile(_settings);
    const bool valid = prof && !prof->_baseUrl.empty()
                       && !prof->_apiKey.empty() && !prof->_model.empty();
    if (valid) {
        if (prof->_providerType == schema::ProviderType::Claude) {
            auto p = std::make_unique<provider::ProviderClaude>(
                prof->_apiKey, prof->_baseUrl, prof->_model);
            if (prof->_temperature) p->setTemperature(*prof->_temperature);
            if (prof->_maxTokens)   p->setMaxTokens(*prof->_maxTokens);
            _provider = std::move(p);
        } else {
            auto p = std::make_unique<provider::ProviderOpenAI>(
                prof->_apiKey, prof->_baseUrl, prof->_model);
            if (prof->_temperature) p->setTemperature(*prof->_temperature);
            if (prof->_maxTokens)   p->setMaxTokens(*prof->_maxTokens);
            _provider = std::move(p);
        }
    } else {
        _postMessage->post(schema::Level::Warn, "未配置有效 profile，回退 mock provider");
        if (kind == EngineKind::TwoStageReAct) {
            _provider = std::make_unique<provider::MockTwoStageProvider>();
        } else {
            _provider = std::make_unique<provider::MockProvider>();
        }
    }
    _provider->setPostMessage(_postMessage);

    // 2) 工具：本阶段仅 MockBashTool；按 enabledTools 名字筛选注册（空 = 不注册任何工具）
    _bashTool    = std::make_unique<tool::MockBashTool>();
    _toolManager = std::make_unique<tool::ToolManager>();
    _toolManager->setPostMessage(_postMessage);
    for (const auto& name : _settings._enabledTools) {
        if (name == "bash") _toolManager->registerTool(*_bashTool);
    }

    // 3) Engine：kind 决定种类（ReAct/两阶段）；enableThinking 仅作用于两阶段 Phase1
    std::string workDir = _settings._workDir.empty()
        ? QDir::currentPath().toStdString()
        : _settings._workDir;
    if (kind == EngineKind::TwoStageReAct) {
        _engine = std::make_unique<engine::Engine2StageReAct>(
            _provider.get(), _toolManager.get(), std::move(workDir), _settings._enableThinking);
    } else {
        _engine = std::make_unique<engine::EngineReActLoop>(
            _provider.get(), _toolManager.get(), std::move(workDir));
    }
    _engine->setPostMessage(_postMessage);
}

EngineThread::~EngineThread() {
    // 先等 worker 线程退出（其引用本对象的 _engine 等成员），再由成员逆序析构销毁子树
    wait();
}

void EngineThread::run() {
    _engine->run(_prompt);   // worker 线程执行 ReAct 循环
}

} // namespace app
} // namespace qh
