/*
 * RADS Constellation - Cluster Management
 * Phase 2: Core clustering implementation
 */

#include "cluster.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

// Global cluster state
static ClusterState g_cluster = {0};
static pthread_t g_gossip_thread = 0;
static pthread_t g_health_thread = 0;
static int g_socket_fd = -1;

/*
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

// Generate node ID from name and address
static void generate_node_id(const char *name, const char *address,
                              char *id_out, size_t size) {
    snprintf(id_out, size, "%s@%s", name, address);
}

// Find node index in array
static int find_node_index(const char *node_id) {
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i]->id, node_id) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * ============================================================================
 * CLUSTER LIFECYCLE
 * ============================================================================
 */

int cluster_init(const char *cluster_name, const char *node_name,
                 const char *bind_address, uint16_t port) {
    if (g_cluster.is_running) {
        fprintf(stderr, "\033[1;31m[CLUSTER]\033[0m Already initialized\n");
        return -1;
    }

    printf("\033[1;36m╔════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║  🌐 RADS CONSTELLATION - Clustering           ║\033[0m\n");
    printf("\033[1;36m║  Phase 2: Multi-Machine Communication          ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════╝\033[0m\n\n");

    // Initialize cluster state
    strncpy(g_cluster.cluster_name, cluster_name, sizeof(g_cluster.cluster_name) - 1);

    // Create local node
    Node *local = calloc(1, sizeof(Node));
    if (!local) {
        return -1;
    }

    strncpy(local->name, node_name, sizeof(local->name) - 1);
    strncpy(local->address, bind_address, sizeof(local->address) - 1);
    local->port = port;
    local->state = NODE_STATE_ALIVE;
    local->last_seen = time(NULL);
    local->incarnation = 0;
    local->failed_pings = 0;

    generate_node_id(node_name, bind_address, local->id, sizeof(local->id));

    g_cluster.local_node = local;

    // Allocate node array
    g_cluster.node_capacity = 100; // Support up to 100 nodes
    g_cluster.nodes = calloc(g_cluster.node_capacity, sizeof(Node*));
    if (!g_cluster.nodes) {
        free(local);
        return -1;
    }

    // Add local node to registry
    g_cluster.nodes[0] = local;
    g_cluster.node_count = 1;

    // Initialize leader state (start as leader until we hear from others)
    strncpy(g_cluster.leader_id, local->id, sizeof(g_cluster.leader_id));
    g_cluster.is_leader = true;
    g_cluster.term = 1;

    // Create UDP socket for cluster communication
    g_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_socket_fd < 0) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to create socket\n");
        free(g_cluster.nodes);
        free(local);
        return -1;
    }

    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(bind_address);
    addr.sin_port = htons(port);

    if (bind(g_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to bind socket: %s\n",
                strerror(errno));
        close(g_socket_fd);
        free(g_cluster.nodes);
        free(local);
        return -1;
    }

    g_cluster.is_running = true;

    // Initialize random seed for gossip target selection
    srand(time(NULL));

    // Start background threads
    if (pthread_create(&g_gossip_thread, NULL, gossip_thread_main, NULL) != 0) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to start gossip thread\n");
        // Non-fatal, continue
    }

    if (pthread_create(&g_health_thread, NULL, health_check_thread_main, NULL) != 0) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to start health thread\n");
        // Non-fatal, continue
    }

    printf("\033[1;32m[CLUSTER]\033[0m Cluster initialized\n");
    printf("  Cluster: %s\n", g_cluster.cluster_name);
    printf("  Node: %s\n", local->name);
    printf("  ID: %s\n", local->id);
    printf("  Address: %s:%u\n", local->address, local->port);
    printf("  State: %s\n", "ALIVE");
    printf("  Leader: %s (term %lu)\n\n",
           g_cluster.is_leader ? "YES" : "NO", g_cluster.term);

    return 0;
}

