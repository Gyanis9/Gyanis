set(sources
        Fiber.h
        Fiber.cpp
        Scheduler.h
        Scheduler.cpp
        IOManager.h
        IOManager.cpp
        FdManager.h
        FdManager.cpp
        Hook.h
        Hook.cpp
        Daemon.h
        Daemon.cpp
)

add_library(Core SHARED ${sources})
if (WIN32)
    set_target_properties(Core PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

target_include_directories(Core PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(Core PUBLIC
        Base
)