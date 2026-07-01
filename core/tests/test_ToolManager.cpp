#include "TestHarness.h"
#include "tool/Tool.h"
#include "tool/ToolManager.h"
#include "tool/ReadFileTool.h"
#include "tool/WriteFileTool.h"
#include "tool/EditFileTool.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>
#include <future>
#include <map>
#include <algorithm>

using nlohmann::json;

namespace {

// 测试用桩工具：definition() 返回固定定义，execute() 回填固定输出
class FakeTool : public qh::tool::Tool {
public:
    FakeTool(std::string name, std::string output = "ok")
        : Tool(makeDef(name)), _output(std::move(output)) {}

    qh::schema::ToolResult execute(const qh::schema::ToolCall& call) override {
        qh::schema::ToolResult r;
        r._toolCallId = call._id;
        r._output = _output;
        r._isError = false;
        return r;
    }

private:
    static qh::schema::ToolDefinition makeDef(const std::string& name) {
        qh::schema::ToolDefinition d;
        d._name = name;
        d._description = "fake tool for test";
        d._inputSchema = json::object();
        return d;
    }
    std::string _output;
};

// 构造一个带 id/name 的 ToolCall，参数给空 JSON 对象
qh::schema::ToolCall makeCall(const std::string& id, const std::string& name) {
    qh::schema::ToolCall c;
    c._id = id;
    c._name = name;
    c._arguments = "{}";
    return c;
}

} // namespace

namespace fs = std::filesystem;
namespace {

fs::path probeWorkDir() {
    fs::path dir = fs::temp_directory_path() / "qh_reskey_test";
    fs::create_directories(dir);
    return dir;
}
qh::schema::ToolCall callWithPath(const std::string& path) {
    qh::schema::ToolCall c;
    c._name = "x";
    c._arguments = std::string("{\"path\":\"") + path + "\"}";
    return c;
}
} // namespace

QH_TEST(resourceKey_file_tools_same_path_same_key) {
    const fs::path dir = probeWorkDir();
    qh::tool::ReadFileTool r(dir.string());
    qh::tool::WriteFileTool w(dir.string());
    qh::tool::EditFileTool e(dir.string());
    auto kr = r.resourceKey(callWithPath("a.txt"));
    auto kw = w.resourceKey(callWithPath("a.txt"));
    auto ke = e.resourceKey(callWithPath("a.txt"));
    QH_CHECK(kr.has_value() && kw.has_value() && ke.has_value());
    QH_CHECK_EQ(*kr, *kw);    // read/write 同 path → 同 key（读写互斥）
    QH_CHECK_EQ(*kr, *ke);
}

QH_TEST(resourceKey_file_tools_diff_path_diff_key) {
    const fs::path dir = probeWorkDir();
    qh::tool::ReadFileTool r(dir.string());
    auto k1 = r.resourceKey(callWithPath("a.txt"));
    auto k2 = r.resourceKey(callWithPath("b.txt"));
    QH_CHECK(k1.has_value() && k2.has_value());
    QH_CHECK(*k1 != *k2);
}

QH_TEST(resourceKey_file_tools_missing_or_traversal_returns_nullopt) {
    const fs::path dir = probeWorkDir();
    qh::tool::ReadFileTool r(dir.string());
    qh::schema::ToolCall noPath; noPath._arguments = "{}";
    QH_CHECK(!r.resourceKey(noPath).has_value());                  // 缺 path
    QH_CHECK(!r.resourceKey(callWithPath("../../../../etc/passwd")).has_value());  // 越界
}

QH_TEST(toolmanager_register_and_lookup) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");

    QH_CHECK(tm.registerTool(bash));
    QH_CHECK(tm.hasTool("bash"));
    qh::tool::Tool* found = tm.getTool("bash");
    QH_CHECK(found != nullptr);
    if (found) {
        QH_CHECK_EQ(std::string(found->definition()._name), std::string("bash"));
    }
}

QH_TEST(toolmanager_register_duplicate_not_replaced) {
    qh::tool::ToolManager tm;
    FakeTool a("dup", "a-out");
    FakeTool b("dup", "b-out");

    QH_CHECK(tm.registerTool(a));
    QH_CHECK(!tm.registerTool(b));  // 重名返回 false，不覆盖

    // 执行应走 a（未被 b 替换）
    auto r = tm.execute(makeCall("c1", "dup"));
    QH_CHECK_EQ(r._output, std::string("a-out"));
}

