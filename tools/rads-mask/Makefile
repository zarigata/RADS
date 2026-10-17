# RADS Masquerading Tool - Makefile
# Version: 0.0.5 "CHAMELEON"

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = rads-mask

# Source files
SRCS = src/simple_transpiler.c
OBJS = $(SRCS:.c=.o)

# Build target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "✓ Built rads-mask transpiler successfully!"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)
	# Only remove GENERATED transpiled outputs (.rads built from .py by
	# `make test`); never tracked test fixtures. The old `rm -f tests/*.rads`
	# here deleted tracked files via the root Makefile's `clean` target —
	# third recurrence, now fixed at the source.

# Run tests.
# The check_* targets assert on the transpiled output itself (brace balance,
# no mangled comparison operators, no Python literal leaks) because merely
# transpiling without error once hid real bugs (nightshift 2026-10-11).
test: $(TARGET) check_all

check_all: $(TARGET)
	@echo "Running transpiler tests..."
	@./$(TARGET) tests/test_hello.py -o tests/test_hello.rads
	@./$(TARGET) tests/test_functions.py -o tests/test_functions.rads
	@./$(TARGET) tests/test_loops.py -o tests/test_loops.rads
	@./$(TARGET) tests/test_assignments.py -o tests/test_assignments.rads
	@./$(TARGET) tests/test_if_elif_else.py -o tests/test_if_elif_else.rads
	@./$(TARGET) tests/test_comprehensive.py -o tests/test_comprehensive.rads
	@# Brace balance: every generated file must have equal { and } counts
	@for f in tests/test_*.rads; do \
		o=$$(grep -o '{' $$f | wc -l); c=$$(grep -o '}' $$f | wc -l); \
		if [ $$o -ne $$c ]; then echo "✗ FAIL brace balance $$f ($$o open vs $$c close)"; exit 1; fi; done
	@# No mangled comparison operators from the assignment rule
	@if grep -n '= =' tests/test_assignments.rads tests/test_comprehensive.rads; then echo "✗ FAIL mangled '==''"; exit 1; fi
	@# No Python literal leaks in return statements
	@if grep -nE 'return (True|False|None)' tests/*.rads; then echo "✗ FAIL Python literal leaked"; exit 1; fi
	@# No stray lone ';' lines
	@if grep -nE '^\s*;\s*$$' tests/*.rads; then echo "✗ FAIL stray ';' line"; exit 1; fi
	@echo "✓ All tests transpiled successfully!"

.PHONY: all clean test check_all install

# Install to system
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	chmod +x /usr/local/bin/$(TARGET)
	@echo "✓ Installed rads-mask to /usr/local/bin/"

.PHONY: all clean test install
