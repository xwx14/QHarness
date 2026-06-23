#ifndef QH_EXPORT_H
#define QH_EXPORT_H

#ifdef _MSC_VER
// 导出含 STL 成员（std::string/std::vector/std::unordered_map 等）的类会触发
// C4251「需要 dll 接口」。core 与消费方（app/tests）使用相同的动态 CRT，
// 跨 DLL 边界传递 STL 类型安全，故在此集中抑制。
#pragma warning(disable: 4251)
#endif

// core 动态链接库导出宏：编译 core（定义 QH_BUILDING_DLL）时为 dllexport，
// 消费方（app/tests）为 dllimport。非 Windows 平台退化为空。
#ifdef _WIN32
  #ifdef QH_BUILDING_DLL
    #define QH_API __declspec(dllexport)
  #else
    #define QH_API __declspec(dllimport)
  #endif
#else
  #define QH_API
#endif

#endif // QH_EXPORT_H
