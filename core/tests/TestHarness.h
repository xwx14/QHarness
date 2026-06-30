#ifndef QH_TEST_HARNESS_H
#define QH_TEST_HARNESS_H
#include <exception>
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

// 单用例运行结果：threw 标记是否抛出异常，what 记录异常说明
struct SingleResult {
    bool threw = false;
    std::string what;
};

// 运行单个用例并隔离其异常：捕获所有 C++ 异常，绝不让单个用例的抛出拖垮整个测试进程。
// 设计为纯函数——不触碰全局 failCount/totalCount，由调用方（TestMain）决定是否计入失败，
// 便于在不污染回归基线的前提下对运行器自身做单元测试。
inline SingleResult runSingle(const TestCase& tc) {
    SingleResult r;
    try {
        tc.fn();
    } catch (const std::exception& e) {
        r.threw = true;
        r.what = e.what();
    } catch (...) {
        r.threw = true;
        r.what = "unknown exception";
    }
    return r;
}

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
#endif // QH_TEST_HARNESS_H
