add_executable(BaseTest
        ConfigTest.cpp
        LoggerTest.cpp
)

target_link_libraries(BaseTest PRIVATE
        Base
        Catch2::Catch2WithMain
)

target_compile_definitions(BaseTest PRIVATE
        CONFIG_TEST_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config"
)

add_test(NAME BaseTest COMMAND BaseTest "~[slow]")
set_tests_properties(BaseTest PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(BaseTest PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)