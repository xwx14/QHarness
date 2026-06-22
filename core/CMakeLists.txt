add_library(qharness_core STATIC
    empty.cpp
)
target_include_directories(qharness_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(qharness_core PUBLIC /utf-8 /W3)
endif()
