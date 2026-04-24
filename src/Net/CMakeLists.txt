add_library(Net
        Address.h
        Address.cpp
        Socket.cpp
        Socket.h
        Buffer.cpp
        Buffer.h
        TcpConnection.cpp
        TcpConnection.h
        TcpAcceptor.cpp
        TcpAcceptor.h
        TcpServer.cpp
        TcpServer.h
        HttpMessage.cpp
        HttpMessage.h
        HttpParser.cpp
        HttpParser.h
        HttpSerializer.h
        Router.cpp
        Router.h
        Tls.cpp
        Tls.h
        HttpServer.cpp
        HttpServer.h
        EventLoopGroup.cpp
        EventLoopGroup.h
)

target_link_libraries(Net PUBLIC
        Base
        Core
        openssl::openssl
)

target_include_directories(Net PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)