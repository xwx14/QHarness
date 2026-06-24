#include "provider/CancellationToken.h"

namespace qh {
namespace provider {

void CancellationToken::cancel() {
    _cancelled.store(true, std::memory_order_release);
}

bool CancellationToken::isCancelled() const {
    if (_cancelled.load(std::memory_order_acquire)) {
        return true;
    }
    if (_deadline.has_value() &&
        std::chrono::steady_clock::now() >= *_deadline) {
        return true;
    }
    return false;
}

void CancellationToken::setDeadline(std::chrono::steady_clock::time_point deadline) {
    _deadline = deadline;
}

} // namespace provider
} // namespace qh