int cluster_shutdown(void) {
    if (!g_cluster.is_running) {
        return -1;
    }

    printf("\033[1;33m[CLUSTER]\033[0m Shutting down cluster...\n");

    // Stop background threads
    if (g_gossip_thread) {
        pthread_cancel(g_gossip_thread);
        pthread_join(g_gossip_thread, NULL);
        g_gossip_thread = 0;
    }

    if (g_health_thread) {
        pthread_cancel(g_health_thread);
        pthread_join(g_health_thread, NULL);
        g_health_thread = 0;
    }

    // Close socket
    if (g_socket_fd >= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
    }

    // Free nodes
    for (int i = 0; i < g_cluster.node_count; i++) {
        free(g_cluster.nodes[i]);
    }
    free(g_cluster.nodes);

    memset(&g_cluster, 0, sizeof(g_cluster));

    printf("\033[1;32m[CLUSTER]\033[0m Cluster stopped\n\n");
    return 0;
}

bool cluster_is_running(void) {
    return g_cluster.is_running;
}

/*
 * ============================================================================
 * NODE MANAGEMENT
 * ============================================================================
 */

int cluster_join(const char **seed_nodes, int seed_count) {
    if (!g_cluster.is_running) {
        return -1;
    }

    printf("\033[1;34m[JOIN]\033[0m Joining cluster with %d seed nodes...\n",
           seed_count);

    // TODO: Send GOSSIP_ANNOUNCE to seed nodes
    // For now, just log the attempt
    for (int i = 0; i < seed_count; i++) {
        printf("  Seed: %s\n", seed_nodes[i]);
    }

    printf("\033[1;32m[JOIN]\033[0m Join initiated\n");
    return 0;
}

int cluster_leave(void) {
    if (!g_cluster.is_running) {
        return -1;
    }

    printf("\033[1;33m[LEAVE]\033[0m Leaving cluster gracefully...\n");

    // Mark local node as LEFT
    g_cluster.local_node->state = NODE_STATE_LEFT;

    // TODO: Send GOSSIP_LEAVE to all nodes

    printf("\033[1;32m[LEAVE]\033[0m Left cluster\n");
    return 0;
}

Node *cluster_get_local_node(void) {
    return g_cluster.local_node;
}

Node **cluster_get_nodes(int *out_count) {
    if (out_count) {
        *out_count = g_cluster.node_count;
    }
    return g_cluster.nodes;
}

Node *cluster_find_node(const char *node_id) {
    int index = find_node_index(node_id);
    if (index < 0) {
        return NULL;
    }
    return g_cluster.nodes[index];
}

/*
 * ============================================================================
 * NODE REGISTRY
 * ============================================================================
 */

int node_registry_add(Node *node) {
    if (!node || g_cluster.node_count >= g_cluster.node_capacity) {
        return -1;
    }

    // Check if node already exists
    if (find_node_index(node->id) >= 0) {
        return -1; // Already exists
    }

    g_cluster.nodes[g_cluster.node_count++] = node;

    printf("\033[1;32m[REGISTRY]\033[0m Node added: %s (%s:%u)\n",
           node->name, node->address, node->port);

    return 0;
}

int node_registry_remove(const char *node_id) {
    int index = find_node_index(node_id);
    if (index < 0) {
        return -1;
    }

    Node *node = g_cluster.nodes[index];

    // Shift remaining nodes
    for (int i = index; i < g_cluster.node_count - 1; i++) {
        g_cluster.nodes[i] = g_cluster.nodes[i + 1];
    }
    g_cluster.node_count--;

    printf("\033[1;33m[REGISTRY]\033[0m Node removed: %s\n", node->name);

    free(node);
    return 0;
}

Node *node_registry_find(const char *node_id) {
    return cluster_find_node(node_id);
}

void node_registry_update(Node *node) {
    Node *existing = cluster_find_node(node->id);
    if (existing) {
        // Update fields (but don't change pointer)
        existing->state = node->state;
        existing->last_seen = node->last_seen;
        existing->incarnation = node->incarnation;
        existing->failed_pings = node->failed_pings;
        existing->current_instances = node->current_instances;
        existing->cpu_available = node->cpu_available;
        existing->ram_available_mb = node->ram_available_mb;
    }
}

/*
 * ============================================================================
 * GOSSIP PROTOCOL (Simple Implementation)
 * ============================================================================
 */

