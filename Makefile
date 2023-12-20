# Compiler settings
CC=gcc
CFLAGS=-Isrc/include -Wall
LDFLAGS=-L./lib -lcmark

# Project settings
SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
TESTS_DIR=tests

# Create a list of source and object files
SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TESTS=$(wildcard $(TESTS_DIR)/*.c)

# Target settings
TARGET=$(BIN_DIR)/postonly

# Create directories
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TARGET)
	$(CC) $(CFLAGS) $(TESTS) -o $(BIN_DIR)/tests
	$(BIN_DIR)/tests

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets
.PHONY: all test clean
