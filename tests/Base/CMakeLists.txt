add_executable(ConfigTest ConfigTest.cpp)

target_link_libraries(ConfigTest PRIVATE
        Base
)

target_compile_definitions(ConfigTest PRIVATE
        CONFIG_TEST_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config"
)