cmake_minimum_required(VERSION 4.2.0)
project(Gyanis VERSION 2.0.0 DESCRIPTION "高性能网络服务框架" LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#if (MSVC)
#    add_compile_options(/W4 /WX- /utf-8)
#else ()
#    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion)
#endif ()

option(BUILD_TEST "Set to ON to compile and build tests" OFF)

if (BUILD_TEST)
    enable_testing()
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory(tests)
endif ()

find_package(yaml-cpp)

include(cmake/utils.cmake)


add_subdirectory(src)
