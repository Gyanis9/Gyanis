#!/bin/sh
# 项目初始化脚本 - 用于快速创建基于Gyanis框架的项目模板

###########################################
# 函数定义部分
###########################################

# 带错误检查的命令执行函数
# 参数: 需要执行的命令
# 功能: 执行命令并检查返回状态码，失败时退出脚本
command_error_exit() {
    # 执行传入的命令
    # shellcheck disable=SC2048
    $*

    # 检查上一条命令的返回状态码
    # shellcheck disable=SC2181
    if [ $? -ne 0 ]; then
        echo "命令执行失败: $*"
        exit 1
    fi
}

###########################################
# 主程序部分
###########################################

# 参数校验
if [ $# -lt 2 ]; then
    echo "使用方法: $0 项目名称 命名空间"
    echo "示例: $0 my_project Gyanis"
    exit 1
fi

# 参数赋值
project_name=$1      # 项目名称参数
namespace=$2         # 命名空间参数
Gyanis_repo="git@github.com:Gyanis9/Gyanis.git"  # Gyanis仓库地址
template_dir="template"                              # 模板目录名称

echo "▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄"
echo "开始创建项目: [${project_name}] 命名空间: [${namespace}]"

# 创建项目目录
echo "───────────────────────────────────────"
echo "步骤1/7：创建项目目录..."
command_error_exit mkdir -p "${project_name}"
command_error_exit cd "${project_name}"

# 克隆Gyanis仓库
echo "───────────────────────────────────────"
echo "步骤2/7：克隆Gyanis仓库..."
command_error_exit git clone "${Gyanis_repo}"

# 复制构建文件
echo "───────────────────────────────────────"
echo "步骤3/7：复制构建文件..."
command_error_exit cp Gyanis/Makefile .

# 处理模板文件
echo "───────────────────────────────────────"
echo "步骤4/7：处理模板文件..."
command_error_exit cp -rf Gyanis/"${template_dir}"/* .
command_error_exit mv "${template_dir}" "${namespace}"

# 配置CMakeLists.txt
echo "───────────────────────────────────────"
echo "步骤5/7：配置CMakeLists.txt..."
sed -i "s/project_name/${project_name}/g" CMakeLists.txt
sed -i "s/template/${namespace}/g" CMakeLists.txt

# 配置移动脚本
echo "───────────────────────────────────────"
echo "步骤6/7：配置部署脚本..."
sed -i "s/project_name/${project_name}/g" move.sh

# 替换命名空间和项目名称
echo "───────────────────────────────────────"
echo "步骤7/7：替换模板内容..."
# 在源代码文件中进行替换
find "${namespace}" -type f -exec sed -i \
    -e "s/name_space/${namespace}/g" \
    -e "s/project_name/${project_name}/g" {} \;

# 在配置文件中进行替换
find bin/config -type f -exec sed -i \
    -e "s/name_space/${namespace}/g" \
    -e "s/project_name/${project_name}/g" {} \;

echo "▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀"
echo "项目创建成功！"
echo "项目目录结构:"
tree -L 2