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
	rm -f tests/*.rads
	@echo "✓ Cleaned build artifacts"

# Run tests
test: $(TARGET)
	@echo "Running transpiler tests..."
	@./$(TARGET) tests/test_hello.py -o tests/test_hello.rads
	@./$(TARGET) tests/test_functions.py -o tests/test_functions.rads
	@./$(TARGET) tests/test_loops.py -o tests/test_loops.rads
	@echo "✓ All tests transpiled successfully!"

# Install to system
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	chmod +x /usr/local/bin/$(TARGET)
	@echo "✓ Installed rads-mask to /usr/local/bin/"

.PHONY: all clean test install
