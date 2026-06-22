set(CMAKE_AUTOMOC ON)
add_executable(qharness_app WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
)
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
