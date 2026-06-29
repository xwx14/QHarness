#include "tool/Tool.h"
#include "tool/PathCodec.h"
#include <system_error>

namespace qh {
namespace tool {

namespace fs = std::filesystem;

std::filesystem::path Tool::resolveWorkBase() const {
    // _workDir(UTF-8) 按平台编码生成候选，取首个可规范化的绝对基目录
    for (const fs::path& cand : pathCodec::candidatePaths(_workDir)) {
        std::error_code ec;
        fs::path base = fs::weakly_canonical(cand, ec);
        if (!ec) return base;
    }
    return fs::path();
}

std::optional<fs::path> Tool::resolveInside(const fs::path& base,
                                            const std::string& relPath) const {
    // relPath 的多种编码候选逐个尝试：穿越检查（位于 base 内）+ 文件存在，取首个命中
    for (const fs::path& cand : pathCodec::candidatePaths(relPath)) {
        std::error_code ec;
        const fs::path full = fs::weakly_canonical(base / cand, ec);
        if (ec) continue;
        const fs::path rel = fs::relative(full, base, ec);
        // rel 为空或以 ".." 开头 → 越出工作目录
        if (ec || rel.empty() ||
            rel.native().rfind(fs::path("..").native(), 0) == 0) {
            continue;
        }
        std::error_code existEc;
        if (fs::exists(full, existEc) && !existEc) return full;
    }
    return std::nullopt;
}

std::optional<fs::path> Tool::resolveInsideForWrite(const fs::path& base,
                                                    const std::string& relPath) const {
    // 写文件语义：穿越检查（位于 base 内）但不要求目标已存在。
    // 取首个能规范化且不越界的候选（写新文件无需命中已存在文件）。
    for (const fs::path& cand : pathCodec::candidatePaths(relPath)) {
        std::error_code ec;
        const fs::path full = fs::weakly_canonical(base / cand, ec);
        if (ec) continue;
        const fs::path rel = fs::relative(full, base, ec);
        if (ec || rel.empty() ||
            rel.native().rfind(fs::path("..").native(), 0) == 0) {
            continue;
        }
        return full;   // 不查 exists
    }
    return std::nullopt;
}

} // namespace tool
} // namespace qh
