# RADS Constellation - Build System
# Phase 1: Foundation
# Phase 2: Clustering

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11
LDFLAGS = -lpthread

# Source files
CORE_SRC = core/controller.c
ISOLATION_SRC = isolation/namespaces.c
RESOURCES_SRC = resources/cgroups.c
CLUSTER_SRC = cluster/cluster.c

ALL_SRC = $(CORE_SRC) $(ISOLATION_SRC) $(RESOURCES_SRC) $(CLUSTER_SRC)
ALL_OBJ = $(ALL_SRC:.c=.o)

# Output library
LIB_NAME = libconstellation.a

# Test programs
TEST_PROG = constellation_test
CLUSTER_TEST = cluster_test

.PHONY: all clean test test-cluster

all: $(LIB_NAME) $(TEST_PROG) $(CLUSTER_TEST)

# Build static library
$(LIB_NAME): $(ALL_OBJ)
	@echo "📦 Creating Constellation library..."
	ar rcs $@ $^
	@echo "✅ Library created: $(LIB_NAME)"
	@echo ""

# Build test program
$(TEST_PROG): test_constellation.c $(LIB_NAME)
	@echo "🔨 Building test program..."
	$(CC) $(CFLAGS) -o $@ $< -L. -lconstellation $(LDFLAGS)
	@echo "✅ Test program built: $(TEST_PROG)"
	@echo ""

# Build cluster test program
$(CLUSTER_TEST): test_cluster.c $(LIB_NAME)
	@echo "🔨 Building cluster test program..."
	$(CC) $(CFLAGS) -o $@ $< -L. -lconstellation $(LDFLAGS)
	@echo "✅ Cluster test program built: $(CLUSTER_TEST)"
	@echo ""

# Compile source files
%.o: %.c constellation.h
	@echo "🔧 Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(ALL_OBJ) $(LIB_NAME) $(TEST_PROG) $(CLUSTER_TEST)
	@echo "✅ Clean complete"
	@echo ""

test: $(TEST_PROG)
	@echo "🧪 Running Constellation Phase 1 tests..."
	@echo ""
	./$(TEST_PROG)

test-cluster: $(CLUSTER_TEST)
	@echo "🧪 Running Constellation Phase 2 cluster tests..."
	@echo ""
	./$(CLUSTER_TEST)

help:
	@echo "RADS Constellation - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make               - Build library and all test programs"
	@echo "  make test          - Build and run Phase 1 tests"
	@echo "  make test-cluster  - Build and run Phase 2 cluster tests"
	@echo "  make clean         - Remove build artifacts"
	@echo "  make help          - Show this help"
	@echo ""
	@echo "Output:"
	@echo "  $(LIB_NAME)    - Constellation static library"
	@echo "  $(TEST_PROG)       - Phase 1 test program"
	@echo "  $(CLUSTER_TEST)         - Phase 2 cluster test program"
