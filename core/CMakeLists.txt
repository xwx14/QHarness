# 所有头文件一并列入源列表，使其在 VS 工程中可见（头文件不被直接编译，仅用于工程组织）
add_library(qharness_core SHARED
    qh_export.h
    schema/Message.h        schema/Message.cpp
    schema/PostMessage.h    schema/PostMessage.cpp
    schema/CancellationToken.h   schema/CancellationToken.cpp
    engine/Engine.h
    engine/EngineReActLoop.h    engine/EngineReActLoop.cpp
    engine/Engine2StageReAct.h engine/Engine2StageReAct.cpp
    provider/Provider.h
    provider/ProviderOpenAI.h   provider/ProviderOpenAI.cpp
    provider/ProviderClaude.h   provider/ProviderClaude.cpp
    provider/MockProvider.h     provider/MockProvider.cpp
    provider/MockTwoStageProvider.h provider/MockTwoStageProvider.cpp
    provider/ProviderHttp.h        provider/ProviderHttp.cpp
    tool/Tool.h
    tool/ToolManager.h          tool/ToolManager.cpp
    tool/MockBashTool.h         tool/MockBashTool.cpp
    interaction/Interaction.h
    interaction/InteractionFeishu.h    interaction/InteractionFeishu.cpp
    memory/Memory.h
    memory/MemoryFile.h         memory/MemoryFile.cpp
    context/Composer.h          context/Composer.cpp
)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(qharness_core PRIVATE QH_BUILDING_DLL)

# OpenSSL（vcpkg 3.6.3，dynamic）：httplib SSLClient 依赖
set(OPENSSL_ROOT_DIR "E:/vcpkg/installed/x64-windows" CACHE PATH "vcpkg OpenSSL 安装根")
find_package(OpenSSL REQUIRED)
target_link_libraries(qharness_core PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(qharness_core PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# OpenSSL 运行时 dll 部署到 core 输出目录（qharness_core.dll 加载需；
# qharness_core_tests.exe / qharness_app.exe 同目录运行也需）
add_custom_command(TARGET qharness_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSSL_ROOT_DIR}/bin/libcrypto-3-x64.dll"
        "${OPENSSL_ROOT_DIR}/bin/libssl-3-x64.dll"
        $<TARGET_FILE_DIR:qharness_core>)

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
    tests/test_PostMessage.cpp
    tests/test_ToolManager.cpp
    tests/test_CancellationToken.cpp
    tests/test_MockProvider.cpp
    tests/test_MockTwoStageProvider.cpp
    tests/test_MockBashTool.cpp
    tests/test_EngineReActLoop.cpp
    tests/test_Engine2StageReAct.cpp
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
