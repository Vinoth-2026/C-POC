# =========================================================================
#  Makefile for 5G Telecom Performance Engine (POC)
# =========================================================================

# --- Directories ---
SRC_DIR  = src
INC_DIR  = include
OBJ_DIR  = obj
BIN_DIR  = bin
LOG_DIR  = logs
TEST_DIR = test

# --- Executables ---
TARGET_APP  = $(BIN_DIR)/performance_engine
TARGET_TEST = $(BIN_DIR)/test_engine

# --- Compiler & strict warning flags ---
CC = gcc

# _POSIX_C_SOURCE/_DEFAULT_SOURCE are required for localtime_r(), usleep(),
# and friends under -std=c11 (which alone hides them behind feature-test
# macros on glibc).
FEATURE_FLAGS = -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

WARN_FLAGS = -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion \
             -Wshadow -Wformat=2 -Wundef -Wstrict-prototypes \
             -Wmissing-prototypes -Wold-style-definition -Wwrite-strings \
             -Wcast-qual -Wpointer-arith -Werror

CFLAGS  = -std=c11 $(WARN_FLAGS) $(FEATURE_FLAGS) -I$(INC_DIR) -g
LDFLAGS = -lpthread

# --- CUnit auto-detection --------------------------------------------------
# Prefer a real, system-installed CUnit. Fall back to the bundled
# test/mocks/mini_cunit shim (see test/mocks/CUnit/CUnit.h) only if
# libcunit1-dev is not available -- e.g. in network-restricted sandboxes.
HAVE_CUNIT := $(shell echo 'int main(void){return 0;}' | \
              $(CC) -x c - -o /tmp/.cunit_probe -lcunit >/dev/null 2>&1 && \
              echo yes)

ifeq ($(HAVE_CUNIT),yes)
    TEST_CFLAGS   = $(CFLAGS)
    TEST_LDFLAGS  = -lcunit -lpthread
    MOCK_OBJS     =
    CUNIT_STATUS  = "Using system CUnit (libcunit1-dev)"
else
    TEST_CFLAGS   = $(CFLAGS) -I$(TEST_DIR)/mocks
    TEST_LDFLAGS  = -lpthread
    MOCK_OBJS     = $(OBJ_DIR)/test/mini_cunit.o
    CUNIT_STATUS  = "libcunit1-dev NOT AVAILABLE - using bundled test/mocks/mini_cunit shim"
endif

# --- Files ------------------------------------------------------------------
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/src/%.o, $(SRCS))

MAIN_OBJ = $(OBJ_DIR)/src/main.o

# test_DataCollection.c and test_KPI_Collection.c are WHITEBOX tests: they
# #include the corresponding .c file directly to reach static helper
# functions. Their production .o files must therefore be excluded from the
# test link, or the two definitions collide at link time.
WHITEBOX_SRCS  = DataCollection.c KPI_Collection.c login.c Report.c
WHITEBOX_OBJS  = $(patsubst %.c, $(OBJ_DIR)/src/%.o, $(WHITEBOX_SRCS))

PROD_OBJS = $(filter-out $(MAIN_OBJ) $(WHITEBOX_OBJS), $(OBJS))

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/test/%.o, $(TEST_SRCS))

# --- Rules -------------------------------------------------------------------

.PHONY: all clean run_tests directories test debug valgrind helgrind cppcheck coverage

all: directories $(TARGET_APP)

$(TARGET_APP): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Test build ---------------------------------------------------------------
test: directories $(TARGET_TEST)
	@echo $(CUNIT_STATUS)

$(TARGET_TEST): $(PROD_OBJS) $(TEST_OBJS) $(MOCK_OBJS)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LDFLAGS)

$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(OBJ_DIR)/test/mini_cunit.o: $(TEST_DIR)/mocks/mini_cunit.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

run_tests: test
	@mkdir -p $(LOG_DIR)
	@echo "--- Running Test Suite ---"
	@echo $(CUNIT_STATUS)
	./$(TARGET_TEST)

# --- Debug build (no optimization, symbols, for gdb) --------------------------
debug: CFLAGS += -O0 -DDEBUG
debug: clean all

directories:
	@mkdir -p $(OBJ_DIR)/src
	@mkdir -p $(OBJ_DIR)/test
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(LOG_DIR)

# --- Dynamic analysis targets ---------------------------------------------
# Each target checks for its tool and prints the required, honest
# "NOT EXECUTED - TOOL UNAVAILABLE" message instead of silently skipping or
# fabricating a result, per project verification policy.

valgrind: all
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "--- Valgrind Memcheck: $(TARGET_APP) ---"; \
		printf 'vinoth\n12345\n4\n' | valgrind --leak-check=full --show-leak-kinds=all \
			--track-origins=yes --error-exitcode=1 ./$(TARGET_APP); \
	else \
		echo "NOT EXECUTED - TOOL UNAVAILABLE (valgrind)"; \
	fi

helgrind: all
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "--- Helgrind: $(TARGET_APP) ---"; \
		printf 'vinoth\n12345\n4\n' | valgrind --tool=helgrind ./$(TARGET_APP); \
	else \
		echo "NOT EXECUTED - TOOL UNAVAILABLE (valgrind/helgrind)"; \
	fi

cppcheck:
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c11 --inconclusive --force -I$(INC_DIR) $(SRC_DIR); \
	else \
		echo "NOT EXECUTED - TOOL UNAVAILABLE (cppcheck)"; \
	fi

coverage:
	@if command -v gcov >/dev/null 2>&1; then \
		$(MAKE) clean >/dev/null; \
		$(MAKE) test CFLAGS="$(CFLAGS) --coverage" TEST_LDFLAGS="$(TEST_LDFLAGS) --coverage"; \
		./$(TARGET_TEST); \
		echo "--- gcov (note: DataCollection.c/KPI_Collection.c are exercised via"; \
		echo "    whitebox #include in their tests, not as separate test objects,"; \
		echo "    so gcov cannot attribute coverage to them here) ---"; \
		gcov -o $(OBJ_DIR)/src src/Analytic.c src/ErrorLog.c src/Report.c src/login.c || true; \
	else \
		echo "NOT EXECUTED - TOOL UNAVAILABLE (gcov)"; \
	fi

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LOG_DIR)
	rm -f *.gcov $(SRC_DIR)/*.gcda $(SRC_DIR)/*.gcno
