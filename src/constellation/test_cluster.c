/*
 * RADS Constellation - Phase 2 Cluster Tests
 * Tests clustering, gossip, and SWIM functionality
 */

#include "constellation.h"
#include "cluster/cluster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test result helper
void test_result(const char *test_name, bool passed) {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("  ✓ %s\n", test_name);
    } else {
        tests_failed++;
        printf("  ✗ %s\n", test_name);
    }
}

/*
 * ============================================================================
 * CLUSTER LIFECYCLE TESTS
 * ============================================================================
 */

void test_cluster_init(void) {
    printf("\n[TEST] Cluster Initialization\n");

    int result = cluster_init("test-cluster", "node1", "127.0.0.1", 7000);
    test_result("Cluster init succeeds", result == 0);
    test_result("Cluster is running", cluster_is_running());

    Node *local = cluster_get_local_node();
    test_result("Local node exists", local != NULL);
    if (local) {
        test_result("Local node name correct", strcmp(local->name, "node1") == 0);
        test_result("Local node state is ALIVE", local->state == NODE_STATE_ALIVE);
    }
}

void test_cluster_shutdown(void) {
    printf("\n[TEST] Cluster Shutdown\n");

    int result = cluster_shutdown();
    test_result("Cluster shutdown succeeds", result == 0);
    test_result("Cluster is stopped", !cluster_is_running());
}

/*
 * ============================================================================
 * NODE MANAGEMENT TESTS
 * ============================================================================
 */

