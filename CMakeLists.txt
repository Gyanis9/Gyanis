cmake_minimum_required(VERSION 3.20)
project(Gyanis VERSION 1.0.0 DESCRIPTION "高性能网络服务框架" LANGUAGES C CXX)

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
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C标准配置
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


# 编译选项配置
#-------------------------------------------------------------------------------
option(BUILD_TEST "Set to ON to compile and build tests" OFF)  # 测试构建开关

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
include(cmake/utils.cmake)


###############################################################################
# 基础功能模块 (base)
# 包含日志系统、工具类、配置管理等基础功能
###############################################################################
set(base
        # 日志系统
        include/base/Log.h
        src/base/Log.cpp

        # 线程安全相关
        include/base/Singleton.h    # 单例模板
        include/base/Mutex.h        # 互斥锁实现
        src/base/Mutex.cpp

        # 工具类
        include/base/NonCopyable.h   # 禁止拷贝基类
        include/base/Config.h        # 配置管理
        src/base/Config.cpp
        include/base/Endian.h        # 字节序转换
        include/base/Utils.h         # 通用工具函数
        src/base/Utils.cpp
        include/base/Macro.h         # 常用宏定义

        # 定时器模块
        include/base/Timer.h
        src/base/Timer.cpp

        # JSON工具
        include/base/JsonUtils.h
        src/base/JsonUtils.cpp

        # 哈希工具
        include/base/HashUtils.h
        src/base/HashUtils.cpp
)

###############################################################################
# 核心功能模块 (core)
# 协程调度、IO管理、Hook系统等核心功能
###############################################################################
set(core
        # 协程实现
        include/core/Fiber.h         # 协程类定义
        src/core/Fiber.cpp

        # 调度系统
        include/core/Scheduler.h     # 协程调度器
        src/core/Scheduler.cpp

        # IO管理
        include/core/IOManager.h     # IO事件管理器
        src/core/IOManager.cpp
        include/core/FdManager.h     # 文件描述符管理
        src/core/FdManager.cpp

        # 系统Hook
        include/core/Hook.h          # 系统调用Hook
        src/core/Hook.cpp

        # 守护进程
        include/core/Daemon.h
        src/core/Daemon.cpp

        # 环境管理
        include/core/Env.h           # 环境变量管理
        src/core/Env.cpp

        # 工作线程管理
        include/core/Worker.h        # 工作线程实现
        src/core/Worker.cpp
)

###############################################################################
# 网络模块 (net)
# 网络通信、协议解析、HTTP服务器等网络相关功能
###############################################################################
# 使用Ragel生成HTTP协议解析器
add_custom_command(
        OUTPUT
        ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.cpp
        ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.cpp
        ${PROJECT_SOURCE_DIR}/src/net/http/Uri.cpp
        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/include -o ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.cpp ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.rl
        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/include -o ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.cpp ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.rl
        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/include -o ${PROJECT_SOURCE_DIR}/src/net/http/Uri.cpp ${PROJECT_SOURCE_DIR}/src/net/http/Uri.rl
        DEPENDS
        ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.rl
        ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.rl
        ${PROJECT_SOURCE_DIR}/src/net/http/Uri.rl
)

