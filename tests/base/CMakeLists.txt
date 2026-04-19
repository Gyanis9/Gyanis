set(TEST_BASE_SOURCES
        TestLog.cpp
        TestSingleton.cpp
        TestConfigUtils.cpp
)

add_executable(TestLog ${TEST_BASE_SOURCES})
target_link_libraries(TestLog PRIVATE
        Base
        Catch2::Catch2WithMain
)

target_include_directories(TestLog PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME TestLog COMMAND TestLog)
