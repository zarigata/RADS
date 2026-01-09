# RADS Compiler Makefile

UNAME_S := $(shell uname -s)
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Isrc/core -Isrc/stdlib -D_GNU_SOURCE
LIBS = -lm
LDFLAGS =
TARGET = rads

# Dependency check for libuv
# Find uv.h
UV_H := $(shell find $(UV_INCLUDE) /usr/include /usr/local/include -name uv.h -print -quit)
ifndef UV_H
    $(error "libuv header (uv.h) not found. Please install libuv-dev or set the UV_INCLUDE environment variable.")
endif

ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
    LIBS += -luv -lpthread
endif
ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
    LIBS += -luv
    CFLAGS += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib
endif
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    LIBS += -luv -lws2_32 -lpsapi -liphlpapi -luserenv
    TARGET := rads.exe
endif

# Optional override if libuv is installed in a non-standard prefix
# export UV_INCLUDE=/usr/local/include
# export UV_LIB=/usr/local/lib
ifneq ($(UV_INCLUDE),)
    CFLAGS += -I$(UV_INCLUDE)
endif
ifneq ($(UV_LIB),)
    LDFLAGS += -L$(UV_LIB)
endif
DEBUG_FLAGS = -g -DDEBUG
SRC_CORE_DIR = src/core
SRC_STDLIB_DIR = src/stdlib
BUILD_DIR = build

# VPATH for finding source files in different directories
VPATH =

# Source files
CORE_SOURCES = $(wildcard $(SRC_CORE_DIR)/*.c)
STDLIB_SOURCES = $(wildcard $(SRC_STDLIB_DIR)/*.c)
SOURCES = $(CORE_SOURCES) $(STDLIB_SOURCES)

OBJECTS = $(patsubst $(SRC_CORE_DIR)/%.c,$(BUILD_DIR)/core/%.o,$(CORE_SOURCES)) \
          $(patsubst $(SRC_STDLIB_DIR)/%.c,$(BUILD_DIR)/stdlib/%.o,$(STDLIB_SOURCES))

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/core $(BUILD_DIR)/stdlib

# Compile object files from core and stdlib with explicit paths
$(BUILD_DIR)/core/%.o: $(SRC_CORE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/stdlib/%.o: $(SRC_STDLIB_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $(TARGET)
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
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	# Build rpm before install if it doesn't exist
	gcc -D_POSIX_C_SOURCE=200809L -Isrc tools/rpm/rpm.c -o tools/rpm/rpm
	install -m 755 tools/rpm/rpm /usr/local/bin/
	@echo "✅ RADS and RPM installed to /usr/local/bin/"

# Platform targets
linux: all
macos: all
windows: all


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
