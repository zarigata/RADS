# RADS Compiler Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LIBS = -lm
DEBUG_FLAGS = -g -DDEBUG
SRC_DIR = src
BUILD_DIR = build
TARGET = rads

# Source files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c $(SRC_DIR)/ast.c $(SRC_DIR)/parser.c $(SRC_DIR)/interpreter.c $(SRC_DIR)/stdlib_io.c $(SRC_DIR)/stdlib_media.c $(SRC_DIR)/stdlib_net.c $(SRC_DIR)/stdlib_ffi.c $(SRC_DIR)/stdlib_string.c $(SRC_DIR)/stdlib_math.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)
	@echo "✅ RADS compiler built successfully!"

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "🧹 Cleaned build artifacts"

# Install (copy to /usr/local/bin)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	# Build rpm before install if it doesn't exist
	gcc -D_POSIX_C_SOURCE=200809L -Isrc tools/rpm/rpm.c -o tools/rpm/rpm
	install -m 755 tools/rpm/rpm /usr/local/bin/
	@echo "✅ RADS and RPM installed to /usr/local/bin/"

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	rm -f /usr/local/bin/rpm
	@echo "🗑️  RADS and RPM uninstalled"

# Run tests (placeholder)
test: $(TARGET)
	@echo "Running tests..."
	./$(TARGET) --version
	./$(TARGET) -t examples/hello_world.rads

# Help
help:
	@echo "RADS Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build RADS compiler (default)"
	@echo "  debug      - Build with debug symbols"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  test       - Run basic tests"
	@echo "  help       - Show this help message"

.PHONY: all debug clean install uninstall test help
