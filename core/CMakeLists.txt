# 所有头文件一并列入源列表，使其在 VS 工程中可见（头文件不被直接编译，仅用于工程组织）
add_library(qharness_core SHARED
    qh_export.h
    schema/Message.h        schema/Message.cpp
    engine/Engine.h
    engine/EngineReActLoop.h    engine/EngineReActLoop.cpp
    provider/Provider.h
    provider/CancellationToken.h   provider/CancellationToken.cpp
    provider/ProviderOpenAI.h   provider/ProviderOpenAI.cpp
    provider/ProviderClaude.h   provider/ProviderClaude.cpp
    tool/Tool.h
    tool/ToolManager.h          tool/ToolManager.cpp
    interaction/Interaction.h
    interaction/InteractionFeishu.h    interaction/InteractionFeishu.cpp
    memory/Memory.h
    memory/MemoryFile.h         memory/MemoryFile.cpp
    context/Composer.h          context/Composer.cpp
)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(qharness_core PRIVATE QH_BUILDING_DLL)
# core 为 SHARED：DLL 与导入库统一输出到 build/out/<config>（与 exe 同目录）
set_target_properties(qharness_core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
)
if(MSVC)
    target_compile_options(qharness_core PUBLIC /utf-8 /W3)
endif()

# 测试可执行
add_executable(qharness_core_tests
    tests/TestHarness.h
    tests/TestMain.cpp
    tests/test_Role.cpp
    tests/test_ToolCall.cpp
    tests/test_ToolResult.cpp
    tests/test_ToolDefinition.cpp
    tests/test_Message.cpp
    tests/test_ToolManager.cpp
    tests/test_CancellationToken.cpp
    tests/skeleton_compile_check.cpp
)
target_link_libraries(qharness_core_tests PRIVATE qharness_core)
# SHARED 后测试 exe 必须与 qharness_core.dll 同目录（build/out/<config>）才能运行
set_target_properties(qharness_core_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
)
if(MSVC)
    target_compile_options(qharness_core_tests PRIVATE /utf-8)
endif()
add_test(NAME schema_tests COMMAND qharness_core_tests)
