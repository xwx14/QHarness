#ifndef QH_PROVIDER_H
#define QH_PROVIDER_H
#include <vector>
#include "schema/Message.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// 大模型连接抽象基类
class QH_API Provider {
public:
    virtual ~Provider() = default;
    // 失败时抛 std::runtime_error（HTTP/解析错误）
    virtual schema::Message generate(
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) = 0;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_H