QH_TEST(toolmanager_list_definitions) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");
    FakeTool edit("edit");
    tm.registerTool(bash);
    tm.registerTool(edit);

    auto defs = tm.getAvailableTools();
    QH_CHECK_EQ(defs.size(), static_cast<size_t>(2));

    // 集合语义：包含 bash 和 edit（unordered_map 顺序不保证）
    bool hasBash = false, hasEdit = false;
    for (const auto& d : defs) {
        if (d._name == "bash") hasBash = true;
        if (d._name == "edit") hasEdit = true;
    }
    QH_CHECK(hasBash);
    QH_CHECK(hasEdit);
}

QH_TEST(toolmanager_execute_dispatches_to_tool) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash", "ls-output");
    tm.registerTool(bash);

    auto r = tm.execute(makeCall("call_1", "bash"));
    QH_CHECK_EQ(r._toolCallId, std::string("call_1"));
    QH_CHECK_EQ(r._output, std::string("ls-output"));
    QH_CHECK(!r._isError);
}

QH_TEST(toolmanager_execute_unknown_returns_error) {
    qh::tool::ToolManager tm;
    auto r = tm.execute(makeCall("call_2", "nope"));

    QH_CHECK(r._isError);
    QH_CHECK_EQ(r._toolCallId, std::string("call_2"));
    QH_CHECK(r._output.find("nope") != std::string::npos);  // 错误信息含工具名
}

QH_TEST(toolmanager_unregister) {
    qh::tool::ToolManager tm;
    FakeTool bash("bash");
    tm.registerTool(bash);

    QH_CHECK(tm.unregisterTool("bash"));          // 注销已存在：true
    QH_CHECK(!tm.hasTool("bash"));                // 注销后查不到
    QH_CHECK(tm.getTool("bash") == nullptr);
    QH_CHECK_EQ(tm.getAvailableTools().size(), static_cast<size_t>(0));

    QH_CHECK(!tm.unregisterTool("bash"));         // 再次注销：false
    QH_CHECK(!tm.unregisterTool("never"));        // 注销不存在的工具：false
}

#ifdef _WIN32
#include "tool/WinBashTool.h"
QH_TEST(resourceKey_winbash_returns_global_key) {
    qh::tool::WinBashTool b(".");          // resourceKey 不依赖工作目录
    qh::schema::ToolCall c; c._name = "bash"; c._arguments = R"({"command":"ls"})";
    auto k = b.resourceKey(c);
    QH_CHECK(k.has_value());
    QH_CHECK_EQ(*k, std::string("__bash__"));
}
#endif

// ===== executeAll 分桶 Fork-Join 测试 =====

namespace {

// 探针工具：可配置 resourceKey、是否 sleep、是否抛异常；记录同时在跑的峰值（测并发度）
class ProbeTool : public qh::tool::Tool {
public:
    ProbeTool(std::string name, std::optional<std::string> key,
              int sleepMs = 0, bool throwOnExec = false)
        : Tool(makeDef(name)), _key(std::move(key)), _sleepMs(sleepMs), _throw(throwOnExec) {}

    std::optional<std::string> resourceKey(const qh::schema::ToolCall&) const override { return _key; }

    qh::schema::ToolResult execute(const qh::schema::ToolCall& call) override {
        if (_throw) throw std::runtime_error("probe boom");
        int cur = ++active();
        int p = peak().load();
        while (cur > p && !peak().compare_exchange_weak(p, cur)) {}
        if (_sleepMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(_sleepMs));
        --active();
        qh::schema::ToolResult r;
        r._toolCallId = call._id;
        r._output = "probe-" + call._id;
        r._isError = false;
        return r;
    }
    static void resetProbe() { active() = 0; peak() = 0; }
    static int peakValue() { return peak().load(); }
private:
    static std::atomic<int>& active() { static std::atomic<int> a{0}; return a; }
    static std::atomic<int>& peak()   { static std::atomic<int> p{0}; return p; }
    static qh::schema::ToolDefinition makeDef(const std::string& name) {
        qh::schema::ToolDefinition d; d._name = name; d._description = "probe"; d._inputSchema = json::object();
        return d;
    }
    std::optional<std::string> _key;
    int _sleepMs;
    bool _throw;
};

// 读改写计数工具（非原子，模拟"文件读改写"）：仅当被串行执行时 counter 才正确累加
class RacyCounterTool : public qh::tool::Tool {
public:
    RacyCounterTool() : Tool(makeDef()) {}
    std::optional<std::string> resourceKey(const qh::schema::ToolCall&) const override { return "same-file"; }
    qh::schema::ToolResult execute(const qh::schema::ToolCall& call) override {
        int c = counter();                 // 读
        std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 放大竞争窗口
        counter() = c + 1;                 // 写
        qh::schema::ToolResult r; r._toolCallId = call._id; r._output = "ok"; return r;
    }
    static int value() { return counter(); }
    static void reset() { counter() = 0; }
private:
    static int& counter() { static int c = 0; return c; }
    static qh::schema::ToolDefinition makeDef() {
        qh::schema::ToolDefinition d; d._name = "racy"; d._description = "racy"; d._inputSchema = json::object();
        return d;
    }
};

} // namespace

