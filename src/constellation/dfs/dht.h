/*
 * RADS Constellation - Distributed Hash Table (DHT)
 * Phase 4: Distributed Filesystem
 *
 * Consistent hashing for distributed file placement
 */

#ifndef CONSTELLATION_DHT_H
#define CONSTELLATION_DHT_H

#include "../constellation.h"
#include "../cluster/cluster.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================================
 * DHT TYPES
 * ============================================================================
 */

// Hash ring position (160-bit hash from SHA-1)
typedef struct {
    uint8_t hash[20];        // SHA-1 hash (160 bits)
} DHTHash;

// Virtual node (for better distribution)
typedef struct {
    char node_id[64];        // Physical node ID
    int vnode_index;         // Virtual node index (0-N)
    DHTHash hash;            // Position on ring
} VirtualNode;

// Hash ring
typedef struct {
    VirtualNode **vnodes;    // Virtual nodes (sorted by hash)
    int vnode_count;         // Number of virtual nodes
    int vnodes_per_node;     // Virtual nodes per physical node
    bool is_sorted;          // Is ring sorted?
} HashRing;

// File location info
typedef struct {
    char file_path[512];     // File path
    DHTHash hash;            // File hash
    Node **replicas;         // Replica nodes (primary + backups)
    int replica_count;       // Number of replicas
} FileLocation;

// DHT configuration
typedef struct {
    int vnodes_per_node;     // Virtual nodes per physical node (default: 150)
    int replication_factor;  // Number of replicas (default: 3)
} DHTConfig;

/*
 * ============================================================================
 * DHT API
 * ============================================================================
 */

// Lifecycle
int dht_init(DHTConfig *config);
int dht_shutdown(void);
bool dht_is_running(void);

// Hash ring management
int dht_add_node(Node *node);
int dht_remove_node(const char *node_id);
int dht_rebuild_ring(void);

// Hashing
DHTHash dht_hash(const void *data, size_t len);
DHTHash dht_hash_string(const char *str);
int dht_hash_compare(const DHTHash *a, const DHTHash *b);

// File placement
FileLocation *dht_locate_file(const char *file_path);
Node **dht_find_replicas(const DHTHash *hash, int count);
Node *dht_find_successor(const DHTHash *hash);

// Node lookup
Node *dht_find_node_for_hash(const DHTHash *hash);
VirtualNode *dht_find_vnode(const DHTHash *hash);

// Rebalancing (when nodes join/leave)
int dht_rebalance(void);
void dht_get_key_range(const char *node_id, DHTHash *start, DHTHash *end);

// Statistics
typedef struct {
    int total_vnodes;
    int physical_nodes;
    double avg_load;
    double max_load;
    double min_load;
} DHTStats;

DHTStats dht_get_stats(void);

// Internal helpers
void dht_sort_ring(void);
int dht_vnode_compare(const void *a, const void *b);

#endif /* CONSTELLATION_DHT_H */
