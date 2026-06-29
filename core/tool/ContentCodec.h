#ifndef QH_TOOL_CONTENTCODEC_H
#define QH_TOOL_CONTENTCODEC_H

// 文件内容编码检测与转换：检测 UTF-8 / GBK / UTF-16(BOM)，统一转 UTF-8 返回。
// 与 PathCodec（路径名编码）职责分离；底层 UTF-8↔GBK 复用 pathCodec::gbkToUtf8。
// 全 inline，编译进各使用方（含 tests），无需 QH_API 导出。

#include "tool/PathCodec.h"
#include <cstdint>
#include <string>

namespace qh {
namespace tool {

namespace contentCodec {

// 码点 → UTF-8 字节（追加到 out）
inline void appendUtf8(std::string& out, unsigned cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {  // cp <= 0x10FFFF
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// UTF-16 字节序列(已去 BOM) → UTF-8。littleEndian 决定字节序。
// 手动解码（含 surrogate pair），不依赖 wchar_t 宽度，跨平台一致。
inline std::string utf16ToUtf8(const std::string& utf16, bool littleEndian) {
    std::string out;
    const size_t n = utf16.size();
    for (size_t i = 0; i + 1 < n; i += 2) {
        const unsigned char b0 = static_cast<unsigned char>(utf16[i]);
        const unsigned char b1 = static_cast<unsigned char>(utf16[i + 1]);
        const uint16_t u = littleEndian
            ? static_cast<uint16_t>(b0 | (b1 << 8))
            : static_cast<uint16_t>(b1 | (b0 << 8));
        // 高代理 + 紧随低代理 → 补充平面码点
        if (u >= 0xD800 && u <= 0xDBFF && i + 3 < n) {
            const unsigned char c0 = static_cast<unsigned char>(utf16[i + 2]);
            const unsigned char c1 = static_cast<unsigned char>(utf16[i + 3]);
            const uint16_t low = littleEndian
                ? static_cast<uint16_t>(c0 | (c1 << 8))
                : static_cast<uint16_t>(c1 | (c0 << 8));
            if (low >= 0xDC00 && low <= 0xDFFF) {
                appendUtf8(out, 0x10000 + ((u - 0xD800) << 10) + (low - 0xDC00));
                i += 2;   // 多消费低代理（循环再 +2）
                continue;
            }
            continue;     // 孤立高代理：跳过
        }
        if (u >= 0xDC00 && u <= 0xDFFF) continue;  // 孤立低代理：跳过
        appendUtf8(out, u);
    }
    return out;
}

// UTF-8 严格语法校验：合法返回 true（纯 ASCII 也合法）。
// 拒绝：残缺序列、非法 continuation、overlong、越界(>U+10FFFF)、代理码点。
inline bool isValidUtf8(const std::string& s) {
    const size_t n = s.size();
    size_t i = 0;
    while (i < n) {
        const unsigned char b = static_cast<unsigned char>(s[i]);
        if (b <= 0x7F) { ++i; continue; }                  // ASCII
        size_t need; unsigned cp; unsigned minCp;
        if      ((b & 0xE0) == 0xC0) { need = 1; cp = b & 0x1F; minCp = 0x80; }
        else if ((b & 0xF0) == 0xE0) { need = 2; cp = b & 0x0F; minCp = 0x800; }
        else if ((b & 0xF8) == 0xF0) { need = 3; cp = b & 0x07; minCp = 0x10000; }
        else return false;                                 // 非法 leading 字节
        if (i + need >= n) return false;                   // continuation 不够（残缺）
        for (size_t k = 1; k <= need; ++k) {
            const unsigned char c = static_cast<unsigned char>(s[i + k]);
            if ((c & 0xC0) != 0x80) return false;          // 非 continuation
            cp = (cp << 6) | (c & 0x3F);
        }
        if (cp < minCp) return false;                      // overlong
        if (cp > 0x10FFFF) return false;                   // 越界
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;    // 代理码点
        i += 1 + need;
    }
    return true;
}

// 截断 UTF-8 到不超过 maxBytes 字节，回退到最后一个完整字符边界（不切断多字节字符）。
inline std::string truncateUtf8Safe(const std::string& s, size_t maxBytes) {
    if (s.size() <= maxBytes) return s;
    size_t end = maxBytes;                 // end < s.size() 成立
    while (end > 0) {
        const unsigned char b = static_cast<unsigned char>(s[end]);
        if ((b & 0xC0) != 0x80) break;     // s[end] 是 leading/单字节 → end 是字符边界
        --end;                             // s[end] 是 continuation，属于前一字符，回退
    }
    return s.substr(0, end);
}

// 检测 raw 的编码并转为 UTF-8（剥离 BOM）。
// 顺序：BOM(UTF-8/UTF-16 LE·BE) → UTF-8 严格校验 → GBK → 原样兜底。
inline std::string toUtf8(const std::string& raw) {
    const size_t n = raw.size();
    // 1. BOM
    if (n >= 3 && (unsigned char)raw[0] == 0xEF && (unsigned char)raw[1] == 0xBB &&
        (unsigned char)raw[2] == 0xBF)
        return raw.substr(3);                              // UTF-8 BOM：剥离
    if (n >= 2 && (unsigned char)raw[0] == 0xFF && (unsigned char)raw[1] == 0xFE)
        return utf16ToUtf8(raw.substr(2), true);          // UTF-16 LE
    if (n >= 2 && (unsigned char)raw[0] == 0xFE && (unsigned char)raw[1] == 0xFF)
        return utf16ToUtf8(raw.substr(2), false);         // UTF-16 BE
    // 2. UTF-8 严格校验
    if (isValidUtf8(raw)) return raw;
    // 3. GBK
    const std::string u8 = pathCodec::gbkToUtf8(raw);
    if (!u8.empty()) return u8;
    // 4. 兜底：原样
    return raw;
}

} // namespace contentCodec
} // namespace tool
} // namespace qh
#endif // QH_TOOL_CONTENTCODEC_H
