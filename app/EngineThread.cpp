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
#include "tool/BashTool.h"
#include "tool/ReadFileTool.h"
#include "tool/WriteFileTool.h"
#include "tool/ToolManager.h"
#include <utility>

namespace qh {
namespace app {

EngineThread::EngineThread(QPostMessage* postMessage, std::string prompt,
                           schema::Settings settings, EngineKind kind, QObject* parent)
    : QThread(parent), _postMessage(postMessage),
      _prompt(std::move(prompt)), _settings(std::move(settings)) {

    // 1) Provider：按激活供应商 + 激活模型创建 OpenAI/Claude；未配/缺字段 → 回退 mock
    const schema::LlmProvider* provider = schema::findActiveProvider(_settings);
    const schema::LlmModel* model = provider
        ? schema::findModel(*provider, _settings._activeModelName) : nullptr;
    const bool valid = provider && model
                       && !provider->_baseUrl.empty() && !provider->_apiKey.empty()
                       && !model->_name.empty();
    if (valid) {
        if (provider->_providerType == schema::ProviderType::Claude) {
            auto p = std::make_unique<provider::ProviderClaude>(
                provider->_apiKey, provider->_baseUrl, model->_name);
            if (model->_temperature) p->setTemperature(*model->_temperature);
            if (model->_maxTokens)   p->setMaxTokens(*model->_maxTokens);
            _provider = std::move(p);
        } else {
            auto p = std::make_unique<provider::ProviderOpenAI>(
                provider->_apiKey, provider->_baseUrl, model->_name);
            if (model->_temperature) p->setTemperature(*model->_temperature);
            if (model->_maxTokens)   p->setMaxTokens(*model->_maxTokens);
            _provider = std::move(p);
        }
    } else {
        _postMessage->post(schema::Level::Warn, "未配置有效供应商/模型，回退 mock provider");
        if (kind == EngineKind::TwoStageReAct) {
            _provider = std::make_unique<provider::MockTwoStageProvider>();
        } else {
            _provider = std::make_unique<provider::MockProvider>();
        }
    }
    _provider->setPostMessage(_postMessage);

    // 工作目录：空 → 回退当前路径（read_file 等工具需在工具创建前确定）
    std::string workDir = _settings._workDir.empty()
        ? QDir::currentPath().toStdString()
        : _settings._workDir;

    // 2) 工具：按 enabledTools 名字筛选创建并注册（空 = 不注册任何工具）
    _toolManager = std::make_unique<tool::ToolManager>();
    _toolManager->setPostMessage(_postMessage);
    for (const auto& name : _settings._enabledTools) {
        std::unique_ptr<tool::Tool> t;
        if (name == "bash") {
            t = std::make_unique<tool::BashTool>(workDir);
        } else if (name == "read_file") {
            t = std::make_unique<tool::ReadFileTool>(workDir);
        } else if (name == "write_file") {
            t = std::make_unique<tool::WriteFileTool>(workDir);
        }
        if (t) {
            _toolManager->registerTool(*t);
            _tools.push_back(std::move(t));   // 持有所有权，生命周期长于 ToolManager 引用
        }
    }

    // 3) Engine：kind 决定种类（ReAct/两阶段）；enableThinking 仅作用于两阶段 Phase1
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
