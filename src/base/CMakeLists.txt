set(sources
        Log.h
        Log.cpp
        Singleton.h
        NonCopyable.h
        Config.h
        Config.cpp
        Utils.h
        Utils.cpp

        #        Endian.h

        #        Macro.h
        #
        #        Timer.h
        #        Timer.cpp
        #
        #        JsonUtils.h
        #        JsonUtils.cpp
        #
        #        HashUtils.h
        #        HashUtils.cpp
)

add_library(Base SHARED ${sources})
if (WIN32)
    set_target_properties(Base PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()
target_include_directories(Base PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)
target_link_libraries(Base PUBLIC
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
)
