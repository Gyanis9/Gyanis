/**
 * @file Hook.h
 * @brief 钩子函数模块封装
 * @date 2025-03-13
 */
#ifndef HOOK_H
#define HOOK_H
#include <cstdint>
#include <unistd.h>


namespace Gyanis::core
{
    /**
     * @brief 检查是否启用钩子
     */
    bool is_hook_enable();

    /**
     * @brief 设置是否启用钩子
     */
    void set_hook_enable(bool flag);
    void hook_init();
}

extern "C" {
/// 系统调用的钩子函数声明

/// sleep相关函数
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec* req, timespec* rem);
extern nanosleep_fun nanosleep_f;

/// socket相关函数
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int s, sockaddr* addr, socklen_t* addrlen);
extern accept_fun accept_f;

/// read相关函数
typedef ssize_t (*read_fun)(int fd, void* buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec* iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void* buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void* buf, size_t len, int flags, sockaddr* src_addr,
                                socklen_t* addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr* msg, int flags);
extern recvmsg_fun recvmsg_f;


/// write相关函数
typedef ssize_t (*write_fun)(int fd, const void* buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const iovec* iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int s, const void* msg, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int s, const void* msg, size_t len, int flags, const sockaddr* to,
                              socklen_t tolen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int s, const msghdr* msg, int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;
/// 自定义带超时的 connect 函数
extern int connect_with_timeout(int fd, const sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);
}

#endif
