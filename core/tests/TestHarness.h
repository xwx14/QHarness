#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace qh {
namespace test {

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

inline int& failCount() { static int c = 0; return c; }
inline int& totalCount() { static int c = 0; return c; }

struct Registrar {
    Registrar(const char* name, void (*fn)()) {
        registry().push_back({name, fn});
    }
};

} // namespace test
} // namespace qh

#define QH_TEST(name)                                              \
    static void name();                                            \
    static ::qh::test::Registrar qh_reg_##name(#name, &name);      \
    static void name()

#define QH_CHECK(cond)                                                      \
    do {                                                                    \
        ::qh::test::totalCount()++;                                         \
        if (!(cond)) {                                                      \
            ::qh::test::failCount()++;                                      \
            std::cerr << "  FAIL: " << #cond << " (" << __FILE__ << ":"     \
                      << __LINE__ << ")\n";                                 \
        }                                                                   \
    } while (0)

#define QH_CHECK_EQ(a, b)                                                   \
    do {                                                                    \
        ::qh::test::totalCount()++;                                         \
        auto _qhA = (a);                                                    \
        auto _qhB = (b);                                                    \
        if (!(_qhA == _qhB)) {                                              \
            ::qh::test::failCount()++;                                      \
            std::cerr << "  FAIL: " << #a << " != " << #b << " ("           \
                      << __FILE__ << ":" << __LINE__ << ")\n"               \
                      << "    lhs=[" << _qhA << "]\n"                       \
                      << "    rhs=[" << _qhB << "]\n";                      \
        }                                                                   \
    } while (0)
