# RADS Constellation - Proof of Concept Makefile

CC = gcc
CFLAGS = -Wall -O2
TARGET = simple_spawner

.PHONY: all clean test

all: $(TARGET)

$(TARGET): simple_spawner.c
	@echo "🔨 Compiling Constellation Proof of Concept..."
	$(CC) $(CFLAGS) -o $(TARGET) simple_spawner.c
	@echo "✅ Build complete: ./$(TARGET)"
	@echo ""

clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(TARGET)
	@echo "✅ Clean complete"

test: $(TARGET)
	@echo "🧪 Running proof-of-concept test..."
	@echo ""
	./$(TARGET) quick_test.rads 5

demo: $(TARGET)
	@echo "🚀 Running full demonstration..."
	@echo ""
	./$(TARGET) quick_test.rads 10

help:
	@echo "RADS Constellation - Proof of Concept"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the spawner"
	@echo "  make test     - Quick 5-second test"
	@echo "  make demo     - Full 10-second demo"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./simple_spawner <script.rads> [monitor_seconds]"
