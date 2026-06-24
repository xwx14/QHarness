#ifndef QH_PROVIDER_H
#define QH_PROVIDER_H
#include <string>
#include <vector>
#include "schema/Message.h"
#include "schema/CancellationToken.h"
#include "schema/PostMessage.h"
#include "qh_export.h"

namespace qh {
namespace provider {

// generate 的返回结果，对齐 Go 的 (*Message, error)：error 空串表示成功
struct QH_API GenerateResult {
    schema::Message message;
    std::string error;   // 空=成功；非空=失败描述（HTTP/解析错误等）
};

// 大模型连接抽象基类
class QH_API Provider : public schema::PostMessageInterface {
public:
    virtual ~Provider() = default;
    // 失败时 error 非空（不再抛异常）；provider 应在耗时点协作式检查 cancel.isCancelled()
    virtual GenerateResult generate(
        const schema::CancellationToken& cancel,
        const std::vector<schema::Message>& messages,
        const std::vector<schema::ToolDefinition>& tools) = 0;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_H
