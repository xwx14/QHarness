#ifndef QH_TOOL_PATHCODEC_H
#define QH_TOOL_PATHCODEC_H

// 跨平台文件名编码工具（参考 CSDN《C++：中文编码转换》的 C++11 标准库方案）。
// 用 std::wstring_convert + std::codecvt_utf8 / std::codecvt_byname 完成
// UTF-8 ↔ wstring ↔ GBK 转换，一套代码跨 Windows/Linux（仅 GBK locale 名不同），
// 无需 Windows API(MultiByteToWideChar) 或 iconv。
//
// 背景：与 AI 对话的内容（含 tool_call 的 path）统一为 UTF-8；但文件名在不同平台/
// 代码页下解析方式不同，直接用 UTF-8 字节构造 fs::path 会损坏中文路径。
//
// 注：<codecvt>/wstring_convert 在 C++17 被标记弃用，C++ 标准尚未给出替代，业界仍普遍
//     使用；此处抑制弃用警告。全 inline，编译进各使用方（含 tests），无需 QH_API 导出。

// 抑制 <codecvt> 的 C++17 弃用警告（MSVC C4996 / GCC·Clang -Wdeprecated-declarations）
#ifdef _MSC_VER
#  ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#    define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#  endif
#endif
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <filesystem>
#include <locale>
#include <codecvt>
#include <string>
#include <vector>

namespace qh {
namespace tool {

namespace fs = std::filesystem;
namespace pathCodec {

// GBK locale 名：Windows 用代码页 .936，Linux 用 zh_CN.GBK
#ifdef _WIN32
inline constexpr const char* kGbkLocale = ".936";
#else
inline constexpr const char* kGbkLocale = "zh_CN.GBK";
#endif

// UTF-8 → wstring（wchar_t 在 Windows 为 UTF-16，Linux 通常为 UTF-32）
inline std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.from_bytes(utf8);
    } catch (...) {
        return std::wstring();
    }
}

// wstring → UTF-8
inline std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.to_bytes(wide);
    } catch (...) {
        return std::string();
    }
}

// wstring → GBK（用 codecvt_byname 按本地 GBK locale 编码）
inline std::string wideToGbk(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    try {
        using F = std::codecvt_byname<wchar_t, char, std::mbstate_t>;
        std::wstring_convert<F> conv(new F(kGbkLocale));
        return conv.to_bytes(wide);
    } catch (...) {
        return std::string();  // locale 不存在或字符不可转换
    }
}

// GBK → wstring
inline std::wstring gbkToWide(const std::string& gbk) {
    if (gbk.empty()) return std::wstring();
    try {
        using F = std::codecvt_byname<wchar_t, char, std::mbstate_t>;
        std::wstring_convert<F> conv(new F(kGbkLocale));
        return conv.from_bytes(gbk);
    } catch (...) {
        return std::wstring();
    }
}

// UTF-8 → GBK
inline std::string utf8ToGbk(const std::string& utf8) {
    return wideToGbk(utf8ToWide(utf8));
}

// GBK → UTF-8
inline std::string gbkToUtf8(const std::string& gbk) {
    return wideToUtf8(gbkToWide(gbk));
}

// 由 path 字符串构造候选 fs::path 列表（平台自适应编码策略）。
// 调用方逐个尝试（穿越检查 + 文件存在性），取首个命中者。
inline std::vector<fs::path> candidatePaths(const std::string& pathStr) {
    std::vector<fs::path> candidates;
    if (pathStr.empty()) return candidates;
#ifdef _WIN32
    // GBK 优先（中文 Windows 本地代码页 936）；UTF-8→wide 兜底（含 GBK 外字符时）
    const std::string gbk = utf8ToGbk(pathStr);
    if (!gbk.empty()) candidates.push_back(fs::path(gbk));
    candidates.push_back(fs::path(utf8ToWide(pathStr)));
#else
    // 原始字节（UTF-8 主流）优先；GBK→UTF-8 回退
    candidates.push_back(fs::path(pathStr));
    const std::string u8 = gbkToUtf8(pathStr);
    if (!u8.empty() && u8 != pathStr) candidates.push_back(fs::path(u8));
#endif
    return candidates;
}

} // namespace pathCodec
} // namespace tool
} // namespace qh

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#endif // QH_TOOL_PATHCODEC_H
