#include "TestHarness.h"
#include "tool/WinBashTool.h"
#include "schema/Message.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {
fs::path makeWorkDir() {
    fs::path dir = fs::temp_directory_path() / "qh_bash_test";
    fs::create_directories(dir);
    return dir;
}
// 在输出里找子串（输出含 exit code 行，整体判断）
bool outHas(const std::string& out, const std::string& sub) {
    return out.find(sub) != std::string::npos;
}
} // namespace

QH_TEST(winbashtool_echo_output_and_exitcode_zero) {
    fs::path dir = makeWorkDir();
    qh::tool::WinBashTool t(dir.string());
    qh::schema::ToolCall call; call._id = "b1"; call._name = "bash";
    call._arguments = "{\"command\":\"echo hello\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK(outHas(r._output, "hello"));
    QH_CHECK(outHas(r._output, "[exit code: 0]"));
}

QH_TEST(winbashtool_nonzero_exitcode) {
    fs::path dir = makeWorkDir();
    qh::tool::WinBashTool t(dir.string());
    qh::schema::ToolCall call; call._id = "b2"; call._name = "bash";
    call._arguments = "{\"command\":\"exit 7\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);   // 退出码非 0
    QH_CHECK(outHas(r._output, "[exit code: 7]"));
}

QH_TEST(winbashtool_runs_in_workdir) {
    fs::path dir = makeWorkDir();
    qh::tool::WinBashTool t(dir.string());
    qh::schema::ToolCall call; call._id = "b3"; call._name = "bash";
    call._arguments = "{\"command\":\"cd\"}";   // cd 无参打印当前目录
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    // 当前目录应为工作目录（小写化比较，cd 输出大小写依系统）
    QH_CHECK(outHas(r._output, dir.filename().string()));
}

QH_TEST(winbashtool_timeout_kills_process) {
    fs::path dir = makeWorkDir();
    qh::tool::WinBashTool t(dir.string());
    qh::schema::ToolCall call; call._id = "b4"; call._name = "bash";
    // ping 长时间运行；设 500ms 超时
    call._arguments = "{\"command\":\"ping -n 10 127.0.0.1\",\"timeout_ms\":500}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);   // 超时
    QH_CHECK(outHas(r._output, "已被终止"));
}

QH_TEST(winbashtool_missing_command_param) {
    qh::tool::WinBashTool t(".");
    qh::schema::ToolCall call; call._id = "b5"; call._name = "bash";
    call._arguments = "{\"wrong\":1}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);
}
