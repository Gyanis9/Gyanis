set(sources
        Log.h
        Log.cpp

        Singleton.h
#        Mutex.h
#        Mutex.cpp
        #
        #        # 工具类
        #        NonCopyable.h
        #        Config.h
        #        Config.cpp
        #        Endian.h
        #        Utils.h
        #        Utils.cpp
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
)
target_link_libraries(Base PUBLIC
        yaml-cpp::yaml-cpp
)
