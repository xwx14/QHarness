#include "TestHarness.h"
#include "tool/ReadFileTool.h"
#include "schema/Message.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {
// 在临时目录下建唯一工作区，写入一个文件，返回工作区路径
fs::path makeWorkDir(const std::string& fileName, const std::string& fileContent) {
    fs::path dir = fs::temp_directory_path() / "qh_readfile_test";
    fs::create_directories(dir);
    std::ofstream ofs(dir / fileName, std::ios::binary);
    ofs << fileContent;
    ofs.close();
    return dir;
}
} // namespace

QH_TEST(readfiletool_definition_name_is_read_file) {
    qh::tool::ReadFileTool t(".");
    QH_CHECK_EQ(t.definition()._name, std::string("read_file"));
}

QH_TEST(readfiletool_reads_existing_file) {
    fs::path dir = makeWorkDir("hello.txt", "你好喵～world");
    qh::tool::ReadFileTool t(dir.string());

    qh::schema::ToolCall call;
    call._id = "call_1";
    call._name = "read_file";
    call._arguments = "{\"path\":\"hello.txt\"}";

    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK_EQ(r._toolCallId, std::string("call_1"));
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(r._output, std::string("你好喵～world"));
}

QH_TEST(readfiletool_missing_file_returns_error) {
    fs::path dir = makeWorkDir("exist.txt", "x");
    qh::tool::ReadFileTool t(dir.string());

    qh::schema::ToolCall call;
    call._id = "call_2";
    call._arguments = "{\"path\":\"nope.txt\"}";

    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);
}

QH_TEST(readfiletool_bad_args_returns_error) {
    qh::tool::ReadFileTool t(".");
    qh::schema::ToolCall call;
    call._id = "call_3";
    call._arguments = "{\"wrong\":123}";  // 缺 path

    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);
}

QH_TEST(readfiletool_path_traversal_is_rejected) {
    fs::path dir = makeWorkDir("inside.txt", "secret");
    qh::tool::ReadFileTool t(dir.string());

    qh::schema::ToolCall call;
    call._id = "call_4";
    call._arguments = "{\"path\":\"../../../../etc/passwd\"}";

    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);
}

QH_TEST(readfiletool_truncates_long_content) {
    // 写入超过 kMaxLen 的内容（全 ASCII，按字节截断边界确定）
    const std::size_t big = qh::tool::ReadFileTool::kMaxLen + 500;
    std::string longContent(big, 'A');
    fs::path dir = makeWorkDir("big.txt", longContent);
    qh::tool::ReadFileTool t(dir.string());

    qh::schema::ToolCall call;
    call._id = "call_5";
    call._arguments = "{\"path\":\"big.txt\"}";

    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    // 前 kMaxLen 字节为 'A'，其后追加截断提示，故总长 > kMaxLen 但远小于原始 big
    QH_CHECK(r._output.size() > qh::tool::ReadFileTool::kMaxLen);
    QH_CHECK(r._output.size() < big);
    QH_CHECK(r._output.find("已被系统截断") != std::string::npos);
}
