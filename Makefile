# kiwiFS Makefile
# Builds the kiwiFS userspace filesystem simulator.
#
# Targets:
#   make         - Build the kiwifs executable
#   make test    - Build and run all unit tests
#   make clean   - Remove all generated files

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g -I include/
LDFLAGS =

# Directories
SRC_DIR   = src
INC_DIR   = include
TEST_DIR  = tests
BUILD_DIR = build

# Source files (all .c in src/)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files (build/<source>.o)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Main executable
TARGET = kiwifs

# Test source files
TEST_SRCS = $(wildcard $(TEST_DIR)/test_*.c)

# Library objects (everything except main.o, used for linking tests)
LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

# ──────────────────────────────────────────────────────────────
# Default target: build the main executable
# ──────────────────────────────────────────────────────────────
all: $(BUILD_DIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build successful: ./$(TARGET)"

# Compile each source file into an object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ──────────────────────────────────────────────────────────────
# Test target: compile and run each test binary
# ──────────────────────────────────────────────────────────────
.PHONY: test
test: $(BUILD_DIR) $(LIB_OBJS)
	@echo "Running all kiwiFS tests..."
	@PASS=0; FAIL=0; \
	for test_src in $(TEST_SRCS); do \
		test_name=$$(basename $$test_src .c); \
		test_bin=$(BUILD_DIR)/$$test_name; \
		$(CC) $(CFLAGS) -o $$test_bin $$test_src $(LIB_OBJS) $(LDFLAGS) 2>&1; \
		if [ $$? -ne 0 ]; then \
			echo "  [BUILD FAIL] $$test_name"; \
			FAIL=$$((FAIL + 1)); \
		else \
			$$test_bin 2>&1; \
			if [ $$? -eq 0 ]; then \
				echo "  [PASS] $$test_name"; \
				PASS=$$((PASS + 1)); \
			else \
				echo "  [FAIL] $$test_name"; \
				FAIL=$$((FAIL + 1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$PASS passed, $$FAIL failed."

# ──────────────────────────────────────────────────────────────
# Clean target: remove all build artifacts and generated files
# ──────────────────────────────────────────────────────────────
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -f *.img
	@echo "Clean complete."

# ──────────────────────────────────────────────────────────────
# Phony targets declaration
# ──────────────────────────────────────────────────────────────
.PHONY: all clean test
