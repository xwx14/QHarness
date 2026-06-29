#include "TestHarness.h"
#include "tool/WriteFileTool.h"
#include "schema/Message.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

namespace {
fs::path makeWorkDir() {
    fs::path dir = fs::temp_directory_path() / "qh_writefile_test";
    fs::create_directories(dir);
    return dir;
}
std::string readBack(const fs::path& p) {
    std::ifstream ifs(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}
} // namespace

QH_TEST(writefiletool_creates_new_file) {
    fs::path dir = makeWorkDir();
    qh::tool::WriteFileTool t(dir.string());
    qh::schema::ToolCall call; call._id = "w1"; call._name = "write_file";
    call._arguments = "{\"path\":\"a.txt\",\"content\":\"hello\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(dir / "a.txt"), std::string("hello"));
}

QH_TEST(writefiletool_overwrites_existing) {
    fs::path dir = makeWorkDir();
    { std::ofstream ofs(dir / "b.txt", std::ios::binary); ofs << "OLD"; }
    qh::tool::WriteFileTool t(dir.string());
    qh::schema::ToolCall call; call._id = "w2"; call._name = "write_file";
    call._arguments = "{\"path\":\"b.txt\",\"content\":\"NEW\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(dir / "b.txt"), std::string("NEW"));
}

QH_TEST(writefiletool_creates_subdir) {
    fs::path dir = makeWorkDir();
    qh::tool::WriteFileTool t(dir.string());
    qh::schema::ToolCall call; call._id = "w3"; call._name = "write_file";
    call._arguments = "{\"path\":\"sub/deep/c.txt\",\"content\":\"x\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(dir / "sub" / "deep" / "c.txt"), std::string("x"));
}

QH_TEST(writefiletool_traversal_rejected) {
    fs::path dir = makeWorkDir();
    qh::tool::WriteFileTool t(dir.string());
    qh::schema::ToolCall call; call._id = "w4"; call._name = "write_file";
    call._arguments = "{\"path\":\"../../../../tmp/qh_evil.txt\",\"content\":\"x\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);  // 越界拒绝
}

QH_TEST(writefiletool_missing_params) {
    qh::tool::WriteFileTool t(".");
    qh::schema::ToolCall call; call._id = "w5"; call._name = "write_file";
    call._arguments = "{\"path\":\"a.txt\"}";  // 缺 content
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(r._isError);
}

QH_TEST(writefiletool_writes_utf8_chinese) {
    fs::path dir = makeWorkDir();
    qh::tool::WriteFileTool t(dir.string());
    qh::schema::ToolCall call; call._id = "w6"; call._name = "write_file";
    call._arguments = "{\"path\":\"zh.txt\",\"content\":\"中文内容\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(dir / "zh.txt"), std::string("中文内容"));
}
