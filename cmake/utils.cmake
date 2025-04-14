#[[
强制重新定义源文件的 __FILE__ 宏为相对路径
--------------------------------------------
功能：
  将指定目标的所有源文件中的 __FILE__ 宏重定义为相对于项目根目录的路径
  例如：/home/project/src/main.cpp 会显示为 src/main.cpp

实现原理：
  使用编译器提供的宏前缀重映射功能（-fmacro-prefix-map），该方案：
  - 比手动定义 __FILE__ 更安全高效
  - 支持现代编译器（GCC>=8、Clang>=10）
  - 兼容Ninja、Makefile等生成器

参数：
  targetname - 需要处理的目标名称
]]
function(force_redefine_file_macro_for_sources targetname)
    # 参数校验
    if(NOT TARGET ${targetname})
        message(FATAL_ERROR "目标 ${targetname} 不存在")
    endif()

    # 获取目标的所有源文件（排除头文件）
    get_target_property(source_files ${targetname} SOURCES)
    if(NOT source_files)
        message(WARNING "目标 ${targetname} 没有源文件")
        return()
    endif()

    # 计算需要映射的路径前缀
    file(REAL_PATH ${PROJECT_SOURCE_DIR} project_dir)
    string(REGEX REPLACE "/$" "" project_dir ${project_dir})  # 去除尾部斜杠

    # 根据编译器类型设置映射参数
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC/Clang 使用 -fmacro-prefix-map 方案
        target_compile_options(${targetname} PRIVATE
                "-fmacro-prefix-map=${project_dir}/="
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # MSVC 使用 /experimental:deterministic 方案
        target_compile_options(${targetname} PRIVATE
                "/experimental:deterministic"
                "/Zc:__FILE__"
        )
    else()
        message(WARNING "不支持的编译器：${CMAKE_CXX_COMPILER_ID}，无法重定义__FILE__宏")
        return()
    endif()

    # 添加调试信息（仅在VERBOSE模式显示）
    message(VERBOSE
            "目标 ${targetname} 已启用路径映射：\n"
            "  原始路径前缀：${project_dir}/\n"
            "  映射为前缀："
    )
endfunction()


#[[
函数：生成 Ragel 状态机代码
功能：将 Ragel 状态机文件编译为 C++ 源代码
参数：
  INPUT_RL_FILE   - 输入 Ragel 文件路径
  OUTPUT_LIST     - 输出文件列表变量名
  OUTPUT_DIR      - 生成文件输出目录
生成文件：
  <源文件名>.rl.cpp
]]
function(RAGEL_CODE_GENERATOR INPUT_RL_FILE OUTPUT_LIST OUTPUT_DIR)
    # 获取不含扩展名的文件名
    get_filename_component(RL_BASE_NAME ${INPUT_RL_FILE} NAME_WE)

    # 设置输出文件路径
    set(GEN_SOURCE_FILE ${OUTPUT_DIR}/${RL_BASE_NAME}.cpp)

    # 将生成文件添加到输出列表
    set(${OUTPUT_LIST} ${${OUTPUT_LIST}} ${GEN_SOURCE_FILE} PARENT_SCOPE)

    # 添加自定义构建命令
    add_custom_command(
            OUTPUT ${GEN_SOURCE_FILE}
            COMMAND ragel
            ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_RL_FILE}
            -o ${GEN_SOURCE_FILE}
            -l -C -G2
            WORKING_DIRECTORY ${OUTPUT_DIR}  # 更安全的目录切换方式
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_RL_FILE}
            COMMENT "Generating Ragel code: ${RL_BASE_NAME}.cpp"
    )

    # 标记为生成文件（避免编译警告）
    set_source_files_properties(
            ${GEN_SOURCE_FILE}
            PROPERTIES GENERATED TRUE
    )
endfunction()

#[[
函数：生成 Protocol Buffers 代码
功能：将 .proto 文件编译为 C++ 源代码
参数：
  INPUT_PROTO_FILE - 输入 Protobuf 文件路径
  OUTPUT_LIST      - 输出文件列表变量名
  OUTPUT_DIR       - 生成文件输出目录
生成文件：
  <源文件名>.pb.cc
  <源文件名>.pb.h（隐式生成）
]]
function(PROTOBUF_CODE_GENERATOR INPUT_PROTO_FILE OUTPUT_LIST OUTPUT_DIR)
    # 获取文件信息
    get_filename_component(PROTO_BASE_NAME ${INPUT_PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_RELATIVE_PATH ${INPUT_PROTO_FILE} DIRECTORY)

    # 设置输出文件路径（保持源目录结构）
    set(PROTO_GEN_SOURCE
            ${OUTPUT_DIR}/${PROTO_RELATIVE_PATH}/${PROTO_BASE_NAME}.pb.cc)

    # 将生成文件添加到输出列表
    set(${OUTPUT_LIST} ${${OUTPUT_LIST}} ${PROTO_GEN_SOURCE} PARENT_SCOPE)

    # 添加自定义构建命令
    add_custom_command(
            OUTPUT ${PROTO_GEN_SOURCE}
            COMMAND protoc
            --cpp_out=${OUTPUT_DIR}
            -I${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_PROTO_FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_PROTO_FILE}
            COMMENT "Generating Protobuf code: ${PROTO_BASE_NAME}.pb.cc"
    )

    # 标记为生成文件
    set_source_files_properties(
            ${PROTO_GEN_SOURCE}
            PROPERTIES GENERATED TRUE
    )
endfunction()