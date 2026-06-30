#include "TestHarness.h"
#include <stdexcept>
#include <string>

// 测试运行器自身的异常隔离能力：runSingle 应捕获用例抛出的异常而非让其传播，
// 从而保证一个用例崩溃不会拖垮整个测试进程（单用例崩溃隔离）。
// runSingle 设计为纯函数——不触碰全局 failCount/totalCount，便于在不污染回归基线的前提下自测。

QH_TEST(runsingle_normal_case_runs_and_passes_through) {
    static bool called;          // 无捕获 lambda 需经函数指针传递，用 static 标志观测
    called = false;
    qh::test::TestCase tc{"dummy_normal", []() { called = true; }};
    auto r = qh::test::runSingle(tc);
    QH_CHECK(called);            // 用例确实被执行
    QH_CHECK(!r.threw);          // 无异常
}

QH_TEST(runsingle_catches_std_exception) {
    qh::test::TestCase tc{"dummy_throws_std", []() { throw std::runtime_error("boom"); }};
    auto r = qh::test::runSingle(tc);
    QH_CHECK(r.threw);
    QH_CHECK(r.what == std::string("boom"));
}

QH_TEST(runsingle_catches_unknown_exception) {
    qh::test::TestCase tc{"dummy_throws_int", []() { throw 42; }};
    auto r = qh::test::runSingle(tc);
    QH_CHECK(r.threw);
    QH_CHECK(r.what == std::string("unknown exception"));
}
