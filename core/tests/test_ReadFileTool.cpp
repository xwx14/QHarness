#include "TestHarness.h"
#include "tool/ReadFileTool.h"
#include "tool/PathCodec.h"
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

QH_TEST(readfiletool_reads_file_with_non_ascii_name) {
    // 复现 Windows 文件名编码 bug：AI 返回的 path 参数为 UTF-8 中文。
    // 用 fs::u8path 按 UTF-8 正确创建真实中文名文件（独立于被测代码，确保文件名真实为中文），
    // 工具内部须同样按 UTF-8 解析路径才能命中该文件。
    fs::path dir = fs::temp_directory_path() / "qh_readfile_test";
    fs::create_directories(dir);
    {
        std::ofstream ofs(dir / fs::u8path("测试.txt"), std::ios::binary);
        ofs << "中文内容";
    }

    qh::tool::ReadFileTool t(dir.string());  // 临时目录为 ASCII 段，等价 UTF-8

    qh::schema::ToolCall call;
    call._id = "call_zh";
    call._name = "read_file";
    call._arguments = "{\"path\":\"测试.txt\"}";  // 源码 /utf-8，此处为 UTF-8 字节

    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(r._output, std::string("中文内容"));
}

#ifdef _WIN32
QH_TEST(pathcodec_utf8_to_gbk_chinese) {
    // "测试" UTF-8 = E6 B5 8B E8 AF 95；GBK(936) = B2 E2 CA D4
    const std::string gbk = qh::tool::pathCodec::utf8ToGbk("测试");
    QH_CHECK_EQ(gbk.size(), std::size_t(4));
    QH_CHECK_EQ((unsigned char)gbk[0], 0xB2u);
    QH_CHECK_EQ((unsigned char)gbk[1], 0xE2u);
    QH_CHECK_EQ((unsigned char)gbk[2], 0xCAu);
    QH_CHECK_EQ((unsigned char)gbk[3], 0xD4u);
}

QH_TEST(pathcodec_candidate_paths_windows_gbk_first) {
    // 中文 Windows 本地代码页为 GBK：候选首项 fs::path(gbk 字节) 按本地 GBK 解码后
    // 应等于 UTF-16 中文 path；次项为 UTF-8→wide 兜底。
    auto v = qh::tool::pathCodec::candidatePaths("测试.txt");
    QH_CHECK_EQ(v.size(), std::size_t(2));
    const fs::path expected = fs::path(L"测试.txt");
    QH_CHECK(v[0] == expected);
    QH_CHECK(v[1] == expected);
}
#endif
