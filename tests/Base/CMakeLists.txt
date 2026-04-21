add_executable(ConfigTest ConfigTest.cpp)

target_link_libraries(ConfigTest PRIVATE
        Base
        Catch2::Catch2WithMain
)

target_compile_definitions(ConfigTest PRIVATE
        CONFIG_TEST_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config"
)

add_test(NAME Base.ConfigTest COMMAND ConfigTest "~[slow]")
set_tests_properties(Base.ConfigTest PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(LoggerTest LoggerTest.cpp)


target_link_libraries(LoggerTest PRIVATE
        Base
        Catch2::Catch2WithMain
)

target_compile_definitions(LoggerTest PRIVATE
        CONFIG_TEST_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config"
)

add_test(NAME Base.LoggerTest COMMAND LoggerTest "~[slow]")
set_tests_properties(Base.LoggerTest PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)