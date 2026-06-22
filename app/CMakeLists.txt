add_executable(qharness_app WIN32
    main.cpp
)
target_link_libraries(qharness_app PRIVATE qharness_core Qt5::Core Qt5::Widgets)
if(MSVC)
    target_compile_options(qharness_app PRIVATE /utf-8)
endif()
