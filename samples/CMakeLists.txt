add_executable(my_http_server my_http_server.cpp)
target_link_libraries(my_http_server base net core)
target_include_directories(my_http_server PRIVATE ../include)
force_redefine_file_macro_for_sources(my_http_server)

