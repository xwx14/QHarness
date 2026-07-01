#include "TestHarness.h"
#include "tool/EditFileTool.h"
#include "schema/Message.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path makeWorkDir() {
    fs::path dir = fs::temp_directory_path() / "qh_editfile_test";
    fs::create_directories(dir);
    return dir;
}

// binary 写原始字节（构造 CRLF / GBK 等场景）
void writeFile(const fs::path& p, const std::string& content) {
    std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
}

std::string readBack(const fs::path& p) {
    std::ifstream ifs(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

// 构造 edit_file 工具调用（args 为完整 JSON 文本；换行须以 JSON 的 \n 转义传入）
qh::schema::ToolCall makeCall(const std::string& args) {
    qh::schema::ToolCall call;
    call._id = "e1";
    call._name = "edit_file";
    call._arguments = args;
    return call;
}

} // namespace

QH_TEST(editfiletool_definition_name_is_edit_file) {
    qh::tool::EditFileTool t(".");
    QH_CHECK_EQ(t.definition()._name, std::string("edit_file"));
}

// L1：精确唯一匹配替换
QH_TEST(editfiletool_exact_unique_replace) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "a.txt";
    writeFile(f, "alpha\nbeta\ngamma");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"beta\",\"new_text\":\"BETA\"}"));
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(f), std::string("alpha\nBETA\ngamma"));
}

// L1：多处匹配拒绝，且不改写文件
QH_TEST(editfiletool_multiple_matches_rejected) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "a.txt";
    writeFile(f, "foo\nfoo\nfoo");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"foo\",\"new_text\":\"bar\"}"));
    QH_CHECK(r._isError);
    QH_CHECK(r._output.find("匹配到了") != std::string::npos);
    QH_CHECK_EQ(readBack(f), std::string("foo\nfoo\nfoo"));
}

// L2：文件 CRLF、old_text 用 LF → 换行归一化后命中，写回统一为 LF
QH_TEST(editfiletool_crlf_normalization) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "a.txt";
    writeFile(f, "alpha\r\nbeta\r\ngamma");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"alpha\\nbeta\",\"new_text\":\"X\\nY\"}"));
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(f), std::string("X\nY\ngamma"));
}

// L3：old_text 前后带空白，trim 后唯一命中
QH_TEST(editfiletool_trim_space_match) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "a.txt";
    writeFile(f, "alpha\nbeta\ngamma");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"  beta  \",\"new_text\":\"B\"}"));
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(f), std::string("alpha\nB\ngamma"));
}

// L4：逐行去缩进匹配（old_text 整体因缩进不连续而非子串，落 L4 唯一命中）
QH_TEST(editfiletool_line_by_line_indent_match) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "a.txt";
    writeFile(f, "a\n    b\n    c\nd");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"b\\nc\",\"new_text\":\"X\\nY\"}"));
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(f), std::string("a\nX\nY\nd"));
}

// L4：未找到（精确/归一化/trim 均不命中，逐行也无匹配）
QH_TEST(editfiletool_not_found_rejected) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "a.txt";
    writeFile(f, "alpha\nbeta");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"zzz\",\"new_text\":\"qq\"}"));
    QH_CHECK(r._isError);
    QH_CHECK(r._output.find("未找到") != std::string::npos);
    QH_CHECK_EQ(readBack(f), std::string("alpha\nbeta"));
}

QH_TEST(editfiletool_missing_params_rejected) {
    qh::tool::EditFileTool t(".");
    auto r = t.execute(makeCall("{\"path\":\"a.txt\",\"old_text\":\"x\"}"));  // 缺 new_text
    QH_CHECK(r._isError);
}

QH_TEST(editfiletool_missing_file_rejected) {
    fs::path dir = makeWorkDir();
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"nope.txt\",\"old_text\":\"a\",\"new_text\":\"b\"}"));
    QH_CHECK(r._isError);
}

QH_TEST(editfiletool_traversal_rejected) {
    fs::path dir = makeWorkDir();
    writeFile(dir / "inside.txt", "data");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"../../../../tmp/qh_evil_edit.txt\",\"old_text\":\"a\",\"new_text\":\"b\"}"));
    QH_CHECK(r._isError);
}

QH_TEST(editfiletool_replaces_utf8_chinese) {
    fs::path dir = makeWorkDir();
    fs::path f = dir / "zh.txt";
    writeFile(f, "你好世界");
    qh::tool::EditFileTool t(dir.string());
    auto r = t.execute(makeCall("{\"path\":\"zh.txt\",\"old_text\":\"世界\",\"new_text\":\"Claude\"}"));
    QH_CHECK(!r._isError);
    QH_CHECK_EQ(readBack(f), std::string("你好Claude"));
}

// 编码处理回归：读 GBK → 转 UTF-8 匹配 → 写回 UTF-8
QH_TEST(editfiletool_edits_gbk_file_to_utf8) {
    // "测试代码" GBK(936) = B2 E2 CA D4 B4 FA C2 EB
    std::string gbk;
    static const unsigned char kGbk[] = {0xB2, 0xE2, 0xCA, 0xD4, 0xB4, 0xFA, 0xC2, 0xEB};
    gbk.append(reinterpret_cast<const char*>(kGbk), sizeof(kGbk));
    fs::path dir = makeWorkDir();
    fs::path f = dir / "gbk.txt";
    writeFile(f, gbk);
    qh::tool::EditFileTool t(dir.string());
    // old_text/new_text 为 UTF-8（AI 传入）；读出 GBK→UTF-8 后 "测试" 唯一匹配
    auto r = t.execute(makeCall("{\"path\":\"gbk.txt\",\"old_text\":\"测试\",\"new_text\":\"生产\"}"));
    QH_CHECK(!r._isError);
    // 写回为 UTF-8："生产代码"
    QH_CHECK_EQ(readBack(f), std::string("生产代码"));
}