int gossip_init(GossipConfig *config) {
    // Store config (could be expanded)
    printf("\033[1;36m[GOSSIP]\033[0m Gossip protocol initialized\n");
    if (config) {
        printf("  Interval: %dms\n", config->gossip_interval_ms);
        printf("  Fanout: %d\n", config->gossip_fanout);
    }
    return 0;
}

int gossip_announce(Node *node) {
    printf("\033[1;36m[GOSSIP]\033[0m Announcing node: %s\n", node->name);

    // Serialize node data
    uint8_t buffer[sizeof(Node)];
    int serialized = serialize_node(node, buffer, sizeof(buffer));
    if (serialized < 0) {
        return -1;
    }

    // Create announce message
    Message *msg = message_create(MSG_GOSSIP_ANNOUNCE, buffer, serialized);
    if (!msg) {
        return -1;
    }

    // Broadcast to all known nodes
    for (int i = 1; i < g_cluster.node_count; i++) {
        Node *target = g_cluster.nodes[i];
        if (target->state != NODE_STATE_DEAD && target->state != NODE_STATE_LEFT) {
            message_send(msg, target->address, target->port);
        }
    }

    message_destroy(msg);
    return 0;
}

int gossip_suspect(const char *node_id) {
    Node *node = cluster_find_node(node_id);
    if (!node) {
        return -1;
    }

    printf("\033[1;33m[GOSSIP]\033[0m Marking node as SUSPECT: %s\n", node_id);
    node->state = NODE_STATE_SUSPECT;

    // Serialize suspect node data
    uint8_t buffer[sizeof(Node)];
    int serialized = serialize_node(node, buffer, sizeof(buffer));
    if (serialized < 0) {
        return -1;
    }

    // Broadcast SUSPECT message
    Message *msg = message_create(MSG_GOSSIP_SUSPECT, buffer, serialized);
    if (!msg) {
        return -1;
    }

    for (int i = 1; i < g_cluster.node_count; i++) {
        Node *target = g_cluster.nodes[i];
        if (target->state != NODE_STATE_DEAD && target->state != NODE_STATE_LEFT) {
            message_send(msg, target->address, target->port);
        }
    }

    message_destroy(msg);
    return 0;
}

int gossip_confirm_dead(const char *node_id) {
    Node *node = cluster_find_node(node_id);
    if (!node) {
        return -1;
    }

    printf("\033[1;31m[GOSSIP]\033[0m Confirming node DEAD: %s\n", node_id);
    node->state = NODE_STATE_DEAD;

    // Serialize dead node data
    uint8_t buffer[sizeof(Node)];
    int serialized = serialize_node(node, buffer, sizeof(buffer));
    if (serialized < 0) {
        return -1;
    }

    // Broadcast CONFIRM message
    Message *msg = message_create(MSG_GOSSIP_CONFIRM, buffer, serialized);
    if (!msg) {
        return -1;
    }

    for (int i = 1; i < g_cluster.node_count; i++) {
        Node *target = g_cluster.nodes[i];
        if (target->state != NODE_STATE_DEAD && target->state != NODE_STATE_LEFT) {
            message_send(msg, target->address, target->port);
        }
    }

    message_destroy(msg);
    return 0;
}

