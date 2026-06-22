#pragma once
#include "memory/Memory.h"
#include <string>

namespace qh {
namespace memory {

// 基于文件的记忆系统（骨架）
class MemoryFile : public Memory {
public:
    explicit MemoryFile(std::string basePath) : basePath_(std::move(basePath)) {}

    void load() override {
        // TODO: 从 basePath 读取记忆
    }
    void save() override {
        // TODO: 将记忆写入 basePath
    }

private:
    std::string basePath_;
};

} // namespace memory
} // namespace qh
