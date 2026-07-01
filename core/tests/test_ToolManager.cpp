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
