#include "TestHarness.h"
#include <iostream>

namespace qh { namespace checks { void touchSkeletons(); } } // 新增声明

int main() {
    std::cout << "=== QHarness Core Tests ===\n";
    for (auto& tc : qh::test::registry()) {
        std::cout << "[ " << tc.name << " ]\n";
        tc.fn();
    }
    qh::checks::touchSkeletons();
    std::cout << "Skeletons compile & link OK\n";
    std::cout << "\n" << qh::test::totalCount() << " checks, "
              << qh::test::failCount() << " failures\n";
    return qh::test::failCount() == 0 ? 0 : 1;
}
