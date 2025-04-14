# 声明伪目标，防止与同名文件冲突
.PHONY: default all clean help

# 定义常用变量
BUILD_DIR    := build
CMAKE_FLAGS  := -DCMAKE_BUILD_TYPE=Release
MAKE_FLAGS   := -j$(shell nproc)

# 定义编译器路径变量
CC  := $(shell which gcc)
CXX := $(shell which g++)

# 默认目标：构建项目
default: all

# 主构建目标
all: $(BUILD_DIR)/CMakeCache.txt
	@echo "▶ 开始构建项目..."
	cd $(BUILD_DIR) && make $(MAKE_FLAGS)

# CMake配置检查规则
$(BUILD_DIR)/CMakeCache.txt:
	@echo "▷ 初始化构建目录..."
	mkdir -p $(BUILD_DIR)
	@echo "▷ 运行CMake配置..."
	cd $(BUILD_DIR) && cmake \
		-DCMAKE_C_COMPILER:FILEPATH=$(CC) \
		-DCMAKE_CXX_COMPILER:FILEPATH=$(CXX) \
		$(CMAKE_FLAGS) ..

# 清理规则
clean:
	@echo "▶ 清理构建产物..."
	if [ -d "$(BUILD_DIR)" ]; then \
		cd $(BUILD_DIR) && make clean; \
	else \
		echo "✔ 构建目录不存在，无需清理"; \
	fi

# 帮助信息
help:
	@echo "使用方法:"
	@echo "  make           - 构建项目 (默认)"
	@echo "  make clean     - 清理构建产物"
	@echo "  make help      - 显示帮助信息"
	@echo "  make install   - 安装目标文件"
	@echo "  make test      - 运行测试套件"

# 通配规则处理其他make目标（install/test等）
%: $(BUILD_DIR)/CMakeCache.txt
	@echo "▶ 执行目标：$@"
	cd $(BUILD_DIR) && make $(MAKE_FLAGS) $@