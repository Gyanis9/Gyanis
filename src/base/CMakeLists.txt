#set(BASE_SOURCES
#        Mutex.h
#        Mutex.cpp
#        Log.h
#        Log.cpp
#)
#
#
#add_library(Base SHARED ${BASE_SOURCES})
#target_include_directories(Base PUBLIC
#        ${PROJECT_SOURCE_DIR}/src
#)
#
#target_link_libraries(Base PUBLIC
#        yaml-cpp::yaml-cpp
#        nlohmann_json::nlohmann_json
#        openssl::openssl
#        Boost::headers
#)