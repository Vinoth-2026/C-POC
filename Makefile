# =========================================================================
#  Makefile for 5G Telecom Performance Engine (POC)
# =========================================================================

# --- Directories ---
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
LOG_DIR = logs
TEST_DIR = test

# --- Executables ---
TARGET_APP  = $(BIN_DIR)/performance_engine
TARGET_TEST = $(BIN_DIR)/test_engine

# --- Compiler & Flags ---
CC = gcc
# Enforce MISRA-C level (C99 is often used) and enable warnings
# Adding -g for debug symbols is essential for Valgrind/Helgrind analysis.
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g
# Include path for application headers
CFLAGS += -I$(INC_DIR)

# Linker flags for app (requires pthreads)
LDFLAGS = -lpthread

# Linker flags for tests (requires CUnit and pthreads)
TEST_LDFLAGS = -lcunit -lpthread

# --- Files ---

# Production Files
# We find all .c files in src/ and create parallel .o file lists in obj/src/
SRCS      = $(wildcard $(SRC_DIR)/*.c)
OBJS      = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/src/%.o, $(SRCS))

# To link the test executable, we need all production objects *except* main.o
MAIN_OBJ  = $(OBJ_DIR)/src/main.o
PROD_OBJS = $(filter-out $(MAIN_OBJ), $(OBJS))

# Test Files
# We find all .c files in test/ and create parallel .o file lists in obj/test/
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/test/%.o, $(TEST_SRCS))


# --- Rules ---

# Special targets that aren't file names
.PHONY: all clean run_tests directories test

# Default target: build the main application
all: directories $(TARGET_APP)

# Link Application
# The output goes to bin/
$(TARGET_APP): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile Production Source
# We create a specific rule for src/ files to output to obj/src/
$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build Test Executable
test: directories $(TARGET_TEST)

# Link Test Executable
# Links in production objects (sans main.o) and the test objects with CUnit flags
$(TARGET_TEST): $(PROD_OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(TEST_LDFLAGS)

# Compile Test Source
# We create a specific rule for test/ files to output to obj/test/
$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Convenient target to build and then execute tests
run_tests: test
	@mkdir -p $(LOG_DIR)
	@echo "\n--- Running CUnit Tests ---"
	./$(TARGET_TEST)

# Target to ensure build directories exist
directories:
	@mkdir -p $(OBJ_DIR)/src
	@mkdir -p $(OBJ_DIR)/test
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(LOG_DIR)

# Target to remove all build artifacts and logs
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LOG_DIR)