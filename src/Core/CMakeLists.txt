add_library(Core
        Async.hpp
        Awaitables.hpp
        Cancellation.hpp
        ExecutionContext.hpp
        MemoryPool.hpp
        PlatformCompat.hpp
        Poller.hpp
        Scheduler.hpp
        Task.hpp
        ThreadPool.hpp
        Timer.hpp
        Poller.cpp
        ThreadPool.cpp
        Timer.cpp
        Scheduler.cpp
        MemoryPool.cpp
)

target_link_libraries(Core PUBLIC
        Base
)