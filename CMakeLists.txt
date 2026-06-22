cmake_minimum_required(VERSION 3.15)
project(QHarnesscc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# 第三方头文件（httplib + nlohmann）全局可见
include_directories(${CMAKE_SOURCE_DIR}/third)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

enable_testing()

add_subdirectory(core)
add_subdirectory(app)
