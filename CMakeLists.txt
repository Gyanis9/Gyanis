cmake_minimum_required(VERSION 4.2.0)
project(Gyanis VERSION 2.0.0 DESCRIPTION "高性能网络服务框架" LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_TEST "Set to ON to compile and build tests" OFF)

if (BUILD_TEST)
    find_package(Catch2 CONFIG REQUIRED)
endif ()

find_package(yaml-cpp)
find_package(Boost)
find_package(nlohmann_json)
find_package(OpenSSL)
find_package(ZLIB)
find_package(protobuf)
find_package(Libevent)
find_package(hiredis)
find_package(redis++)
find_package(Catch2)

include(cmake/utils.cmake)


add_subdirectory(src)
