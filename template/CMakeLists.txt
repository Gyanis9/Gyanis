cmake_minimum_required(VERSION 3.20)
project(project_name)

# 使用vcpkg进行第三方依赖管理
set(CMAKE_PREFIX_PATH "/home/study/vcpkg/installed/x64-linux/share" CACHE PATH "Vcpkg package installation directory")
set(CMAKE_TOOLCHAIN_FILE "/home/study/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain file")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Gyanis)


include(Gyanis/cmake/utils.cmake)

# 输出目录配置
#-------------------------------------------------------------------------------
set(CMAKE_VERBOSE_MAKEFILE ON)  # 显示详细编译信息
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)   # 静态库输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)   # 动态库输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)   # 可执行文件输出目录

###############################################################################
# 全局编译配置
###############################################################################
# C++标准配置
set(CMAKE_CXX_STANDARD 17)       # 强制使用C++17标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)     # 禁用编译器扩展

# C标准配置
set(CMAKE_C_STANDARD 11)         # 强制使用C11标准
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 编译选项配置
#-------------------------------------------------------------------------------
add_compile_options(
        -fPIC                        # 生成位置无关代码（适用于动态库）
        $<$<CONFIG:Debug>:-O0>       # 调试模式不优化
        $<$<CONFIG:Debug>:-ggdb>     # 生成GDB调试信息
        $<$<NOT:$<CONFIG:Debug>>:-O3># 发布模式优化级别O3
        -Wall                        # 启用所有警告
        -Wno-deprecated              # 忽略废弃声明警告
        -Wno-unused-function         # 忽略未使用函数警告
        -Wno-builtin-macro-redefined
)

# 仅GNU/Clang编译器生效的选项
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-rdynamic)  # 支持动态符号解析（用于更好的堆栈跟踪）
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-macro-redefined)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC 默认忽略此类警告，无需特殊处理
endif ()

###############################################################################
# 依赖项配置
###############################################################################
# 使用find_package查找预安装的库
find_package(yaml-cpp CONFIG REQUIRED)       # YAML配置文件解析库
find_package(Boost CONFIG REQUIRED COMPONENTS coroutine2 system filesystem)  # Boost协程组件
find_package(nlohmann_json CONFIG REQUIRED)  # JSON解析库
find_package(OpenSSL CONFIG REQUIRED)        # SSL/TLS支持
find_package(ZLIB REQUIRED)                  # 压缩支持
find_package(Protobuf CONFIG REQUIRED)       # Protocol Buffers序列化库
find_package(Libevent CONFIG REQUIRED)       # Libevent事件循环库
find_package(hiredis CONFIG REQUIRED)        # hiredis Redis客户端库
find_package(redis++ CONFIG REQUIRED)        # redis++ C++ Redis客户端库

# MySQL配置（手动指定路径）
set(MYSQL_INCLUDE_DIR "/usr/include/mysql/")
set(MYSQL_LIB_DIR "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")

# 包含自定义CMake工具函数
include(Gyanis/cmake/utils.cmake)

###############################################################################
# 项目功能模块
###############################################################################
set(LIB_SRC
        template/my_module.cpp
        template/my_module.h
)

add_library(project_name SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(project_name)
target_include_directories(project_name PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Gyanis/include)