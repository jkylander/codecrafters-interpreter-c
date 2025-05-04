# Compiler and flags
CC = clang
CFLAGS_COMMON = -Wall -Wextra -std=c23
CFLAGS_COMMON += -Wno-unused-command-line-argument
DEPFLAGS = -MMD -MP
EXECUTABLE = interpreter
CLI_ARGS = run
INPUT_FILE = class.lox

# Build modes
ifeq ($(BUILD),)
  override BUILD = debug
  $(info Using default MODE = $(BUILD).)
endif

ifeq ($(BUILD),debug)
	CFLAGS = $(CFLAGS_COMMON) -g -Og -DDEBUG #-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
else ifeq ($(BUILD),release)
	CFLAGS = $(CFLAGS_COMMON) -O3 -flto -march=native
else ifeq ($(BUILD),coverage)
	CFLAGS = $(CFLAGS_COMMON) -Og -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fprofile-instr-generate -fcoverage-mapping
else
	CFLAGS = $(CFLAGS_COMMON)
endif

# Directories
SRC_DIR = src
TEST_DIR = tests
BENCH_DIR = bench
OBJ_DIR = obj
BIN_DIR = build
TEST_OBJ_DIR = $(OBJ_DIR)/tests
TEST_BIN_DIR = $(BIN_DIR)/tests
BENCH_OBJ_DIR = $(OBJ_DIR)/bench
BENCH_BIN_DIR = $(BIN_DIR)/bench
COV_DIR = coverage

# Main program sources and objects
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
DEPS = $(OBJ_FILES:.o=.d)

# Test sources and objects (excluding main.c)
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ = $(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SRC))
TEST_DEPS = $(TEST_OBJ:.o=.d)


# Bench sources and objects (excluding main.c)
BENCH_SRC = $(wildcard $(BENCH_DIR)/*.c)
BENCH_OBJ = $(patsubst $(BENCH_DIR)/%.c,$(BENCH_OBJ_DIR)/%.o,$(BENCH_SRC))
BENCH_DEPS = $(BENCH_OBJ:.o=.d)

# Main program object files excluding main.c for test builds
MAIN_FILE = $(SRC_DIR)/main.c
LIB_SRC = $(filter-out $(MAIN_FILE),$(SRC_FILES))
LIB_OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRC))

# Target executables
MAIN_TARGET = $(BIN_DIR)/$(EXECUTABLE)
TEST_TARGETS = $(patsubst $(TEST_DIR)/%.c,$(TEST_BIN_DIR)/%,$(TEST_SRC))
BENCH_TARGETS = $(patsubst $(BENCH_DIR)/%.c,$(BENCH_BIN_DIR)/%,$(BENCH_SRC))
ifeq ($(OS),Windows_NT)
#	LDLIBS = -Wl,/subsystem:console
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS
	MAIN_TARGET := $(MAIN_TARGET).exe

else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		LDLIBS = -lm
	endif
	ifeq ($(UNAME_S),Darwin)
		LDLIBS = -lm
	endif
endif

# Default target
all: $(MAIN_TARGET)

# Main program
$(MAIN_TARGET): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Test targets
test: $(TEST_TARGETS)
	@echo "Running tests..."
	@for test in $(TEST_TARGETS) ; do \
		echo "Running $$test" ; \
		$$test || exit 1 ; \
	done
	@echo "All tests passed!"


# Bench targets
bench: $(BENCH_TARGETS)
	@echo "Running benchmarks..."
	@for bench in $(BENCH_TARGETS) ; do \
		echo "Running $$bench" ; \
		$$bench || exit 1 ; \
	done
	@echo "All benchmarks complete!"

coverage-build: clean
	$(MAKE) BUILD=coverage
coverage-run:
	mkdir -p $(COV_DIR)
	cd $(COV_DIR) && ../$(MAIN_TARGET) $(CLI_ARGS) ../bench.lox

coverage-process:
	llvm-profdata merge -sparse $(COV_DIR)/default.profraw -o $(COV_DIR)/coverage.profdata

coverage-report:
	llvm-cov show $(MAIN_TARGET) -instr-profile=$(COV_DIR)/coverage.profdata -format=html -output-dir=$(COV_DIR)
	llvm-cov report $(MAIN_TARGET) -instr-profile=$(COV_DIR)/coverage.profdata

coverage: coverage-build coverage-run coverage-process coverage-report

# Compile individual test executables
$(TEST_BIN_DIR)/%: $(TEST_OBJ_DIR)/%.o $(LIB_OBJ)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile test source files to object files
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I$(SRC_DIR) -c $< -o $@


# Compile individual bench executables
$(BENCH_BIN_DIR)/%: $(BENCH_OBJ_DIR)/%.o $(LIB_OBJ)
	@mkdir -p $(BENCH_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile bench source files to object files
$(BENCH_OBJ_DIR)/%.o: $(BENCH_DIR)/%.c
	@mkdir -p $(BENCH_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I$(SRC_DIR) -c $< -o $@

# Include generated dependency files
-include $(DEPS)
-include $(TEST_DEPS)
-include $(BENCH_DEPS)
run:
	./$(MAIN_TARGET) $(CLI_ARGS) $(INPUT_FILE)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

help:
	@echo "Usage:"
	@echo "  make                   - Build main program with default flags"
	@echo "  make BUILD=debug       - Build with debug flags"
	@echo "  make BUILD=release     - Build with release flags"
	@echo "  make test              - Build and run unit tests"
	@echo "  make coverage          - Build and generate coverage report"
	@echo "  make bench             - Build and run benchmarks"
	@echo "  make clean             - Remove build artifacts"

# Phony targets
.PHONY: all clean help test run coverage bench

