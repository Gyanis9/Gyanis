add_executable(test_log
        test_log.cpp
)

target_link_libraries(test_log PRIVATE
        Base
        pthread
        Catch2::Catch2WithMain
)

target_include_directories(test_log PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

set_target_properties(test_log PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

add_test(NAME test_log COMMAND test_log)
