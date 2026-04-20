cmake_minimum_required(VERSION 3.20)
project(Gyanis VERSION 2.0.0 DESCRIPTION "高性能网络服务框架" LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)  # 显示详细编译信息
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(
        -fPIC
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-ggdb>
        $<$<NOT:$<CONFIG:Debug>>:-O3>
        -Wall
        -Wno-deprecated
        -Wno-unused-function
        -Wno-builtin-macro-redefined
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-rdynamic)  # 支持动态符号解析（用于更好的堆栈跟踪）
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-macro-redefined)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
endif ()

find_package(yaml-cpp)
find_package(Boost)
find_package(nlohmann_json)
find_package(OpenSSL)
find_package(ZLIB)
find_package(Protobuf)
find_package(Libevent)
find_package(hiredis)
find_package(redis++)
find_package(libmysqlclient)

include(cmake/utils.cmake)

add_subdirectory(src)


#
#set(base
#        src/base/Log.h
#        src/base/Log.cpp
#
#        src/base/Singleton.h
#        src/base/Mutex.h
#        src/base/Mutex.cpp
#
#        # 工具类
#        src/base/NonCopyable.h
#        src/base/Config.h        # 配置管理
#        src/base/Config.cpp
#        src/base/Endian.h        # 字节序转换
#        src/base/Utils.h         # 通用工具函数
#        src/base/Utils.cpp
#        src/base/Macro.h         # 常用宏定义
#
#        # 定时器模块
#        src/base/Timer.h
#        src/base/Timer.cpp
#
#        # JSON工具
#        src/base/JsonUtils.h
#        src/base/JsonUtils.cpp
#
#        # 哈希工具
#        src/base/HashUtils.h
#        src/base/HashUtils.cpp
#)
#
################################################################################
## 核心功能模块 (core)
## 协程调度、IO管理、Hook系统等核心功能
################################################################################
#set(core
#        # 协程实现
#        src/core/Fiber.h         # 协程类定义
#        src/core/Fiber.cpp
#
#        # 调度系统
#        src/core/Scheduler.h     # 协程调度器
#        src/core/Scheduler.cpp
#
#        # IO管理
#        src/core/IOManager.h     # IO事件管理器
#        src/core/IOManager.cpp
#        src/core/FdManager.h     # 文件描述符管理
#        src/core/FdManager.cpp
#
#        # 系统Hook
#        src/core/Hook.h          # 系统调用Hook
#        src/core/Hook.cpp
#
#        # 守护进程
#        src/core/Daemon.h
#        src/core/Daemon.cpp
#
#        # 环境管理
#        src/core/Env.h           # 环境变量管理
#        src/core/Env.cpp
#
#        # 工作线程管理
#        src/core/Worker.h        # 工作线程实现
#        src/core/Worker.cpp
#)
#
################################################################################
## 网络模块 (net)
## 网络通信、协议解析、HTTP服务器等网络相关功能
################################################################################
## 使用Ragel生成HTTP协议解析器
##add_custom_command(
##        OUTPUT
##        ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.cpp
##        ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.cpp
##        ${PROJECT_SOURCE_DIR}/src/net/http/Uri.cpp
##        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/src -o ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.cpp ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.rl
##        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/src -o ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.cpp ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.rl
##        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/src -o ${PROJECT_SOURCE_DIR}/src/net/http/Uri.cpp ${PROJECT_SOURCE_DIR}/src/net/http/Uri.rl
##        DEPENDS
##        ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.rl
##        ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.rl
##        ${PROJECT_SOURCE_DIR}/src/net/http/Uri.rl
##)
#
#set(net
#        # 网络基础
#        src/net/Socket.h          # Socket封装
#        src/net/Socket.cpp
#        src/net/Address.h         # 网络地址处理
#        src/net/Address.cpp
#
#        # 流处理
#        src/net/stream/ByteArray.h  # 字节数组处理
#        src/net/stream/ByteArray.cpp
#
#        # HTTP协议实现
#        src/net/http/Http.h       # HTTP基础定义
#        src/net/http/Http.cpp
#        src/net/http/httpclient_parser.h  # HTTP客户端解析器
#        src/net/http/httpclient_parser.cpp
#        src/net/http/httpclient_parser.rl
#        src/net/http/http11_common.h      # HTTP 1.1通用定义
#        src/net/http/http11_parser.h      # HTTP 1.1解析器
#        src/net/http/http11_parser.cpp
#        src/net/http/http11_parser.rl
#        src/net/http/HttpParser.h         # HTTP解析器封装
#        src/net/http/HttpParser.cpp
#
#        # 服务器实现
#        src/net/web/TcpServer.h           # TCP服务器
#        src/net/web/TcpServer.cpp
#
#        # 流处理
#        src/net/stream/Stream.h
#        src/net/stream/Stream.cpp
#        src/net/stream/SocketStream.h
#        src/net/stream/SocketStream.cpp
#        src/net/stream/ZlibStream.h
#        src/net/stream/ZlibStream.cpp
#        src/net/http/HttpSession.h
#        src/net/http/HttpSession.cpp
#
#        # 高级协议支持
#        src/net/protocol/Protocol.h        # 协议基类
#        src/net/protocol/Protocol.cpp
#        src/net/rock/RockProtocol.h        # 自定义Rock协议
#        src/net/rock/RockProtocol.cpp
#        src/net/http/servlets/Servlet.h
#        src/net/http/servlets/Servlet.cpp
#        src/net/http/servlets/ConfigServlet.h
#        src/net/http/servlets/ConfigServlet.cpp
#        src/net/http/servlets/StatusServlet.h
#        src/net/http/servlets/StatusServlet.cpp
#        src/net/http/Uri.h
#        src/net/http/Uri.cpp
#        src/net/http/Uri.rl
#        src/net/http/HttpServer.h           # HTTP服务器
#        src/net/http/HttpServer.cpp
#        src/net/http/HttpConnection.h
#        src/net/http/HttpConnection.cpp
#        src/net/http/WebSocketSession.h
#        src/net/http/WebSocketSession.cpp
#        src/net/http/WebSocketServlet.h
#        src/net/http/WebSocketServlet.cpp
#        src/net/http/WebSocketServer.h
#        src/net/http/WebSocketServer.cpp
#        src/net/http/WebSocketConnection.h
#        src/net/http/WebSocketConnection.cpp
#
#        # 其他组件
#        src/net/Application.h # 网络应用基类
#        src/net/Application.cpp
#        src/net/Module.h
#        src/net/Module.cpp
#        src/net/Library.h
#        src/net/Library.cpp
#)
#
################################################################################
## 数据库模块 (db)
## 数据库连接与操作
################################################################################
#set(db
#        src/db/Db.h          # 数据库接口定义
#        src/db/Mysql.cpp         # MySQL实现
#        src/db/Mysql.h
#        src/db/FoxThread.cpp
#        src/db/FoxThread.h
#        src/db/Redis.cpp
#        src/db/Redis.h
#)

#add_executable(Gyanis src/main.cpp ${base} ${core} ${net} ${db})
#
#target_link_libraries(Gyanis PUBLIC
#        dl
#        yaml-cpp::yaml-cpp
#        nlohmann_json::nlohmann_json
#        boost::boost
#        openssl::openssl
#        ZLIB::ZLIB
#        protobuf::protobuf
#        libevent::libevent
#        hiredis::hiredis
#        redis++::redis++_static
#        libmysqlclient::libmysqlclient
#)
#
#target_include_directories(Gyanis PUBLIC
#        src
#)
#
## 强制重定义 __FILE__ 宏，方便日志打印/调试
#force_redefine_file_macro_for_sources(Gyanis)
