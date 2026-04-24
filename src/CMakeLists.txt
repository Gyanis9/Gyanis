add_subdirectory(Base)
add_subdirectory(Core)
add_subdirectory(Net)
add_executable(Gyanis main.cpp)

target_link_libraries(Gyanis PRIVATE
        Base
        Core
        Net
)

target_include_directories(Gyanis PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)