add_executable(test_base_log
        test_log.cpp
)

if (TARGET Base)
    target_link_libraries(test_base_log PRIVATE
            Base
    )
else ()
    target_sources(test_base_log PRIVATE
            ${PROJECT_SOURCE_DIR}/src/base/Log.cpp
            ${PROJECT_SOURCE_DIR}/src/base/Mutex.cpp
    )
    target_link_libraries(test_base_log PRIVATE
            yaml-cpp::yaml-cpp
            nlohmann_json::nlohmann_json
            openssl::openssl
            Boost::headers
    )
endif ()

target_link_libraries(test_base_log PRIVATE
        pthread
        Catch2::Catch2WithMain
)

target_include_directories(test_base_log PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

set_target_properties(test_base_log PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

add_test(NAME test_base_log COMMAND test_base_log)
