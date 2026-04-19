set(TEST_BASE_SOURCES
        test_log_stress.cpp
)

add_executable(test_base_log_stress ${TEST_BASE_SOURCES})
target_link_libraries(test_base_log_stress PRIVATE
        Base
        Catch2::Catch2WithMain
)

target_include_directories(test_base_log_stress PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME test_base_log_stress COMMAND test_base_log_stress)
