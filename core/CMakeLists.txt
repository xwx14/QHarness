# 抑制 <codecvt>/wstring_convert 的 C++17 弃用警告（PathCodec.h 用其做编码转换；标准尚未给替代）
if(MSVC)
    add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()

# 所有头文件一并列入源列表，使其在 VS 工程中可见（头文件不被直接编译，仅用于工程组织）
add_library(qharness_core SHARED
    qh_export.h
    schema/Message.h        schema/Message.cpp
    schema/PostMessage.h    schema/PostMessage.cpp
    schema/CancellationToken.h   schema/CancellationToken.cpp
    schema/Settings.h       schema/Settings.cpp
    engine/Engine.h
    engine/EngineReActLoop.h    engine/EngineReActLoop.cpp
    engine/Engine2StageReAct.h engine/Engine2StageReAct.cpp
    provider/Provider.h
    provider/ProviderOpenAI.h   provider/ProviderOpenAI.cpp
    provider/ProviderClaude.h   provider/ProviderClaude.cpp
    provider/MockProvider.h     provider/MockProvider.cpp
    provider/MockTwoStageProvider.h provider/MockTwoStageProvider.cpp
    provider/ProviderHttp.h        provider/ProviderHttp.cpp
    tool/Tool.h                 tool/Tool.cpp
    tool/ToolManager.h          tool/ToolManager.cpp
    tool/MockBashTool.h         tool/MockBashTool.cpp
    tool/ReadFileTool.h         tool/ReadFileTool.cpp
    tool/PathCodec.h
    tool/ContentCodec.h
    interaction/Interaction.h
    interaction/InteractionFeishu.h    interaction/InteractionFeishu.cpp
    memory/Memory.h
    memory/MemoryFile.h         memory/MemoryFile.cpp
    context/Composer.h          context/Composer.cpp
    config/SettingsStore.h      config/SettingsStore.cpp
)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(qharness_core PRIVATE QH_BUILDING_DLL)

# OpenSSL（vcpkg 3.6.3，dynamic）：httplib SSLClient 依赖。
# 产物在 third/openssl：Release 用 lib/+bin/，Debug 用 libd/+bind/。
# 手动多 config（绕过 find_package 单 config 限制，按 $<CONFIG> 选 lib/dll）；FORCE 覆盖旧 cache。
set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/third/openssl" CACHE PATH "OpenSSL 安装根（见 third/openssl-README.md）" FORCE)
target_include_directories(qharness_core PRIVATE ${OPENSSL_ROOT_DIR}/include)
target_link_libraries(qharness_core PRIVATE
    $<IF:$<CONFIG:Debug>,${OPENSSL_ROOT_DIR}/libd/libssl.lib,${OPENSSL_ROOT_DIR}/lib/libssl.lib>
    $<IF:$<CONFIG:Debug>,${OPENSSL_ROOT_DIR}/libd/libcrypto.lib,${OPENSSL_ROOT_DIR}/lib/libcrypto.lib>)
target_compile_definitions(qharness_core PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# OpenSSL 运行时 dll 部署到 core 输出目录（Debug→bind/，Release→bin/；
# qharness_core.dll 加载需，tests/app exe 同目录运行也需）
add_custom_command(TARGET qharness_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<CONFIG:Debug>,${OPENSSL_ROOT_DIR}/bind/libcrypto-3-x64.dll,${OPENSSL_ROOT_DIR}/bin/libcrypto-3-x64.dll>
        $<IF:$<CONFIG:Debug>,${OPENSSL_ROOT_DIR}/bind/libssl-3-x64.dll,${OPENSSL_ROOT_DIR}/bin/libssl-3-x64.dll>
        $<IF:$<CONFIG:Debug>,${OPENSSL_ROOT_DIR}/bind/legacy.dll,${OPENSSL_ROOT_DIR}/bin/legacy.dll>
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
    tests/test_Settings.cpp
    tests/test_ToolManager.cpp
    tests/test_CancellationToken.cpp
    tests/test_MockProvider.cpp
    tests/test_MockTwoStageProvider.cpp
    tests/test_MockBashTool.cpp
    tests/test_ReadFileTool.cpp
    tests/test_ContentCodec.cpp
    tests/test_EngineReActLoop.cpp
    tests/test_Engine2StageReAct.cpp
    tests/MockHttpServer.h
    tests/test_ProviderHttp.cpp
    tests/test_ProviderOpenAI.cpp
    tests/test_ProviderClaude.cpp
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
