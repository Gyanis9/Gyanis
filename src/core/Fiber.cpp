#include "core/Fiber.h"
#include "base/Macro.h"
#include "base/Config.h"

#include <boost/coroutine2/fixedsize_stack.hpp>


namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");
    static auto global_fiber_stack_size = base::Config::LookUp<uint32_t>(
        "fiber.stack_size", 128 * 1024, "fiber stack size");

    static thread_local Fiber* currentFiber = nullptr;
    static thread_local Fiber* preFiber = nullptr;

    Fiber::Fiber(std::function<void()> callback, const uint32_t stackSize): m_callback(std::move(callback)),
                                                                            m_stackSize(
                                                                                stackSize == 0
                                                                                    ? global_fiber_stack_size->
                                                                                    getValue()
                                                                                    : stackSize)
    {
        m_coro = std::make_unique<CoroType::push_type>(boost::coroutines2::fixedsize_stack(m_stackSize),
                                                       [](CoroType::pull_type& yield)
                                                       {
                                                           Entry(yield);
                                                       });
    }

    Fiber::~Fiber() = default;

    void Fiber::resume()
    {
        SetThis(this);
        if (m_state == EXEC)
        {
            LOG_ERROR(g_logger) << "Fiber::resume - Fiber is already running.";
            return;
        }
        m_state = EXEC;
        if (m_coro && m_coro.get())
        {
            m_coro->operator()();
        }
    }

    void Fiber::yield() const
    {
        if (m_yield)
        {
            m_yield->operator()();
        }
    }

    Fiber::State Fiber::getState() const
    {
        return m_state;
    }

    void Fiber::setHold()
    {
        m_state = HOLD;
    }

    void Fiber::setReady()
    {
        m_state = READY;
    }

    void Fiber::reset(std::function<void()> callback)
    {
        ASSERT(m_state == TERM|| m_state == EXCEPT|| m_state == INIT);
        m_callback = std::move(callback);
        m_coro = std::make_unique<CoroType::push_type>(boost::coroutines2::fixedsize_stack(m_stackSize),
                                                       [](CoroType::pull_type& yield)
                                                       {
                                                           Entry(yield);
                                                       });
        m_state = INIT;
    }

    void Fiber::SetThis(Fiber* fiber)
    {
        preFiber = currentFiber;
        currentFiber = fiber;
    }

    std::shared_ptr<Fiber> Fiber::GetThis()
    {
        if (currentFiber)
        {
            return currentFiber->shared_from_this();
        }
        LOG_WARN(g_logger) << "Fiber::GetThis - Fiber is null"
                           << " | Status: Invalid";
        return nullptr;
    }

    void Fiber::Yield()
    {
        if (const auto fiber = GetThis())
        {
            fiber->yield();
        }
    }

    void Fiber::Entry(CoroType::pull_type& yield)
    {
        const auto fiber = GetThis();
        fiber->m_yield = &yield;
        ASSERT(fiber);
        try
        {
            fiber->m_callback();
            fiber->m_callback = nullptr;
            fiber->m_state = TERM;
        }
        catch (std::exception& e)
        {
            LOG_ERROR(g_logger)
                << "Fiber::Entry - encountered an exception. "
                << "Error details: " << e.what();
            fiber->m_state = EXCEPT;
        }catch (...)
        {
            LOG_ERROR(g_logger)
                << "Fiber::Entry - Unknown exception encountered. "
                << "An unexpected error occurred during fiber entry.";
            fiber->m_state = EXCEPT;
        }
        fiber->m_yield = nullptr;
        SetThis(preFiber);
    }
}
