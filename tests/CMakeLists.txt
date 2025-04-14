
    #=========================test log========================#
    add_executable(test_log test_log.cpp)
    target_link_libraries(test_log PRIVATE base)
    target_include_directories(test_log PRIVATE ../include)
    force_redefine_file_macro_for_sources(test_log)

    #=========================test config========================#
    add_executable(test_config test_config.cpp)
    target_link_libraries(test_config PRIVATE base)
    target_include_directories(test_config PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_config)

    #=========================test timer========================#
    add_executable(test_timer test_timer.cpp)
    target_link_libraries(test_timer PRIVATE base)
    target_include_directories(test_timer PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_timer)

    #=========================test json========================#
    add_executable(test_json test_json.cpp)
    target_link_libraries(test_json PRIVATE base)
    target_include_directories(test_json PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_json)

    #=========================test fiber========================#
    add_executable(test_fiber test_fiber.cpp)
    target_link_libraries(test_fiber PRIVATE base core)
    target_include_directories(test_fiber PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_fiber)

    #=========================test scheduler========================#
    add_executable(test_scheduler test_scheduler.cpp)
    target_link_libraries(test_scheduler PRIVATE base core)
    target_include_directories(test_scheduler PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_scheduler)

    #=========================test iomanager========================#
    add_executable(test_iomanager test_iomanager.cpp)
    target_link_libraries(test_iomanager PRIVATE base core)
    target_include_directories(test_iomanager PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_iomanager)

    #=========================test hook========================#
    add_executable(test_hook test_hook.cpp)
    target_link_libraries(test_hook PRIVATE base core)
    target_include_directories(test_hook PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_hook)

    #=========================test daemon========================#
    add_executable(test_daemon test_daemon.cpp)
    target_link_libraries(test_daemon PRIVATE base core)
    target_include_directories(test_daemon PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_daemon)

    #=========================test daemon========================#
    add_executable(test_env test_env.cpp)
    target_link_libraries(test_env PRIVATE base core)
    target_include_directories(test_env PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_env)

    #=========================test bytearray========================#
    add_executable(test_bytearray test_bytearray.cpp)
    target_link_libraries(test_bytearray PRIVATE base core net)
    target_include_directories(test_bytearray PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_bytearray)

    #=========================test mysql========================#
    add_executable(test_mysql test_mysql.cpp)
    target_link_libraries(test_mysql PRIVATE base db core)
    target_include_directories(test_mysql PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_mysql)

    #=========================test redis========================#
    add_executable(test_redis test_redis.cpp)
    target_link_libraries(test_redis PRIVATE base db core)
    target_include_directories(test_redis PUBLIC ../include)
    force_redefine_file_macro_for_sources(test_redis)
