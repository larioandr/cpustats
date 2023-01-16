cmake_minimum_required(VERSION 3.18)
project(CpuStats VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

include_directories(include)
include_directories($ENV{HOME}/.local/include)
link_directories($ENV{HOME}/.local/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{HOME}/.local/lib/cmake")


add_subdirectory(src)

find_package(GTest)
if(GTEST_FOUND)
    add_subdirectory(tests)
endif()