int gossip_process_message(Message *msg) {
    if (!msg) {
        return -1;
    }

    printf("\033[1;36m[GOSSIP]\033[0m Processing message type: %d from %s\n",
           msg->header.type, msg->header.sender_id);

    switch (msg->header.type) {
        case MSG_PING: {
            // Respond with PONG
            Message *pong = message_create(MSG_PONG, NULL, 0);
            if (pong) {
                // Extract sender address from message (simplified)
                Node *sender = cluster_find_node(msg->header.sender_id);
                if (sender) {
                    message_send(pong, sender->address, sender->port);
                }
                message_destroy(pong);
            }
            break;
        }

        case MSG_PONG:
            // Update sender's last_seen
            {
                Node *sender = cluster_find_node(msg->header.sender_id);
                if (sender) {
                    sender->last_seen = time(NULL);
                    sender->state = NODE_STATE_ALIVE;
                    sender->failed_pings = 0;
                }
            }
            break;

        case MSG_GOSSIP_ANNOUNCE: {
            // New node announced
            if (msg->payload && msg->header.payload_size > 0) {
                Node announced_node;
                if (deserialize_node((uint8_t*)msg->payload, msg->header.payload_size,
                                    &announced_node) >= 0) {
                    // Check if we already know this node
                    Node *existing = cluster_find_node(announced_node.id);
                    if (!existing) {
                        // Add new node
                        Node *new_node = calloc(1, sizeof(Node));
                        if (new_node) {
                            memcpy(new_node, &announced_node, sizeof(Node));
                            node_registry_add(new_node);
                        }
                    } else {
                        // Update existing node
                        node_registry_update(&announced_node);
                    }
                }
            }
            break;
        }

        case MSG_GOSSIP_SUSPECT: {
            // Node suspected dead
            if (msg->payload && msg->header.payload_size > 0) {
                Node suspect_node;
                if (deserialize_node((uint8_t*)msg->payload, msg->header.payload_size,
                                    &suspect_node) >= 0) {
                    Node *existing = cluster_find_node(suspect_node.id);
                    if (existing && existing->state == NODE_STATE_ALIVE) {
                        existing->state = NODE_STATE_SUSPECT;
                        printf("\033[1;33m[GOSSIP]\033[0m Node %s marked SUSPECT by gossip\n",
                               existing->name);
                    }
                }
            }
            break;
        }

        case MSG_GOSSIP_CONFIRM: {
            // Node confirmed dead
            if (msg->payload && msg->header.payload_size > 0) {
                Node dead_node;
                if (deserialize_node((uint8_t*)msg->payload, msg->header.payload_size,
                                    &dead_node) >= 0) {
                    Node *existing = cluster_find_node(dead_node.id);
                    if (existing) {
                        existing->state = NODE_STATE_DEAD;
                        printf("\033[1;31m[GOSSIP]\033[0m Node %s confirmed DEAD by gossip\n",
                               existing->name);
                    }
                }
            }
            break;
        }

        case MSG_GOSSIP_LEAVE: {
            // Node left gracefully
            if (msg->payload && msg->header.payload_size > 0) {
                Node left_node;
                if (deserialize_node((uint8_t*)msg->payload, msg->header.payload_size,
                                    &left_node) >= 0) {
                    Node *existing = cluster_find_node(left_node.id);
                    if (existing) {
                        existing->state = NODE_STATE_LEFT;
                        printf("\033[1;33m[GOSSIP]\033[0m Node %s LEFT cluster\n",
                               existing->name);
                    }
                }
            }
            break;
        }

        case MSG_HEARTBEAT:
            // Update sender's last_seen
            {
                Node *sender = cluster_find_node(msg->header.sender_id);
                if (sender) {
                    sender->last_seen = time(NULL);
                }
            }
            break;

        case MSG_SYNC_REQUEST:
        case MSG_SYNC_RESPONSE:
            // TODO: Implement full state sync
            break;
    }

    return 0;
}

/*
 * ============================================================================
 * SWIM PROTOCOL (Failure Detection)
 * ============================================================================
 */

int swim_ping(Node *node) {
    if (!node) {
        return -1;
    }

    printf("\033[1;34m[SWIM]\033[0m Pinging node: %s\n", node->name);

    // Send PING message
    Message *msg = message_create(MSG_PING, NULL, 0);
    if (!msg) {
        return -1;
    }

    if (message_send(msg, node->address, node->port) < 0) {
        message_destroy(msg);
        node->failed_pings++;
        return -1;
    }

    message_destroy(msg);

    // Wait for PONG (with timeout)
    Message *response = message_receive(g_socket_fd, 1000); // 1 second timeout
    if (response && response->header.type == MSG_PONG) {
        // Pong received, node is alive
        node->last_seen = time(NULL);
        node->failed_pings = 0;
        message_destroy(response);
        return 0;
    }

    // No response
    if (response) {
        message_destroy(response);
    }
    node->failed_pings++;
    return -1;
}

