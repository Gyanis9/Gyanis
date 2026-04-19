cmake_minimum_required(VERSION 4.2.0)
project(Gyanis VERSION 2.0.0 DESCRIPTION "高性能网络服务框架" LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_TEST "Set to ON to compile and build tests" OFF)  # 测试构建开关

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

#set(MYSQL_INCLUDE_DIR "/usr/include/mysql/")
#set(MYSQL_LIB_DIR "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")

# 包含自定义CMake工具函数
include(cmake/utils.cmake)


add_subdirectory(src)
add_subdirectory(tests/base)


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
#        src/base/Config.h
#        src/base/Config.cpp
#        src/base/Endian.h
#        src/base/Utils.h
#        src/base/Utils.cpp
#        src/base/Macro.h
#
#        src/base/Timer.h
#        src/base/Timer.cpp
#
#        src/base/JsonUtils.h
#        src/base/JsonUtils.cpp
#
#        src/base/HashUtils.h
#        src/base/HashUtils.cpp
#)

#set(core
#        src/core/Fiber.h
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

#add_custom_command(
#        OUTPUT
#        ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.cpp
#        ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.cpp
#        ${PROJECT_SOURCE_DIR}/src/net/http/Uri.cpp
#        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/include -o ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.cpp ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.rl
#        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/include -o ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.cpp ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.rl
#        COMMAND ragel -G2 -C -I${PROJECT_SOURCE_DIR}/include -o ${PROJECT_SOURCE_DIR}/src/net/http/Uri.cpp ${PROJECT_SOURCE_DIR}/src/net/http/Uri.rl
#        DEPENDS
#        ${PROJECT_SOURCE_DIR}/src/net/http/httpclient_parser.rl
#        ${PROJECT_SOURCE_DIR}/src/net/http/http11_parser.rl
#        ${PROJECT_SOURCE_DIR}/src/net/http/Uri.rl
#)

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
#        dl                            # 支持动态库加载（如 dlsym, dlopen 等）
#        yaml-cpp::yaml-cpp            # YAML 配置解析库
#        nlohmann_json::nlohmann_json  # JSON 解析库（现代C++风格）
#        boost::boost
#        OpenSSL::SSL                  # OpenSSL 库，用于SSL加密与通信
#        ZLIB::ZLIB                    # Zlib 库，支持数据压缩
#        protobuf::protobuf
#        libevent::libevent
#        hiredis::hiredis              # Redis C客户端库
#        redis++::redis++_static       # Redis++ C++客户端库（静态链接）
#        ${MYSQL_LIB_DIR}              # MySQL 客户端库路径变量
#)
#
#target_include_directories(Gyanis PUBLIC
#        include                       # 项目主头文件目录
#        ${MYSQL_INCLUDE_DIR}          # MySQL 头文件目录
#)

#force_redefine_file_macro_for_sources(Gyanis)

if (BUILD_TEST)
	enable_testing()
	add_subdirectory(tests)     # 测试代码目录
endif ()