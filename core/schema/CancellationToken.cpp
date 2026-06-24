#include "schema/CancellationToken.h"

namespace qh {
namespace schema {

void CancellationToken::cancel() {
    std::lock_guard<std::mutex> lock(_mutex);
    _cancelled = true;
}

bool CancellationToken::isCancelled() const {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_cancelled) {
        return true;
    }
    if (_deadline.has_value() &&
        std::chrono::steady_clock::now() >= *_deadline) {
        return true;
    }
    return false;
}

void CancellationToken::setDeadline(std::chrono::steady_clock::time_point deadline) {
    std::lock_guard<std::mutex> lock(_mutex);
    _deadline = deadline;
}

} // namespace schema
} // namespace qh
