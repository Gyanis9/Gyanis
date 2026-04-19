# Hook 钩子接口

## 概述

`Hook` 模块用于在协程调度场景下接管部分阻塞 IO 行为，并提供 `connect_with_timeout` 这类带超时控制的能力。

- 头文件：`src/core/Hook.h`
- 实现文件：`src/core/Hook.cpp`

本次重构重点：

1. 使用 C++20 写法增强初始化与状态管理（`std::atomic_bool`、`std::numeric_limits`、更清晰的条件分支）。
2. 日志输出以中文为主，定位平台行为与超时配置更直观。
3. 同时兼容 Ubuntu 与 Windows：
   - Linux 保留完整 Hook 路径（`dlsym + IOManager/Fiber`）。
   - Windows 提供安全回退实现，不依赖 `dlsym`/`unistd`。

## 主要接口

- `[[nodiscard]] bool is_hook_enable()`：获取当前线程 Hook 开关状态。
- `void set_hook_enable(bool flag)`：设置当前线程 Hook 开关。
- `void hook_init()`：初始化 Hook。
- `int connect_with_timeout(int fd, const sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)`：带超时的连接接口。

## 跨平台设计

### Ubuntu / Linux

- 使用 `dlsym(RTLD_NEXT, ...)` 绑定原始系统调用。
- 在 Hook 打开时，通过 `IOManager` 注册读写事件并结合 `Fiber::Yield()` 做协程让出。
- `do_io` 模板统一处理 `read/recv/write/send` 一类可中断重试与超时取消逻辑。

### Windows

- 不使用 `dlsym` 劫持，采用回退模式，避免平台不支持导致的构建失败。
- `connect_with_timeout` 使用 `ioctlsocket + select + getsockopt(SO_ERROR)` 实现超时连接。
- 头文件不直接暴露 WinSock 头，避免宏污染影响其他模块编译。

## 日志约定

日志使用中文主导，典型场景包括：

- Hook 初始化路径说明（Windows 回退模式提示）
- TCP 连接超时配置更新
- Linux Hook 调度失败（无 IOManager、事件注册失败等）

## 测试

新增测试文件：`tests/core/TestHook.cpp`

覆盖点：

- Hook 开关状态可读写（`set_hook_enable` / `is_hook_enable`）。

测试接入：

- `tests/core/CMakeLists.txt` 已加入 `TestHook.cpp`。

验证结果：

- `Build_CMakeTools` 构建通过
- `RunCtest_CMakeTools` 中 `TestBase` 与 `TestCore` 全通过
