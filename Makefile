# Makefile for llurl - Fast URL Parser

CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -funroll-loops
DEBUG_CFLAGS = -Wall -Wextra -g -std=c99 -fsanitize=address

# SIMD flags - auto-detect architecture
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
  # x86_64: Enable SSE2 (baseline) and optionally AVX2
  SIMD_CFLAGS = -msse2
  # Uncomment to enable AVX2 for better performance (requires AVX2-capable CPU)
  # SIMD_CFLAGS = -msse2 -mavx2
else ifeq ($(ARCH),aarch64)
  # ARM64: Enable NEON
  SIMD_CFLAGS = -march=armv8-a+simd
else ifeq ($(ARCH),arm64)
  # ARM64 (macOS naming): Enable NEON
  SIMD_CFLAGS = -march=armv8-a+simd
endif

# Add SIMD flags to default CFLAGS
CFLAGS += $(SIMD_CFLAGS)

# Library
LIB_SRC = llurl.c
LIB_OBJ = llurl.o
LIB_STATIC = libllurl.a
LIB_SHARED = libllurl.so

# Test
TEST_SRC = test_llurl.c
TEST_BIN = test_llurl

# Comprehensive Test
TEST_COMP_SRC = test_llurl_comprehensive.c
TEST_COMP_BIN = test_llurl_comprehensive

# Example
EXAMPLE_SRC = example.c
EXAMPLE_BIN = example

# Benchmark
BENCH_SRC = benchmark.c
BENCH_BIN = benchmark
BENCH_CFLAGS = $(CFLAGS) -D_POSIX_C_SOURCE=199309L

.PHONY: all clean test test-all test-comprehensive example run-example benchmark run-benchmark show-simd

all: $(LIB_STATIC) $(LIB_SHARED) benchmark

# Show SIMD configuration
show-simd:
	@echo "Architecture: $(ARCH)"
	@echo "SIMD flags: $(SIMD_CFLAGS)"

# Static library
$(LIB_STATIC): $(LIB_OBJ)
	ar rcs $@ $^

# Shared library
$(LIB_SHARED): $(LIB_SRC)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^

# Object file
$(LIB_OBJ): $(LIB_SRC) llurl.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Test binary
$(TEST_BIN): $(TEST_SRC) $(LIB_STATIC)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_STATIC)

# Comprehensive test binary
$(TEST_COMP_BIN): $(TEST_COMP_SRC) $(LIB_STATIC)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_STATIC)

# Example binary
$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB_STATIC)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_STATIC)

# Benchmark binary
$(BENCH_BIN): $(BENCH_SRC) $(LIB_STATIC)
	$(CC) $(BENCH_CFLAGS) -o $@ $< $(LIB_STATIC)

# Run basic tests
test: $(TEST_BIN)
	./$(TEST_BIN)

# Run comprehensive tests
test-comprehensive: $(TEST_COMP_BIN)
	./$(TEST_COMP_BIN)

# Run all tests
test-all: test test-comprehensive

# Build example
example: $(EXAMPLE_BIN)

# Run example
run-example: $(EXAMPLE_BIN)
	./$(EXAMPLE_BIN)

# Build benchmark
benchmark: $(BENCH_BIN)

# Run benchmark
run-benchmark: $(BENCH_BIN)
	./$(BENCH_BIN)

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(LIB_OBJ) $(LIB_STATIC) $(LIB_SHARED) $(TEST_BIN) $(TEST_COMP_BIN) $(EXAMPLE_BIN) $(BENCH_BIN)

# Install (optional)
install: $(LIB_STATIC) $(LIB_SHARED)
	install -d /usr/local/lib
	install -m 644 $(LIB_STATIC) /usr/local/lib/
	install -m 755 $(LIB_SHARED) /usr/local/lib/
	install -d /usr/local/include
	install -m 644 llurl.h /usr/local/include/

uninstall:
	rm -f /usr/local/lib/$(LIB_STATIC)
	rm -f /usr/local/lib/$(LIB_SHARED)
	rm -f /usr/local/include/llurl.h
