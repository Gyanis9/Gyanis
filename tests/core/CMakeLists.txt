set(TEST_CORE_SOURCES
        TestFiber.cpp
        TestScheduler.cpp
        TestIOManager.cpp
        TestFdManager.cpp
        TestHook.cpp
        TestDaemon.cpp
)

add_executable(TestCore ${TEST_CORE_SOURCES})
target_link_libraries(TestCore PRIVATE
        Base
        Core
        Catch2::Catch2WithMain
)

target_include_directories(TestCore PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME TestCore COMMAND TestCore)
