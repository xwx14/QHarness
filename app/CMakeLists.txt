set(CMAKE_AUTOMOC ON)
add_executable(qharness_app WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
)
# 统一输出到 build/out：可执行程序与所有 DLL（含 windeployqt 部署的 Qt 运行时）集中于此
# 多配置生成器（VS）自动追加配置子目录：build/out/Release、build/out/Debug
set_target_properties(qharness_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
target_link_libraries(qharness_app PRIVATE qharness_core Qt5::Core Qt5::Widgets)
if(MSVC)
    target_compile_options(qharness_app PRIVATE /utf-8)
endif()

# 自动部署 Qt 运行时（DLL + 平台插件）到 exe 同级目录，使应用可开箱即运
# 将正确的 Qt bin 置于 PATH 最前，规避环境中其他 Qt（如 Anaconda）干扰
# 按构建配置强制部署：Debug 用 debug 版 Qt DLL（Qt5Cored.dll 等），其余用 release 版
get_filename_component(QT_BIN_DIR "${Qt5_DIR}/../../../bin" ABSOLUTE)
if(EXISTS "${QT_BIN_DIR}/windeployqt.exe")
    add_custom_command(TARGET qharness_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${QT_BIN_DIR};$ENV{PATH}"
                "${QT_BIN_DIR}/windeployqt.exe"
                $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
                --no-translations --no-quick-import
                $<TARGET_FILE:qharness_app>
        COMMENT "Deploying Qt runtime next to qharness_app.exe")
endif()
