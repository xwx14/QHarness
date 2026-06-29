#include "TestHarness.h"
#include "tool/ReadFileTool.h"
#include "tool/PathCodec.h"
#include "schema/Message.h"
#include <filesystem>
#include <fstream>
#include <initializer_list>
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

// 写原始字节到临时工作区文件（构造 GBK/UTF-16 等非 UTF-8 编码的典型文本文件）
fs::path makeWorkDirBinary(const std::string& fileName, const std::string& rawBytes) {
    fs::path dir = fs::temp_directory_path() / "qh_readfile_test";
    fs::create_directories(dir);
    std::ofstream ofs(dir / fileName, std::ios::binary);
    ofs.write(rawBytes.data(), static_cast<std::streamsize>(rawBytes.size()));
    return dir;
}

// 字节数组 → std::string
std::string bytes(std::initializer_list<unsigned char> bs) {
    return std::string(reinterpret_cast<const char*>(bs.begin()), bs.size());
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

// === 典型文本文件读取回归测试：各编码端到端转 UTF-8，防止 ReadFileTool 内容编码处理回归 ===
QH_TEST(readfiletool_reads_ascii_file) {
    fs::path dir = makeWorkDirBinary("ascii.txt", "plain ascii text");
    qh::tool::ReadFileTool t(dir.string());
    qh::schema::ToolCall call;
    call._id = "t1"; call._name = "read_file";
    call._arguments = "{\"path\":\"ascii.txt\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(r._output, std::string("plain ascii text"));
}

QH_TEST(readfiletool_reads_utf8_with_bom) {
    // EF BB BF + "测试"(UTF-8)
    std::string content = bytes({0xEF, 0xBB, 0xBF}) + std::string("测试");
    fs::path dir = makeWorkDirBinary("utf8bom.txt", content);
    qh::tool::ReadFileTool t(dir.string());
    qh::schema::ToolCall call;
    call._id = "t2"; call._name = "read_file";
    call._arguments = "{\"path\":\"utf8bom.txt\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(r._output, std::string("测试"));  // BOM 剥离
}

QH_TEST(readfiletool_reads_gbk_file) {
    // "测试代码" GBK = B2 E2 CA D4 B4 FA C2 EB
    std::string gbk = bytes({0xB2, 0xE2, 0xCA, 0xD4, 0xB4, 0xFA, 0xC2, 0xEB});
    fs::path dir = makeWorkDirBinary("gbk.txt", gbk);
    qh::tool::ReadFileTool t(dir.string());
    qh::schema::ToolCall call;
    call._id = "t3"; call._name = "read_file";
    call._arguments = "{\"path\":\"gbk.txt\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(r._output, std::string("测试代码"));  // GBK→UTF-8
}

QH_TEST(readfiletool_reads_utf16le_file) {
    // BOM(FF FE) + "测试"(LE: 测=4B 6D, 试=D5 8B)
    std::string u16 = bytes({0xFF, 0xFE, 0x4B, 0x6D, 0xD5, 0x8B});
    fs::path dir = makeWorkDirBinary("utf16le.txt", u16);
    qh::tool::ReadFileTool t(dir.string());
    qh::schema::ToolCall call;
    call._id = "t4"; call._name = "read_file";
    call._arguments = "{\"path\":\"utf16le.txt\"}";
    qh::schema::ToolResult r = t.execute(call);
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(r._output, std::string("测试"));  // UTF-16 LE→UTF-8
}
