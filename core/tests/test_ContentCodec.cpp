#include "TestHarness.h"
#include "tool/ContentCodec.h"
#include <initializer_list>
#include <string>

namespace {
// 字节数组 → std::string（规避字符串字面量里 \x 被当结束符等陷阱）
std::string bytes(std::initializer_list<unsigned char> bs) {
    return std::string(reinterpret_cast<const char*>(bs.begin()), bs.size());
}
} // namespace

QH_TEST(contentcodec_valid_utf8) {
    QH_CHECK(qh::tool::contentCodec::isValidUtf8("hello"));
    QH_CHECK(qh::tool::contentCodec::isValidUtf8(bytes({0xE6, 0xB5, 0x8B})));   // "测"
    QH_CHECK(qh::tool::contentCodec::isValidUtf8(std::string()));               // 空
}

QH_TEST(contentcodec_invalid_utf8) {
    QH_CHECK(!qh::tool::contentCodec::isValidUtf8(bytes({0xFF})));              // 非法 leading
    QH_CHECK(!qh::tool::contentCodec::isValidUtf8(bytes({0xE6, 0xB5})));        // 残缺 3 字节序列
    QH_CHECK(!qh::tool::contentCodec::isValidUtf8(bytes({0xC0, 0x80})));        // overlong NUL
    QH_CHECK(!qh::tool::contentCodec::isValidUtf8(bytes({0xED, 0xA0, 0x80})));  // 代理码点 U+D800
}

QH_TEST(contentcodec_utf8_passthrough) {
    std::string u8 = bytes({0xE6, 0xB5, 0x8B, 0xE8, 0xAF, 0x95});     // "测试"
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8(u8), u8);
}

QH_TEST(contentcodec_utf8_bom_stripped) {
    std::string withBom = bytes({0xEF, 0xBB, 0xBF, 0xE6, 0xB5, 0x8B});  // BOM + "测"
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8(withBom), bytes({0xE6, 0xB5, 0x8B}));
}

QH_TEST(contentcodec_gbk_to_utf8) {
    std::string gbk = bytes({0xB2, 0xE2, 0xCA, 0xD4});               // "测试" GBK
    std::string u8 = bytes({0xE6, 0xB5, 0x8B, 0xE8, 0xAF, 0x95});
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8(gbk), u8);
}

QH_TEST(contentcodec_utf16le_to_utf8) {
    std::string utf16le = bytes({0xFF, 0xFE, 0x4B, 0x6D});           // BOM + "测"(LE: 4B 6D)
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8(utf16le), bytes({0xE6, 0xB5, 0x8B}));
}

QH_TEST(contentcodec_utf16be_to_utf8) {
    std::string utf16be = bytes({0xFE, 0xFF, 0x6D, 0x4B});           // BOM + "测"(BE: 6D 4B)
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8(utf16be), bytes({0xE6, 0xB5, 0x8B}));
}

QH_TEST(contentcodec_ascii_passthrough) {
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8("hello world"), std::string("hello world"));
}

QH_TEST(contentcodec_invalid_falls_back_raw) {
    // 单 0xFF：非合法 UTF-8，GBK 也非法 → gbkToUtf8 抛异常返回空 → 原样兜底
    std::string bin = bytes({0xFF});
    QH_CHECK_EQ(qh::tool::contentCodec::toUtf8(bin), bin);
}

QH_TEST(contentcodec_truncate_at_char_boundary) {
    std::string u8 = bytes({0xE6, 0xB5, 0x8B, 0xE8, 0xAF, 0x95});     // "测试"(6 字节)
    // 截到 4 字节：落在"试"字符中间，应回退到 3 字节(完整"测")
    QH_CHECK_EQ(qh::tool::contentCodec::truncateUtf8Safe(u8, 4), bytes({0xE6, 0xB5, 0x8B}));
    // 截到 3 字节：恰好多到字符边界
    QH_CHECK_EQ(qh::tool::contentCodec::truncateUtf8Safe(u8, 3), bytes({0xE6, 0xB5, 0x8B}));
    // 短于阈值：原样
    QH_CHECK_EQ(qh::tool::contentCodec::truncateUtf8Safe(u8, 100), u8);
}
