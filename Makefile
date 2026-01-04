# RADS Compiler Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
DEBUG_FLAGS = -g -DDEBUG
SRC_DIR = src
BUILD_DIR = build
TARGET = rads

# Source files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c
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
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)
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
	@echo "✅ RADS installed to /usr/local/bin/"

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "🗑️  RADS uninstalled"

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
