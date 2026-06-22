#include "TestHarness.h"
#include <iostream>

int main() {
    std::cout << "=== QHarness Core Tests ===\n";
    for (auto& tc : qh::test::registry()) {
        std::cout << "[ " << tc.name << " ]\n";
        tc.fn();
    }
    std::cout << "\n" << qh::test::totalCount() << " checks, "
              << qh::test::failCount() << " failures\n";
    return qh::test::failCount() == 0 ? 0 : 1;
}
