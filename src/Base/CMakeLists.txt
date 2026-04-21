add_library(Base
        ConfigType.cpp
        ConfigType.h
        ConfigValue.cpp
        ConfigValue.h
        ConfigFileWatcher.h
        ConfigManager.h
        ConfigManager.cpp
        ConfigFileWatcher.cpp
        Expection.cpp
        Expection.h
)

target_link_libraries(Base PUBLIC
        yaml-cpp::yaml-cpp
)

## 平台特定链接
#if (LINUX)
#    # inotify 不需要额外链接库
#elseif (WIN32)
#    # ReadDirectoryChangesW 在 kernel32 中
#    target_link_libraries(Base PRIVATE kernel32)
#endif ()
