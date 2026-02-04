# RADS Compiler Makefile

UNAME_S := $(shell uname -s)
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Isrc/core -Isrc/stdlib -D_GNU_SOURCE
LIBS = -lm -lreadline -lsqlite3
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
BIN_DIR = bin

# VPATH for finding source files in different directories
VPATH =

# Source files
CORE_SOURCES = $(wildcard $(SRC_CORE_DIR)/*.c)
STDLIB_SOURCES = $(wildcard $(SRC_STDLIB_DIR)/*.c)
VM_SOURCES = $(wildcard src/vm/*.c)
PROFILER_SOURCES = $(wildcard src/profiler/*.c)
# Exclude debug_protocol.c to avoid duplicate symbols with conditional_breakpoints.c
DEBUG_SOURCES = $(filter-out src/debug/debug_protocol.c, $(wildcard src/debug/*.c))
SOURCES = $(CORE_SOURCES) $(STDLIB_SOURCES) $(VM_SOURCES) $(PROFILER_SOURCES) $(DEBUG_SOURCES)

OBJECTS = $(patsubst $(SRC_CORE_DIR)/%.c,$(BUILD_DIR)/core/%.o,$(CORE_SOURCES)) \
          $(patsubst $(SRC_STDLIB_DIR)/%.c,$(BUILD_DIR)/stdlib/%.o,$(STDLIB_SOURCES)) \
          $(patsubst src/vm/%.c,$(BUILD_DIR)/vm/%.o,$(VM_SOURCES)) \
          $(patsubst src/profiler/%.c,$(BUILD_DIR)/profiler/%.o,$(PROFILER_SOURCES)) \
          $(patsubst src/debug/%.c,$(BUILD_DIR)/debug/%.o,$(DEBUG_SOURCES))

# Tools
RSTAR = bin/rstar
RADPKG = bin/radpkg
RADS_MASK = bin/rads-mask

# Default target
all: $(TARGET) $(RSTAR) $(RADPKG) $(RADS_MASK)

# Create build and bin directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/core $(BUILD_DIR)/stdlib $(BUILD_DIR)/vm $(BUILD_DIR)/profiler $(BUILD_DIR)/debug $(BIN_DIR)

# Ensure bin directory exists for tools
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile object files from core and stdlib with explicit paths
$(BUILD_DIR)/core/%.o: $(SRC_CORE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/stdlib/%.o: $(SRC_STDLIB_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vm/%.o: src/vm/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/profiler/%.o: src/profiler/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/debug/%.o: src/debug/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $(BIN_DIR)/$(TARGET)
	@ln -sf bin/$(TARGET) $(TARGET)
	@echo "✅ RADS compiler built successfully in bin/$(TARGET)"

# Build RADStar package manager
$(RSTAR): tools/rstar/rstar.c | $(BUILD_DIR)
	$(CC) -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 -O2 -Isrc tools/rstar/rstar.c -o $(RSTAR)
	@ln -sf bin/rstar rstar
	@echo "✅ RADStar package manager built successfully in bin/rstar"

# Build RADPKG (RADS Package Manager)
$(RADPKG): tools/radpkg/radpkg.c | $(BIN_DIR)
	$(CC) -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 -O2 -Isrc tools/radpkg/radpkg.c -o $(RADPKG)
	@ln -sf bin/radpkg radpkg
	@echo "✅ RADPKG built successfully in bin/radpkg"

# Build rads-mask transpiler
$(RADS_MASK): tools/rads-mask/src/*.c tools/rads-mask/src/converter/*.c | $(BIN_DIR)
	$(MAKE) -C tools/rads-mask
	@cp tools/rads-mask/rads-mask $(RADS_MASK)
	@ln -sf bin/rads-mask rads-mask
	@echo "✅ rads-mask built successfully in bin/rads-mask"

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(TARGET) rstar radpkg rads-mask
	$(MAKE) -C tools/rads-mask clean
	@echo "🧹 Cleaned build artifacts"

# Install (copy to /usr/local/bin)
install: $(TARGET) $(RSTAR) $(RADPKG) $(RADS_MASK)
	install -m 755 $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)
	install -m 755 $(RSTAR) /usr/local/bin/rstar
	install -m 755 $(RADPKG) /usr/local/bin/radpkg
	install -m 755 $(RADS_MASK) /usr/local/bin/rads-mask
	@echo "✅ RADS, RStar, RADPKG, and rads-mask installed to /usr/local/bin/"

# Platform targets
linux: all
macos: all
windows: all

# Build all tools
all-tools: $(TARGET) $(RSTAR) $(RADPKG) $(RADS_MASK)
	@echo "✅ All RADS tools built successfully"

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	rm -f /usr/local/bin/rstar
	rm -f /usr/local/bin/radpkg
	rm -f /usr/local/bin/rads-mask
	@echo "🗑️  RADS and all tools uninstalled"

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
	@echo "  all        - Build RADS compiler and all tools (default)"
	@echo "  all-tools  - Build all RADS tools explicitly"
	@echo "  debug      - Build with debug symbols"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  test       - Run basic tests"
	@echo "  help       - Show this help message"

.PHONY: all all-tools debug clean install uninstall test help
