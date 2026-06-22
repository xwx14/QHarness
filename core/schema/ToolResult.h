#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace qh {
namespace schema {

// 工具本地执行完毕返回的物理结果
struct ToolResult {
    std::string toolCallId;
    std::string output;   // 控制台输出或报错堆栈
    bool isError = false; // 是否失败，供错误自愈使用
};

void to_json(nlohmann::json& j, const ToolResult& tr);
void from_json(const nlohmann::json& j, ToolResult& tr);

} // namespace schema
} // namespace qh
