#ifndef QH_PROVIDER_CANCEL_TOKEN_H
#define QH_PROVIDER_CANCEL_TOKEN_H
#include "qh_export.h"
#include <atomic>
#include <chrono>
#include <optional>

namespace qh {
namespace provider {

// 轻量取消/超时令牌，对齐 Go context.Context 的取消与超时核心语义（不含 Value map）
// 纯 C++17、零外部依赖；provider 在耗时循环中协作式查询 isCancelled()
class QH_API CancellationToken {
public:
    CancellationToken() = default;

    // 请求取消（线程安全）
    void cancel();

    // 是否已被取消：显式 cancel() 为真，或已设 deadline 且当前时刻越过之
    bool isCancelled() const;

    // 设置超时截止时刻；不设置则仅响应显式 cancel()
    void setDeadline(std::chrono::steady_clock::time_point deadline);

private:
    std::atomic<bool> _cancelled{false};
    std::optional<std::chrono::steady_clock::time_point> _deadline;
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_CANCEL_TOKEN_H
