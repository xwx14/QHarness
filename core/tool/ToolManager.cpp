#include "tool/ToolManager.h"
#include <future>
#include <map>
#include <algorithm>

namespace qh {
namespace tool {

bool ToolManager::registerTool(Tool& tool) {
    const std::string name = tool.definition()._name;
    if (_tools.count(name) > 0) {
        return false;  // 重名：不覆盖
    }
    _tools.emplace(name, &tool);
    return true;
}

Tool* ToolManager::getTool(const std::string& name) const {
    auto it = _tools.find(name);
    return it == _tools.end() ? nullptr : it->second;
}

bool ToolManager::hasTool(const std::string& name) const {
    return _tools.count(name) > 0;
}

bool ToolManager::unregisterTool(const std::string& name) {
    return _tools.erase(name) > 0;
}

std::vector<schema::ToolDefinition> ToolManager::getAvailableTools() const {
    std::vector<schema::ToolDefinition> defs;
    defs.reserve(_tools.size());
    for (const auto& kv : _tools) {
        defs.push_back(kv.second->definition());
    }
    return defs;
}

schema::ToolResult ToolManager::execute(const schema::ToolCall& call) {
    Tool* tool = getTool(call._name);
    if (tool == nullptr) {
        schema::ToolResult result;
        result._toolCallId = call._id;
        result._output = "未找到工具: " + call._name;
        result._isError = true;
        return result;
    }
    return tool->execute(call);
}

std::vector<schema::ToolResult> ToolManager::executeAll(
    const std::vector<schema::ToolCall>& calls, int maxConcurrency) {
    const size_t n = calls.size();
    std::vector<schema::ToolResult> results(n);
    if (n == 0) return results;

    // 1) 分桶：key → 原索引列表（保持原序）。无工具/无 key → 唯一哨兵桶（自由并发）
    std::map<std::string, std::vector<size_t>> buckets;
    for (size_t i = 0; i < n; ++i) {
        Tool* tool = getTool(calls[i]._name);
        std::optional<std::string> key = tool ? tool->resourceKey(calls[i]) : std::nullopt;
        std::string bk = key.value_or("__free_" + std::to_string(i) + "__");
        buckets[bk].push_back(i);
    }

    // 2) 异常隔离：子线程 execute 万一抛 C++ 异常，转为 isError，绝不炸进程
    auto runOne = [this](const schema::ToolCall& c) -> schema::ToolResult {
        try {
            return this->execute(c);
        } catch (const std::exception& e) {
            schema::ToolResult r; r._toolCallId = c._id;
            r._isError = true; r._output = std::string("工具执行异常: ") + e.what();
            return r;
        } catch (...) {
            schema::ToolResult r; r._toolCallId = c._id;
            r._isError = true; r._output = "工具执行异常: 未知错误";
            return r;
        }
    };

    // 3) 每桶一个 async，桶内按原序串行 execute；桶间并发
    auto runBucket = [&](const std::vector<size_t>& idxs) {
        for (size_t idx : idxs) results[idx] = runOne(calls[idx]);
    };
    std::vector<std::pair<std::string, std::vector<size_t>>> bucketList(buckets.begin(), buckets.end());

    auto launchBatch = [&](size_t begin, size_t end) {
        std::vector<std::future<void>> fs;
        for (size_t b = begin; b < end; ++b)
            fs.push_back(std::async(std::launch::async, runBucket, std::cref(bucketList[b].second)));
        for (auto& f : fs) f.get();
    };

    if (maxConcurrency <= 0) {
        launchBatch(0, bucketList.size());
    } else {
        for (size_t b = 0; b < bucketList.size(); b += maxConcurrency)
            launchBatch(b, std::min(b + maxConcurrency, bucketList.size()));
    }
    return results;
}

} // namespace tool
} // namespace qh