QH_TEST(executeAll_preserves_order) {
    qh::tool::ToolManager tm;
    ProbeTool a("a", std::nullopt, 0), b("b", std::nullopt, 0), c("c", std::nullopt, 0);
    tm.registerTool(a); tm.registerTool(b); tm.registerTool(c);
    std::vector<qh::schema::ToolCall> calls{makeCall("1","a"), makeCall("2","b"), makeCall("3","c")};
    auto rs = tm.executeAll(calls, 0);
    QH_CHECK_EQ(rs.size(), (size_t)3);
    QH_CHECK_EQ(rs[0]._toolCallId, std::string("1")); QH_CHECK_EQ(rs[0]._output, std::string("probe-1"));
    QH_CHECK_EQ(rs[1]._toolCallId, std::string("2"));
    QH_CHECK_EQ(rs[2]._toolCallId, std::string("3"));
}

QH_TEST(executeAll_empty_calls) {
    qh::tool::ToolManager tm;
    auto rs = tm.executeAll({}, 0);
    QH_CHECK_EQ(rs.size(), (size_t)0);
}

QH_TEST(executeAll_diff_keys_run_concurrently) {
    qh::tool::ToolManager tm;
    ProbeTool a("a", "ka", 50), b("b", "kb", 50), c("c", "kc", 50);   // 不同 key，各 sleep 50ms
    tm.registerTool(a); tm.registerTool(b); tm.registerTool(c);
    ProbeTool::resetProbe();
    auto rs = tm.executeAll({makeCall("1","a"),makeCall("2","b"),makeCall("3","c")}, 0);
    QH_CHECK_EQ(rs.size(), (size_t)3);
    QH_CHECK(ProbeTool::peakValue() >= 2);                 // 真并发（峰值>=2，不依赖 timing 避免 CI flaky）
}

QH_TEST(executeAll_same_key_run_serially_no_lost_update) {
    qh::tool::ToolManager tm;
    RacyCounterTool r; tm.registerTool(r);                 // resourceKey 恒 "same-file"
    RacyCounterTool::reset();
    auto rs = tm.executeAll({makeCall("1","racy"),makeCall("2","racy"),makeCall("3","racy")}, 0);
    QH_CHECK_EQ(rs.size(), (size_t)3);
    QH_CHECK_EQ(RacyCounterTool::value(), 3);              // 串行→无 lost update，计数==3
}

QH_TEST(executeAll_throwing_tool_isolated_as_error) {
    qh::tool::ToolManager tm;
    ProbeTool boom("boom", std::nullopt, 0, true), ok("ok", std::nullopt, 0, false);
    tm.registerTool(boom); tm.registerTool(ok);
    auto rs = tm.executeAll({makeCall("1","boom"),makeCall("2","ok")}, 0);
    QH_CHECK_EQ(rs.size(), (size_t)2);
    QH_CHECK(rs[0]._isError);                              // 抛异常→isError，不炸进程
    QH_CHECK_EQ(rs[1]._toolCallId, std::string("2"));      // 其他工具正常
    QH_CHECK(!rs[1]._isError);
}

QH_TEST(executeAll_bash_bucket_serial) {
    qh::tool::ToolManager tm;
    ProbeTool b1("b1", "__bash__", 30), b2("b2", "__bash__", 30);  // 同 __bash__ 桶
    tm.registerTool(b1); tm.registerTool(b2);
    ProbeTool::resetProbe();
    tm.executeAll({makeCall("1","b1"),makeCall("2","b2")}, 0);
    QH_CHECK_EQ(ProbeTool::peakValue(), 1);                // bash 桶串行
}

QH_TEST(executeAll_max_concurrency_limits_active) {
    qh::tool::ToolManager tm;
    ProbeTool a("a","ka",40), b("b","kb",40), c("c","kc",40), d("d","kd",40), e("e","ke",40);
    for (auto* t : {&a,&b,&c,&d,&e}) tm.registerTool(*t);
    ProbeTool::resetProbe();
    tm.executeAll({makeCall("1","a"),makeCall("2","b"),makeCall("3","c"),makeCall("4","d"),makeCall("5","e")}, 2);
    QH_CHECK(ProbeTool::peakValue() <= 2);                 // 限流：同时活跃<=2
}
