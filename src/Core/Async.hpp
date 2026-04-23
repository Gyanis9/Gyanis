/**
 * @file Async.hpp
 * @brief 协程任务异步启动辅助函数。
 */

#ifndef ASYNC_HPP
#define ASYNC_HPP

#include "Awaitables.hpp"
#include "Task.hpp"

#include <functional>

namespace Core
{
    /**
    * @brief 启动协程任务并自动附加优先级与执行上下文。
    * @tparam F 协程可调用对象类型。
    * @tparam Args 可调用对象参数类型。
    * @param ctx 执行上下文。
    * @param priority 调度优先级。
    * @param f 协程函数或可调用对象。
    * @param args 传递给可调用对象的参数包。
    * @return auto 无显式返回值，任务通过调度器异步执行。
     */
    template<typename F, typename... Args>
    auto async_spawn(ExecutionContext &ctx, TaskPriority priority, F &&f, Args &&... args)
    {
        auto task = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        // 设置执行上下文和优先级
        task = std::move(task).with_execution_context(ctx).with_priority(priority);
        // 提交到调度器恢复执行
        ctx.scheduler()->schedule([task = std::move(task)]() mutable
        {
            task.get();
        }, priority);
    }
}

#endif //ASYNC_HPP
