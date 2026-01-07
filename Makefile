# Makefile for llurl - Fast URL Parser

CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99
DEBUG_CFLAGS = -Wall -Wextra -g -std=c99 -fsanitize=address

# Library
LIB_SRC = llurl.c
LIB_OBJ = llurl.o
LIB_STATIC = libllurl.a
LIB_SHARED = libllurl.so

# Test
TEST_SRC = test_llurl.c
TEST_BIN = test_llurl

# Example
EXAMPLE_SRC = example.c
EXAMPLE_BIN = example

.PHONY: all clean test example run-example

all: $(LIB_STATIC) $(LIB_SHARED)

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

# Example binary
$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB_STATIC)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_STATIC)

# Run tests
test: $(TEST_BIN)
	./$(TEST_BIN)

# Build example
example: $(EXAMPLE_BIN)

# Run example
run-example: $(EXAMPLE_BIN)
	./$(EXAMPLE_BIN)

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(LIB_OBJ) $(LIB_STATIC) $(LIB_SHARED) $(TEST_BIN) $(EXAMPLE_BIN)

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
