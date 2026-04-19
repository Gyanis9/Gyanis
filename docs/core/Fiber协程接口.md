# Fiber 协程接口

## 概述

`Fiber` 模块已从 Boost.Coroutine2 迁移为 C++20 原生协程实现，核心目标：

- 去除对 Boost 协程库的依赖
- 保持协程状态机语义（`INIT/HOLD/EXEC/TERM/READY/EXCEPT`）
- 提供 C++20 协程挂起接口 `co_await Fiber::Suspend()`
- 日志输出中文化，便于排障
- 同时兼容 Ubuntu 与 Windows

头文件位置：`src/core/Fiber.h`
实现位置：`src/core/Fiber.cpp`

## 状态说明

- `INIT`：初始态，未执行
- `EXEC`：执行中
- `HOLD`：挂起态
- `READY`：就绪态（等待再次调度）
- `TERM`：正常结束
- `EXCEPT`：异常结束

## 主要接口

### 构造与重置

- `explicit Fiber(Callback callback, uint32_t stackSize = 0)`
- `explicit Fiber(CoroutineCallback callback, uint32_t stackSize = 0)`
- `void reset(Callback callback)`
- `void reset(CoroutineCallback callback)`

说明：

- `Callback` 为兼容旧代码的 `std::function<void()>`。
- `CoroutineCallback` 为推荐方式，类型是 `std::function<Fiber::Task()>`。

### 调度与状态

- `void resume()`：恢复执行（或首次启动）
- `State getState() const`：读取状态
- `void setHold()` / `void setReady()`：手动设置状态

### 当前协程访问

- `static void SetThis(Fiber* fiber)`
- `static std::shared_ptr<Fiber> GetThis()`

### 挂起接口

- `static SuspendAwaitable Suspend()`：C++20 协程挂起点，使用方式：`co_await Fiber::Suspend();`
- `static void Yield()` / `void yield()`：兼容旧接口（仅状态兼容，不是 C++20 真挂起）

## 推荐写法

```cpp
auto fiber = std::make_shared<Gyanis::core::Fiber>(
    Gyanis::core::Fiber::CoroutineCallback([]() -> Gyanis::core::Fiber::Task {
        // 执行前半段
        co_await Gyanis::core::Fiber::Suspend();
        // 恢复后执行后半段
        co_return;
    })
);

fiber->resume(); // 运行到 Suspend 挂起
fiber->resume(); // 继续执行到结束
```

## 兼容性说明（Ubuntu / Windows）

本模块仅依赖：

- C++20 标准库（`<coroutine>`）
- 项目内 Base 组件（日志/断言）

不再依赖 Boost 协程库，因此在 Ubuntu 与 Windows 下均可使用同一套协程实现。

## 测试

新增测试文件：`tests/core/TestFiber.cpp`

覆盖点：

- C++20 协程挂起与恢复行为
- `reset` 后回调替换行为

测试构建入口：

- `tests/core/CMakeLists.txt`
- `tests/CMakeLists.txt` 已新增 `add_subdirectory(core)`