int swim_indirect_ping(Node *node, Node *proxy) {
    if (!node || !proxy) {
        return -1;
    }

    printf("\033[1;34m[SWIM]\033[0m Indirect ping to %s via %s\n",
           node->name, proxy->name);

    // For now, just send direct ping
    // In full implementation, would send PING_REQ to proxy with target info
    Message *msg = message_create(MSG_PING, NULL, 0);
    if (!msg) {
        return -1;
    }

    message_send(msg, node->address, node->port);
    message_destroy(msg);

    return 0;
}

int swim_update_state(const char *node_id, NodeState new_state) {
    Node *node = cluster_find_node(node_id);
    if (!node) {
        return -1;
    }

    NodeState old_state = node->state;
    node->state = new_state;

    const char *state_names[] = {"ALIVE", "SUSPECT", "DEAD", "LEFT"};
    printf("\033[1;36m[SWIM]\033[0m Node %s: %s -> %s\n",
           node->name,
           state_names[old_state],
           state_names[new_state]);

    return 0;
}

/*
 * ============================================================================
 * MESSAGE HANDLING (Simple Implementation)
 * ============================================================================
 */

Message *message_create(MessageType type, const void *payload, size_t payload_size) {
    Message *msg = calloc(1, sizeof(Message));
    if (!msg) {
        return NULL;
    }

    msg->header.magic = 0x52414453; // "RADS"
    msg->header.version = 1;
    msg->header.type = type;
    msg->header.payload_size = payload_size;
    strncpy(msg->header.sender_id, g_cluster.local_node->id,
            sizeof(msg->header.sender_id) - 1);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    msg->header.timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if (payload && payload_size > 0) {
        msg->payload = malloc(payload_size);
        if (msg->payload) {
            memcpy(msg->payload, payload, payload_size);
        }
    }

    return msg;
}

void message_destroy(Message *msg) {
    if (msg) {
        if (msg->payload) {
            free(msg->payload);
        }
        free(msg);
    }
}

