set(TEST_BASE_SOURCES
        TestLog.cpp
        TestSingleton.cpp
        TestConfigUtils.cpp
        TestEndian.cpp
        TestMacro.cpp
        TestTimer.cpp
        TestJsonUtils.cpp
        TestHashUtils.cpp
)

add_executable(TestBase ${TEST_BASE_SOURCES})
target_link_libraries(TestBase PRIVATE
        Base
        Catch2::Catch2WithMain
)

target_include_directories(TestBase PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME TestBase COMMAND TestBase)
