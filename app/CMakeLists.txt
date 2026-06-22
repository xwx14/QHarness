set(CMAKE_AUTOMOC ON)
add_executable(qharness_app WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
)
# 统一输出到 build/out：可执行程序与所有 DLL（含 windeployqt 部署的 Qt 运行时）集中于此
# 多配置生成器（VS）会追加 Release/ 子目录，故用每配置属性覆盖，强制直接落入 out/
foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CFG} CFG_UPPER)
    set_target_properties(qharness_app PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} "${CMAKE_BINARY_DIR}/out")
endforeach()
set_target_properties(qharness_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
target_link_libraries(qharness_app PRIVATE qharness_core Qt5::Core Qt5::Widgets)
if(MSVC)
    target_compile_options(qharness_app PRIVATE /utf-8)
endif()

# 自动部署 Qt 运行时（DLL + 平台插件）到 exe 同级目录，使应用可开箱即运
# 将正确的 Qt bin 置于 PATH 最前，规避环境中其他 Qt（如 Anaconda）干扰
get_filename_component(QT_BIN_DIR "${Qt5_DIR}/../../../bin" ABSOLUTE)
if(EXISTS "${QT_BIN_DIR}/windeployqt.exe")
    add_custom_command(TARGET qharness_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${QT_BIN_DIR};$ENV{PATH}"
                "${QT_BIN_DIR}/windeployqt.exe"
                --release --no-translations --no-quick-import
                $<TARGET_FILE:qharness_app>
        COMMENT "Deploying Qt runtime next to qharness_app.exe")
endif()