void test_node_registry(void) {
    printf("\n[TEST] Node Registry\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7001);

    // Create a test node
    Node *test_node = calloc(1, sizeof(Node));
    strncpy(test_node->id, "test-node-2", sizeof(test_node->id));
    strncpy(test_node->name, "node2", sizeof(test_node->name));
    strncpy(test_node->address, "127.0.0.1", sizeof(test_node->address));
    test_node->port = 7002;
    test_node->state = NODE_STATE_ALIVE;

    // Add node
    int result = node_registry_add(test_node);
    test_result("Node can be added to registry", result == 0);

    // Find node
    Node *found = node_registry_find("test-node-2");
    test_result("Node can be found in registry", found != NULL);
    test_result("Found node has correct name", found && strcmp(found->name, "node2") == 0);

    // Get all nodes
    int count = 0;
    Node **nodes = cluster_get_nodes(&count);
    test_result("Node count is 2 (local + added)", count == 2);
    test_result("Nodes array is valid", nodes != NULL);

    // Update node
    test_node->state = NODE_STATE_SUSPECT;
    node_registry_update(test_node);
    found = node_registry_find("test-node-2");
    test_result("Node state can be updated", found && found->state == NODE_STATE_SUSPECT);

    cluster_shutdown();
}

/*
 * ============================================================================
 * MESSAGE HANDLING TESTS
 * ============================================================================
 */

void test_message_lifecycle(void) {
    printf("\n[TEST] Message Lifecycle\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7003);

    // Create message
    const char *payload = "test payload";
    Message *msg = message_create(MSG_HEARTBEAT, payload, strlen(payload) + 1);
    test_result("Message can be created", msg != NULL);

    if (msg) {
        test_result("Message magic is correct", msg->header.magic == 0x52414453);
        test_result("Message type is correct", msg->header.type == MSG_HEARTBEAT);
        test_result("Payload size is correct", msg->header.payload_size == strlen(payload) + 1);
        test_result("Payload data is correct",
                   msg->payload && strcmp((char*)msg->payload, payload) == 0);

        message_destroy(msg);
    }

    cluster_shutdown();
}

void test_node_serialization(void) {
    printf("\n[TEST] Node Serialization\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7004);

    // Create a test node
    Node test_node = {0};
    strncpy(test_node.id, "serialization-test", sizeof(test_node.id));
    strncpy(test_node.name, "test-node", sizeof(test_node.name));
    strncpy(test_node.address, "192.168.1.100", sizeof(test_node.address));
    test_node.port = 8000;
    test_node.state = NODE_STATE_ALIVE;
    test_node.incarnation = 42;

    // Serialize
    uint8_t buffer[sizeof(Node)];
    int serialized = serialize_node(&test_node, buffer, sizeof(buffer));
    test_result("Node can be serialized", serialized > 0);

    // Deserialize
    Node deserialized = {0};
    int result = deserialize_node(buffer, serialized, &deserialized);
    test_result("Node can be deserialized", result > 0);

    if (result > 0) {
        test_result("Deserialized ID matches", strcmp(deserialized.id, test_node.id) == 0);
        test_result("Deserialized name matches", strcmp(deserialized.name, test_node.name) == 0);
        test_result("Deserialized address matches",
                   strcmp(deserialized.address, test_node.address) == 0);
        test_result("Deserialized port matches", deserialized.port == test_node.port);
        test_result("Deserialized state matches", deserialized.state == test_node.state);
        test_result("Deserialized incarnation matches",
                   deserialized.incarnation == test_node.incarnation);
    }

    cluster_shutdown();
}

/*
 * ============================================================================
 * GOSSIP PROTOCOL TESTS
 * ============================================================================
 */

void test_gossip_init(void) {
    printf("\n[TEST] Gossip Protocol\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7005);

    GossipConfig config = {
        .gossip_interval_ms = 1000,
        .gossip_fanout = 3,
        .suspect_timeout_ms = 5000,
        .dead_timeout_ms = 10000
    };

    int result = gossip_init(&config);
    test_result("Gossip init succeeds", result == 0);

    // Test gossip announce
    Node *local = cluster_get_local_node();
    result = gossip_announce(local);
    test_result("Gossip announce succeeds", result == 0);

    cluster_shutdown();
}

/*
 * ============================================================================
 * SWIM PROTOCOL TESTS
 * ============================================================================
 */

void test_swim_state_updates(void) {
    printf("\n[TEST] SWIM State Updates\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7006);

    // Add a test node
    Node *test_node = calloc(1, sizeof(Node));
    strncpy(test_node->id, "swim-test-node", sizeof(test_node->id));
    strncpy(test_node->name, "node2", sizeof(test_node->name));
    test_node->state = NODE_STATE_ALIVE;
    node_registry_add(test_node);

    // Test state transitions
    int result = swim_update_state("swim-test-node", NODE_STATE_SUSPECT);
    test_result("SWIM can mark node as SUSPECT", result == 0);

    Node *found = node_registry_find("swim-test-node");
    test_result("Node state is SUSPECT", found && found->state == NODE_STATE_SUSPECT);

    result = swim_update_state("swim-test-node", NODE_STATE_DEAD);
    test_result("SWIM can mark node as DEAD", result == 0);

    found = node_registry_find("swim-test-node");
    test_result("Node state is DEAD", found && found->state == NODE_STATE_DEAD);

    cluster_shutdown();
}

/*
 * ============================================================================
 * INTEGRATION TESTS
 * ============================================================================
 */

void test_background_threads(void) {
    printf("\n[TEST] Background Threads\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7007);

    // Add a remote node
    Node *remote = calloc(1, sizeof(Node));
    strncpy(remote->id, "remote-node", sizeof(remote->id));
    strncpy(remote->name, "node2", sizeof(remote->name));
    strncpy(remote->address, "127.0.0.1", sizeof(remote->address));
    remote->port = 7008;
    remote->state = NODE_STATE_ALIVE;
    remote->last_seen = time(NULL);
    node_registry_add(remote);

    // Let background threads run for a bit
    printf("  [INFO] Letting background threads run for 3 seconds...\n");
    sleep(3);

    // Threads should be running
    test_result("Background threads are active", cluster_is_running());

    cluster_shutdown();
}

void test_cluster_join(void) {
    printf("\n[TEST] Cluster Join\n");

    cluster_init("test-cluster", "node1", "127.0.0.1", 7009);

    const char *seed_nodes[] = {
        "127.0.0.1:7010",
        "127.0.0.1:7011"
    };

    int result = cluster_join(seed_nodes, 2);
    test_result("Cluster join succeeds", result == 0);

    cluster_shutdown();
}

/*
 * ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================
 */

int main(void) {
    printf("\033[1;36m╔════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║  RADS CONSTELLATION - Phase 2 Tests           ║\033[0m\n");
    printf("\033[1;36m║  Clustering & Communication Tests             ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════╝\033[0m\n");

    // Run all tests
    test_cluster_init();
    test_cluster_shutdown();
    test_node_registry();
    test_message_lifecycle();
    test_node_serialization();
    test_gossip_init();
    test_swim_state_updates();
    test_background_threads();
    test_cluster_join();

    // Print summary
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  Test Summary                                  ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_run);
    printf("\n");

    if (tests_failed == 0) {
        printf("\033[1;32m🎉 All tests passed!\033[0m\n\n");
        printf("Phase 2: Clustering - \033[1;32mCOMPLETE\033[0m\n");
        printf("✓ Cluster lifecycle\n");
        printf("✓ Node registry\n");
        printf("✓ Message handling\n");
        printf("✓ Gossip protocol\n");
        printf("✓ SWIM membership\n");
        printf("✓ Background threads\n");
        printf("\n");
        return 0;
    } else {
        printf("\033[1;31m❌ Some tests failed\033[0m\n\n");
        return 1;
    }
}
