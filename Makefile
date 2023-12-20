# 编译配置
CC=gcc
CFLAGS=-Isrc/include -Wall
LDFLAGS=-L./lib -lcmark

# 项目路径配置
SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
TESTS_DIR=tests

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TESTS=$(wildcard $(TESTS_DIR)/*.c)

# 目标设置
TARGET=$(BIN_DIR)/webMD

# 创建目录
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

# 编译
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 运行测试
test: $(TARGET)
	$(CC) $(CFLAGS) $(TESTS) -o $(BIN_DIR)/tests
	$(BIN_DIR)/tests

# 清理代码
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# 伪目标
.PHONY: all test clean