int message_send(Message *msg, const char *address, uint16_t port) {
    if (!msg || g_socket_fd < 0) {
        return -1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(address);
    dest_addr.sin_port = htons(port);

    // Send header
    ssize_t sent = sendto(g_socket_fd, &msg->header, sizeof(msg->header), 0,
                          (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        return -1;
    }

    // Send payload if present
    if (msg->payload && msg->header.payload_size > 0) {
        sent = sendto(g_socket_fd, msg->payload, msg->header.payload_size, 0,
                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (sent < 0) {
            return -1;
        }
    }

    return 0;
}

Message *message_receive(int socket_fd, int timeout_ms) {
    if (socket_fd < 0) {
        return NULL;
    }

    // Set up timeout using select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int ready = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ready <= 0) {
        return NULL; // Timeout or error
    }

    // Allocate message
    Message *msg = calloc(1, sizeof(Message));
    if (!msg) {
        return NULL;
    }

    // Receive header
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    ssize_t received = recvfrom(socket_fd, &msg->header, sizeof(msg->header), 0,
                                (struct sockaddr*)&sender_addr, &addr_len);
    if (received < (ssize_t)sizeof(msg->header)) {
        free(msg);
        return NULL;
    }

    // Verify magic number
    if (msg->header.magic != 0x52414453) { // "RADS"
        free(msg);
        return NULL;
    }

    // Receive payload if present
    if (msg->header.payload_size > 0) {
        msg->payload = malloc(msg->header.payload_size);
        if (!msg->payload) {
            free(msg);
            return NULL;
        }

        received = recvfrom(socket_fd, msg->payload, msg->header.payload_size, 0,
                           (struct sockaddr*)&sender_addr, &addr_len);
        if (received < (ssize_t)msg->header.payload_size) {
            free(msg->payload);
            free(msg);
            return NULL;
        }
    }

    return msg;
}

/*
 * ============================================================================
 * NODE SERIALIZATION
 * ============================================================================
 */

int serialize_node(Node *node, uint8_t *buffer, size_t buffer_size) {
    if (!node || !buffer) {
        return -1;
    }

    // Calculate required size
    size_t required_size = sizeof(Node);
    if (buffer_size < required_size) {
        return -1; // Buffer too small
    }

    // Simple binary serialization (memcpy approach)
    // In production, use MessagePack or Protocol Buffers
    memcpy(buffer, node, sizeof(Node));

    return (int)sizeof(Node);
}

int deserialize_node(const uint8_t *buffer, size_t buffer_size, Node *out_node) {
    if (!buffer || !out_node) {
        return -1;
    }

    if (buffer_size < sizeof(Node)) {
        return -1; // Buffer too small
    }

    // Simple binary deserialization
    memcpy(out_node, buffer, sizeof(Node));

    return (int)sizeof(Node);
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *gossip_thread_main(void *arg) {
    (void)arg;

    printf("\033[1;36m[GOSSIP]\\033[0m Background thread started\n");

    while (g_cluster.is_running) {
        // Sleep for gossip interval (default: 1 second)
        sleep(1);

        if (!g_cluster.is_running) {
            break;
        }

        // Select random nodes to gossip with (fanout = 3)
        int fanout = 3;
        if (g_cluster.node_count <= 1) {
            continue; // Only local node, nothing to gossip
        }

        for (int i = 0; i < fanout && i < g_cluster.node_count - 1; i++) {
            // Pick random node (excluding self)
            int idx = (rand() % (g_cluster.node_count - 1)) + 1;
            Node *target = g_cluster.nodes[idx];

            if (target->state == NODE_STATE_DEAD || target->state == NODE_STATE_LEFT) {
                continue;
            }

            // Send heartbeat to target node
            Message *msg = message_create(MSG_HEARTBEAT, NULL, 0);
            if (msg) {
                message_send(msg, target->address, target->port);
                message_destroy(msg);
            }
        }
    }

    printf("\033[1;36m[GOSSIP]\033[0m Background thread stopped\n");
    return NULL;
}

void *health_check_thread_main(void *arg) {
    (void)arg;

    printf("\033[1;34m[HEALTH]\033[0m Background thread started\n");

    while (g_cluster.is_running) {
        // Sleep for health check interval (default: 2 seconds)
        sleep(2);

        if (!g_cluster.is_running) {
            break;
        }

        time_t now = time(NULL);

        // Check all nodes for health
        for (int i = 1; i < g_cluster.node_count; i++) { // Skip local node (index 0)
            Node *node = g_cluster.nodes[i];

            // Calculate time since last seen
            time_t elapsed = now - node->last_seen;

            switch (node->state) {
                case NODE_STATE_ALIVE:
                    // If not seen in 5 seconds, mark as suspect
                    if (elapsed > 5) {
                        printf("\033[1;33m[HEALTH]\033[0m Node %s not responding, marking SUSPECT\n",
                               node->name);
                        node->state = NODE_STATE_SUSPECT;
                        node->failed_pings++;
                        gossip_suspect(node->id);
                    } else {
                        // Send ping
                        swim_ping(node);
                    }
                    break;

                case NODE_STATE_SUSPECT:
                    // If suspect for 10 seconds, mark as dead
                    if (elapsed > 10) {
                        printf("\033[1;31m[HEALTH]\033[0m Node %s confirmed DEAD\n",
                               node->name);
                        node->state = NODE_STATE_DEAD;
                        gossip_confirm_dead(node->id);
                    } else {
                        // Try indirect ping through other nodes
                        if (g_cluster.node_count > 2) {
                            int proxy_idx = (i + 1) % g_cluster.node_count;
                            if (proxy_idx == 0) proxy_idx = 1; // Skip local node
                            Node *proxy = g_cluster.nodes[proxy_idx];
                            swim_indirect_ping(node, proxy);
                        }
                    }
                    break;

                case NODE_STATE_DEAD:
                case NODE_STATE_LEFT:
                    // Don't check dead or left nodes
                    break;
            }
        }
    }

    printf("\033[1;34m[HEALTH]\033[0m Background thread stopped\n");
    return NULL;
}
