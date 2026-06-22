#pragma once
#include <vector>
#include "schema/Message.h"
#include "schema/ToolDefinition.h"

namespace qh {
namespace provider {

// 大模型连接抽象基类
class Provider {
public:
    virtual ~Provider() = default;
    virtual schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) = 0;
};

} // namespace provider
} // namespace qh
