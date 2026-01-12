/*
 * RADS Constellation - Clustering Header
 * Phase 2: Multi-Machine Communication
 *
 * Defines types and APIs for:
 * - Node management
 * - Gossip protocol
 * - SWIM membership
 * - Cluster state
 */

#ifndef CONSTELLATION_CLUSTER_H
#define CONSTELLATION_CLUSTER_H

#include "../constellation.h"
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * ============================================================================
 * NODE TYPES
 * ============================================================================
 */

// Node states (SWIM membership)
typedef enum {
    NODE_STATE_ALIVE,        // Node is healthy
    NODE_STATE_SUSPECT,      // Node might be dead (missed heartbeats)
    NODE_STATE_DEAD,         // Node is confirmed dead
    NODE_STATE_LEFT          // Node gracefully left cluster
} NodeState;

// Node metadata
typedef struct {
    char id[64];             // Unique node ID
    char name[256];          // Node name
    char address[64];        // IP address
    uint16_t port;           // Port number
    NodeState state;         // Current state
    time_t last_seen;        // Last heartbeat timestamp
    uint64_t incarnation;    // Incarnation number (for refuting suspicion)
    int failed_pings;        // Consecutive failed pings

    // Node capabilities
    uint32_t max_instances;  // Maximum instances this node can handle
    uint32_t current_instances; // Current instance count

    // Resource info
    double cpu_total;        // Total CPU cores
    double cpu_available;    // Available CPU
    uint64_t ram_total_mb;   // Total RAM
    uint64_t ram_available_mb; // Available RAM
} Node;

// Cluster state
typedef struct {
    char cluster_name[256];  // Cluster name
    Node *local_node;        // This node
    Node **nodes;            // All known nodes
    int node_count;          // Number of nodes
    int node_capacity;       // Array capacity
    bool is_running;         // Cluster running flag

    // Leader election (Raft - simplified for now)
    char leader_id[64];      // Current leader node ID
    bool is_leader;          // Is this node the leader?
    uint64_t term;           // Current election term
} ClusterState;

/*
 * ============================================================================
 * MESSAGE TYPES (RNP Protocol)
 * ============================================================================
 */

// Message types
typedef enum {
    MSG_PING,                // Ping request
    MSG_PONG,                // Ping response
    MSG_GOSSIP_ANNOUNCE,     // Node announces presence
    MSG_GOSSIP_SUSPECT,      // Report suspected dead node
    MSG_GOSSIP_CONFIRM,      // Confirm node is dead
    MSG_GOSSIP_LEAVE,        // Node leaving cluster
    MSG_SYNC_REQUEST,        // Request full state sync
    MSG_SYNC_RESPONSE,       // Full state sync response
    MSG_HEARTBEAT            // Regular heartbeat
} MessageType;

// Message header
typedef struct {
    uint32_t magic;          // Protocol magic number (0x52414453 = "RADS")
    uint16_t version;        // Protocol version
    MessageType type;        // Message type
    uint32_t payload_size;   // Payload size in bytes
    char sender_id[64];      // Sender node ID
    uint64_t timestamp;      // Message timestamp (ms)
} MessageHeader;

// Generic message
typedef struct {
    MessageHeader header;
    void *payload;           // Payload data
} Message;

/*
 * ============================================================================
 * GOSSIP PROTOCOL
 * ============================================================================
 */

// Gossip configuration
typedef struct {
    int gossip_interval_ms;  // How often to gossip (default: 1000ms)
    int gossip_fanout;       // How many random nodes to gossip to (default: 3)
    int suspect_timeout_ms;  // Time before marking suspect (default: 5000ms)
    int dead_timeout_ms;     // Time before marking dead (default: 10000ms)
} GossipConfig;

/*
 * ============================================================================
 * CLUSTER API
 * ============================================================================
 */

// Cluster lifecycle
int cluster_init(const char *cluster_name, const char *node_name,
                 const char *bind_address, uint16_t port);
int cluster_shutdown(void);
bool cluster_is_running(void);

// Node management
int cluster_join(const char **seed_nodes, int seed_count);
int cluster_leave(void);
Node *cluster_get_local_node(void);
Node **cluster_get_nodes(int *out_count);
Node *cluster_find_node(const char *node_id);

// Gossip protocol
int gossip_init(GossipConfig *config);
int gossip_announce(Node *node);
int gossip_suspect(const char *node_id);
int gossip_confirm_dead(const char *node_id);
int gossip_process_message(Message *msg);

// Health checking (SWIM)
int swim_ping(Node *node);
int swim_indirect_ping(Node *node, Node *proxy);
int swim_update_state(const char *node_id, NodeState new_state);

// Message handling
Message *message_create(MessageType type, const void *payload, size_t payload_size);
int message_send(Message *msg, const char *address, uint16_t port);
Message *message_receive(int socket_fd, int timeout_ms);
void message_destroy(Message *msg);

// Serialization (simple format for now)
int serialize_node(Node *node, uint8_t *buffer, size_t buffer_size);
int deserialize_node(const uint8_t *buffer, size_t buffer_size, Node *out_node);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

// Node registry
int node_registry_add(Node *node);
int node_registry_remove(const char *node_id);
Node *node_registry_find(const char *node_id);
void node_registry_update(Node *node);

// Gossip background thread
void *gossip_thread_main(void *arg);

// Health check background thread
void *health_check_thread_main(void *arg);

#endif /* CONSTELLATION_CLUSTER_H */
