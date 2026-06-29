#ifndef QH_TOOL_BASH_H
#define QH_TOOL_BASH_H

// BashTool：编译期按当前系统选用对应平台的命令执行工具。
// 单平台二进制只含一个实现（Windows→WinBashTool / macOS→MacBashTool / Linux→LinuxBashTool）。
#ifdef _WIN32
    #include "tool/WinBashTool.h"
#elif defined(__APPLE__)
    #include "tool/MacBashTool.h"
#elif defined(__linux__)
    #include "tool/LinuxBashTool.h"
#else
    #error "BashTool: 不支持的平台（需 _WIN32 / __APPLE__ / __linux__ 之一）"
#endif

namespace qh {
namespace tool {
#ifdef _WIN32
    using BashTool = WinBashTool;
#elif defined(__APPLE__)
    using BashTool = MacBashTool;
#elif defined(__linux__)
    using BashTool = LinuxBashTool;
#endif
} // namespace tool
} // namespace qh

#endif // QH_TOOL_BASH_H
