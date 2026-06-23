#ifndef QH_MEMORY_FILE_H
#define QH_MEMORY_FILE_H
#include "memory/Memory.h"
#include "qh_export.h"
#include <string>

namespace qh {
namespace memory {

// 基于文件的记忆系统（骨架）
class QH_API MemoryFile : public Memory {
public:
    explicit MemoryFile(std::string basePath);

    void load() override;
    void save() override;

private:
    std::string basePath_;
};

} // namespace memory
} // namespace qh
#endif // QH_MEMORY_FILE_H