set(net
        # 网络基础
        include/net/Socket.h          # Socket封装
        src/net/Socket.cpp
        include/net/Address.h         # 网络地址处理
        src/net/Address.cpp

        # 流处理
        include/net/stream/ByteArray.h  # 字节数组处理
        src/net/stream/ByteArray.cpp

        # HTTP协议实现
        include/net/http/Http.h       # HTTP基础定义
        src/net/http/Http.cpp
        include/net/http/httpclient_parser.h  # HTTP客户端解析器
        src/net/http/httpclient_parser.cpp
        src/net/http/httpclient_parser.rl
        include/net/http/http11_common.h      # HTTP 1.1通用定义
        include/net/http/http11_parser.h      # HTTP 1.1解析器
        src/net/http/http11_parser.cpp
        src/net/http/http11_parser.rl
        include/net/http/HttpParser.h         # HTTP解析器封装
        src/net/http/HttpParser.cpp

        # 服务器实现
        include/net/web/TcpServer.h           # TCP服务器
        src/net/web/TcpServer.cpp

        # 流处理
        include/net/stream/Stream.h
        src/net/stream/Stream.cpp
        include/net/stream/SocketStream.h
        src/net/stream/SocketStream.cpp
        include/net/stream/ZlibStream.h
        src/net/stream/ZlibStream.cpp
        include/net/http/HttpSession.h
        src/net/http/HttpSession.cpp

        # 高级协议支持
        include/net/protocol/Protocol.h        # 协议基类
        src/net/protocol/Protocol.cpp
        include/net/rock/RockProtocol.h        # 自定义Rock协议
        src/net/rock/RockProtocol.cpp
        include/net/http/servlets/Servlet.h
        src/net/http/servlets/Servlet.cpp
        include/net/http/servlets/ConfigServlet.h
        src/net/http/servlets/ConfigServlet.cpp
        include/net/http/servlets/StatusServlet.h
        src/net/http/servlets/StatusServlet.cpp
        include/net/http/Uri.h
        src/net/http/Uri.cpp
        src/net/http/Uri.rl
        include/net/http/HttpServer.h           # HTTP服务器
        src/net/http/HttpServer.cpp
        include/net/http/HttpConnection.h
        src/net/http/HttpConnection.cpp
        include/net/http/WebSocketSession.h
        src/net/http/WebSocketSession.cpp
        include/net/http/WebSocketServlet.h
        src/net/http/WebSocketServlet.cpp
        include/net/http/WebSocketServer.h
        src/net/http/WebSocketServer.cpp
        include/net/http/WebSocketConnection.h
        src/net/http/WebSocketConnection.cpp

        # 其他组件
        include/net/Application.h # 网络应用基类
        src/net/Application.cpp
        include/net/Module.h
        src/net/Module.cpp
        include/net/Library.h
        src/net/Library.cpp
)

###############################################################################
# 数据库模块 (db)
# 数据库连接与操作
###############################################################################
set(db
        include/db/Db.h          # 数据库接口定义
        src/db/Mysql.cpp         # MySQL实现
        include/db/Mysql.h
        src/db/FoxThread.cpp
        include/db/FoxThread.h
        src/db/Redis.cpp
        include/db/Redis.h
)

###############################################################################
# 主程序配置
###############################################################################
add_executable(Gyanis src/main.cpp ${base} ${core} ${net} ${db})

target_link_libraries(Gyanis PUBLIC
        dl                            # 支持动态库加载（如 dlsym, dlopen 等）
        yaml-cpp::yaml-cpp            # YAML 配置解析库
        nlohmann_json::nlohmann_json  # JSON 解析库（现代C++风格）
        Boost::coroutine2             # Boost 协程库
        Boost::system                 # Boost 系统库，处理错误码等
        OpenSSL::SSL                  # OpenSSL 库，用于SSL加密与通信
        ZLIB::ZLIB                    # Zlib 库，支持数据压缩
        protobuf::libprotoc           # Protocol Buffers 编码/解码（protoc工具）
        protobuf::libprotobuf         # Protocol Buffers 主库
        protobuf::libprotobuf-lite    # Protocol Buffers 轻量级版本
        libevent::core                # libevent 核心事件驱动支持
        libevent::extra               # libevent 扩展库
        libevent::pthreads            # libevent 多线程支持
        hiredis::hiredis              # Redis C客户端库
        redis++::redis++_static       # Redis++ C++客户端库（静态链接）
        ${MYSQL_LIB_DIR}              # MySQL 客户端库路径变量
)

target_include_directories(Gyanis PUBLIC
        include                       # 项目主头文件目录
        ${MYSQL_INCLUDE_DIR}          # MySQL 头文件目录
)

# 强制重定义 __FILE__ 宏，方便日志打印/调试
force_redefine_file_macro_for_sources(Gyanis)


###############################################################################
# 子目录
###############################################################################
if (BUILD_TEST)
    add_subdirectory(tests)     # 测试代码目录
    add_subdirectory(samples)   # 示例代码目录
endif ()