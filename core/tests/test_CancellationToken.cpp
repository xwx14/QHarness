#include "TestHarness.h"
#include "provider/CancellationToken.h"
#include <chrono>

QH_TEST(canceltoken_default_not_cancelled) {
    qh::provider::CancellationToken token;
    QH_CHECK(!token.isCancelled());
}

QH_TEST(canceltoken_cancel_marks_cancelled) {
    qh::provider::CancellationToken token;
    token.cancel();
    QH_CHECK(token.isCancelled());
}

QH_TEST(canceltoken_past_deadline_is_cancelled) {
    qh::provider::CancellationToken token;
    token.setDeadline(std::chrono::steady_clock::now() - std::chrono::seconds(1));
    QH_CHECK(token.isCancelled());
}

QH_TEST(canceltoken_future_deadline_not_cancelled) {
    qh::provider::CancellationToken token;
    token.setDeadline(std::chrono::steady_clock::now() + std::chrono::hours(1));
    QH_CHECK(!token.isCancelled());
}
