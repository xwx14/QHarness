# 所有头文件一并列入源列表，使其在 VS 工程中可见（头文件不被直接编译，仅用于工程组织）
add_library(qharness_core STATIC
    schema/Message.h        schema/Message.cpp
    engine/Engine.h
    engine/EngineReActLoop.h
    provider/Provider.h
    provider/ProviderOpenAI.h
    provider/ProviderClaude.h
    tool/Tool.h
    robot/Robot.h
    robot/RobotFeishu.h
    memory/Memory.h
    memory/MemoryFile.h
)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
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
    tests/skeleton_compile_check.cpp
)
target_link_libraries(qharness_core_tests PRIVATE qharness_core)
if(MSVC)
    target_compile_options(qharness_core_tests PRIVATE /utf-8)
endif()
add_test(NAME schema_tests COMMAND qharness_core_tests)
