#include "EventLoopGroup.h"

namespace Net
{
    EventLoop::EventLoop(size_t poolThreads) : m_threadPool(std::make_unique<Core::PriorityThreadPool>(poolThreads)),
                                               m_scheduler(std::make_unique<Core::IoScheduler>(*m_threadPool))
    {
    }

    void EventLoop::start() const
    {
        m_scheduler->start();
    }

    void EventLoop::stop() const
    {
        m_scheduler->stop();
    }

    Core::ExecutionContext EventLoop::getContext() const
    {
        return Core::ExecutionContext(m_scheduler.get(), m_threadPool.get());
    }

    EventLoopGroup::EventLoopGroup(const size_t numLoops, size_t poolThreadsPerLoop) : m_loops(numLoops)
    {
        for (auto &loop: m_loops)
        {
            loop = std::make_unique<EventLoop>(poolThreadsPerLoop);
        }
    }

    void EventLoopGroup::start() const
    {
        for (auto &loop: m_loops)
        {
            loop->start();
        }
    }

    void EventLoopGroup::stop() const
    {
        for (auto &loop: m_loops)
        {
            loop->stop();
        }
    }

    Core::ExecutionContext EventLoopGroup::getContext(const size_t index) const
    {
        return m_loops[index]->getContext();
    }

    size_t EventLoopGroup::size() const noexcept
    {
        return m_loops.size();
    }
}
