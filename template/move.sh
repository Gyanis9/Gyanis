#!/bin/sh
# 项目部署脚本 - 用于构建产物管理和模块部署

# 遇到错误立即退出脚本
set -e

# 配置区（可根据项目需求修改）
PROJECT_NAME="project_name"     # 项目名称
SRC_EXECUTABLE="Gyanis/bin/Gyanis"  # 源可执行文件路径
BIN_DIR="bin"                   # 输出目录
MODULE_DIR="${BIN_DIR}/module"  # 模块目录

# 创建输出目录结构
echo "▷ 创建目录结构..."
mkdir -p "${MODULE_DIR}"

# 清理旧部署文件
echo "▷ 清理旧部署文件..."
# 使用-f参数避免文件不存在时报错
rm -f "${BIN_DIR}/${PROJECT_NAME}"
rm -f "${MODULE_DIR}/lib${PROJECT_NAME}.so"

# 部署可执行文件
echo "▶ 部署可执行文件..."
if [ ! -f "${SRC_EXECUTABLE}" ]; then
    echo "❌ 错误：可执行文件不存在 ${SRC_EXECUTABLE}"
    exit 1
fi
cp -v "${SRC_EXECUTABLE}" "${BIN_DIR}/${PROJECT_NAME}"
chmod +x "${BIN_DIR}/${PROJECT_NAME}"  # 确保可执行权限

# 部署共享库
echo "▶ 部署共享库..."
LIB_SOURCE="lib/lib${PROJECT_NAME}.so"
if [ ! -f "${LIB_SOURCE}" ]; then
    echo "❌ 错误：共享库不存在 ${LIB_SOURCE}"
    exit 1
fi
cp -v "${LIB_SOURCE}" "${MODULE_DIR}/"

echo "✔ 部署完成！"
echo "可执行文件路径: ${BIN_DIR}/${PROJECT_NAME}"
echo "共享库路径: ${MODULE_DIR}/lib${PROJECT_NAME}.so"