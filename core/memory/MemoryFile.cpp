#include "memory/MemoryFile.h"

namespace qh {
namespace memory {

MemoryFile::MemoryFile(std::string basePath) : basePath_(std::move(basePath)) {}

void MemoryFile::load() {
    // TODO: 从 basePath 读取记忆
}

void MemoryFile::save() {
    // TODO: 将记忆写入 basePath
}

} // namespace memory
} // namespace qh
