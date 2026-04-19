# IOManager 接口

## 概述

`IOManager` 是 `Scheduler` 与 `TimerManager` 的组合实现，负责：

- 管理 fd 事件（读/写）
- 驱动定时器回调调度
- 在空闲循环中统一推进 IO 事件与超时任务

头文件：`src/core/IOManager.h`
实现文件：`src/core/IOManager.cpp`

## 本次重构目标

1. 使用 C++20 风格增强接口安全性（`[[nodiscard]]`、更严格参数校验、现代并发原语）。
2. 日志以中文为主，便于线上定位。
3. 同时支持 Ubuntu 与 Windows 编译运行。
4. 保持原有 `Scheduler` 调度行为与 `TimerManager` 语义。

## 主要接口

- `explicit IOManager(size_t threadCount = 1, const std::string& name = "")`
- `int addEvent(int fd, Event event, std::function<void()> callback = nullptr)`
- `[[nodiscard]] bool delEvent(int fd, Event event)`
- `[[nodiscard]] bool cancelEvent(int fd, Event event)`
- `[[nodiscard]] bool cancelAll(int fd)`
- `static IOManager* GetThis()`

事件类型：

- `READ`
- `WRITE`

## 跨平台行为说明

### Ubuntu / Linux

- 使用 `epoll` + `eventfd` 实现高效 IO 多路复用。
- `tickle()` 通过 `eventfd_write` 唤醒空闲线程。
- `idle()` 中统一处理：
  - epoll 事件分发
  - 定时器过期回调调度
  - 协程让出执行权

### Windows

- 自动进入回退模式（无 `epoll/eventfd`）。
- 仍支持：
  - 调度器线程调度
  - 定时器驱动与回调调度
- 当前不支持 fd 事件注册：
  - `addEvent` 返回 `-1`
  - `delEvent/cancelEvent/cancelAll` 返回 `false`
- 使用 `std::condition_variable` 实现 `tickle()` 唤醒空闲线程。

## 稳定性与防御性改进

- 对负数 `fd` 与非法事件做前置校验，避免越界访问。
- 统一错误路径中文日志，包含关键上下文（fd、事件、errno）。
- `m_epFd/m_wakeFd` 初始值改为 `-1`，析构关闭时更安全。
- `schedule` 批量调度路径已适配 C++20 concepts 约束（按值传递任务）。

## 测试

新增测试文件：`tests/core/TestIOManager.cpp`

覆盖点：

- 无效 `fd` 下 `addEvent/delEvent/cancelEvent/cancelAll` 行为符合预期。

测试入口：

- `tests/core/CMakeLists.txt`
- 通过 `RunCtest_CMakeTools` 验证 `TestBase` 与 `TestCore` 全通过。
