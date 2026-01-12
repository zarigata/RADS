# RADS Constellation - Build System
# Phase 1: Foundation
# Phase 2: Clustering
# Phase 3: Resource Orchestration
# Phase 4: Distributed Filesystem
# Phase 5: Service Mesh

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=gnu11
LDFLAGS = -lpthread -lssl -lcrypto

# Source files
CORE_SRC = core/controller.c
ISOLATION_SRC = isolation/namespaces.c
RESOURCES_SRC = resources/cgroups.c
CLUSTER_SRC = cluster/cluster.c
SCHEDULER_SRC = scheduler/scheduler.c
CONSENSUS_SRC = consensus/raft.c
DFS_SRC = dfs/dht.c dfs/dfs.c dfs/locks.c
MESH_SRC = mesh/registry.c mesh/loadbalancer.c mesh/circuit.c

ALL_SRC = $(CORE_SRC) $(ISOLATION_SRC) $(RESOURCES_SRC) $(CLUSTER_SRC) $(SCHEDULER_SRC) $(CONSENSUS_SRC) $(DFS_SRC) $(MESH_SRC)
ALL_OBJ = $(ALL_SRC:.c=.o)

# Output library
LIB_NAME = libconstellation.a

# Test programs
TEST_PROG = constellation_test
CLUSTER_TEST = cluster_test
SCHEDULER_TEST = scheduler_test
DFS_TEST = dfs_test
MESH_TEST = mesh_test

.PHONY: all clean test test-cluster test-scheduler test-dfs test-mesh test-all

all: $(LIB_NAME) $(TEST_PROG) $(CLUSTER_TEST) $(SCHEDULER_TEST) $(DFS_TEST) $(MESH_TEST)

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

# Build scheduler test program
$(SCHEDULER_TEST): test_scheduler.c $(LIB_NAME)
	@echo "🔨 Building scheduler test program..."
	$(CC) $(CFLAGS) -o $@ $< -L. -lconstellation $(LDFLAGS)
	@echo "✅ Scheduler test program built: $(SCHEDULER_TEST)"
	@echo ""

# Build DFS test program
$(DFS_TEST): test_dfs.c $(LIB_NAME)
	@echo "🔨 Building DFS test program..."
	$(CC) $(CFLAGS) -o $@ $< -L. -lconstellation $(LDFLAGS)
	@echo "✅ DFS test program built: $(DFS_TEST)"
	@echo ""

# Build Mesh test program
$(MESH_TEST): test_mesh.c $(LIB_NAME)
	@echo "🔨 Building Mesh test program..."
	$(CC) $(CFLAGS) -o $@ $< -L. -lconstellation $(LDFLAGS)
	@echo "✅ Mesh test program built: $(MESH_TEST)"
	@echo ""

# Compile source files
%.o: %.c constellation.h
	@echo "🔧 Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(ALL_OBJ) $(LIB_NAME) $(TEST_PROG) $(CLUSTER_TEST) $(SCHEDULER_TEST) $(DFS_TEST) $(MESH_TEST)
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

test-scheduler: $(SCHEDULER_TEST)
	@echo "🧪 Running Constellation Phase 3 scheduler tests..."
	@echo ""
	./$(SCHEDULER_TEST)

test-dfs: $(DFS_TEST)
	@echo "🧪 Running Constellation Phase 4 DFS tests..."
	@echo ""
	./$(DFS_TEST)

test-mesh: $(MESH_TEST)
	@echo "🧪 Running Constellation Phase 5 Mesh tests..."
	@echo ""
	./$(MESH_TEST)

test-all: test test-cluster test-scheduler test-dfs test-mesh
	@echo "✅ All tests completed!"

help:
	@echo "RADS Constellation - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make                 - Build library and all test programs"
	@echo "  make test            - Build and run Phase 1 tests"
	@echo "  make test-cluster    - Build and run Phase 2 cluster tests"
	@echo "  make test-scheduler  - Build and run Phase 3 scheduler tests"
	@echo "  make test-dfs        - Build and run Phase 4 DFS tests"
	@echo "  make test-mesh       - Build and run Phase 5 Mesh tests"
	@echo "  make test-all        - Run all tests (Phase 1, 2, 3, 4, 5)"
	@echo "  make clean           - Remove build artifacts"
	@echo "  make help            - Show this help"
	@echo ""
	@echo "Output:"
	@echo "  $(LIB_NAME)    - Constellation static library"
	@echo "  $(TEST_PROG)       - Phase 1 test program"
	@echo "  $(CLUSTER_TEST)         - Phase 2 cluster test program"
	@echo "  $(SCHEDULER_TEST)       - Phase 3 scheduler test program"
	@echo "  $(DFS_TEST)              - Phase 4 DFS test program"
	@echo "  $(MESH_TEST)             - Phase 5 Mesh test program"
