/*
 * RADS Constellation - Distributed Hash Table Implementation
 * Phase 4: Distributed Filesystem
 */

#define _GNU_SOURCE

#include "dht.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <openssl/sha.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static HashRing *g_ring = NULL;
static DHTConfig g_config;
static pthread_mutex_t g_dht_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_initialized = false;

/*
 * ============================================================================
 * HASHING FUNCTIONS
 * ============================================================================
 */

DHTHash dht_hash(const void *data, size_t len) {
    DHTHash hash;
    SHA1(data, len, hash.hash);
    return hash;
}

DHTHash dht_hash_string(const char *str) {
    return dht_hash(str, strlen(str));
}

int dht_hash_compare(const DHTHash *a, const DHTHash *b) {
    return memcmp(a->hash, b->hash, 20);
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int dht_init(DHTConfig *config) {
    pthread_mutex_lock(&g_dht_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_dht_mutex);
        return 1; // Already initialized
    }

    // Set configuration
    if (config) {
        g_config = *config;
    } else {
        g_config.vnodes_per_node = 150;
        g_config.replication_factor = 3;
    }

    // Initialize hash ring
    g_ring = calloc(1, sizeof(HashRing));
    if (!g_ring) {
        pthread_mutex_unlock(&g_dht_mutex);
        return -1;
    }

    g_ring->vnodes = NULL;
    g_ring->vnode_count = 0;
    g_ring->vnodes_per_node = g_config.vnodes_per_node;
    g_ring->is_sorted = true;

    g_initialized = true;

    pthread_mutex_unlock(&g_dht_mutex);
    return 0;
}

int dht_shutdown(void) {
    pthread_mutex_lock(&g_dht_mutex);

    if (!g_initialized || !g_ring) {
        pthread_mutex_unlock(&g_dht_mutex);
        return -1;
    }

    // Free all virtual nodes
    for (int i = 0; i < g_ring->vnode_count; i++) {
        free(g_ring->vnodes[i]);
    }
    free(g_ring->vnodes);

    free(g_ring);
    g_ring = NULL;
    g_initialized = false;

    pthread_mutex_unlock(&g_dht_mutex);
    return 0;
}

bool dht_is_running(void) {
    pthread_mutex_lock(&g_dht_mutex);
    bool running = g_initialized;
    pthread_mutex_unlock(&g_dht_mutex);
    return running;
}

/*
 * ============================================================================
 * HASH RING MANAGEMENT
 * ============================================================================
 */

int dht_vnode_compare(const void *a, const void *b) {
    const VirtualNode *vnode_a = *(const VirtualNode **)a;
    const VirtualNode *vnode_b = *(const VirtualNode **)b;
    return dht_hash_compare(&vnode_a->hash, &vnode_b->hash);
}

void dht_sort_ring(void) {
    if (!g_ring || g_ring->is_sorted) return;

    qsort(g_ring->vnodes, g_ring->vnode_count, sizeof(VirtualNode *), dht_vnode_compare);
    g_ring->is_sorted = true;
}

int dht_add_node(Node *node) {
    if (!g_initialized || !node) return -1;

    pthread_mutex_lock(&g_dht_mutex);

    // Create virtual nodes for this physical node
    for (int i = 0; i < g_ring->vnodes_per_node; i++) {
        VirtualNode *vnode = calloc(1, sizeof(VirtualNode));
        if (!vnode) {
            pthread_mutex_unlock(&g_dht_mutex);
            return -1;
        }

        strncpy(vnode->node_id, node->id, sizeof(vnode->node_id) - 1);
        vnode->vnode_index = i;

        // Hash: node_id + vnode_index
        char hash_input[128];
        snprintf(hash_input, sizeof(hash_input), "%s-%d", node->id, i);
        vnode->hash = dht_hash_string(hash_input);

        // Add to ring
        g_ring->vnodes = realloc(g_ring->vnodes,
                                 sizeof(VirtualNode *) * (g_ring->vnode_count + 1));
        g_ring->vnodes[g_ring->vnode_count] = vnode;
        g_ring->vnode_count++;
    }

    g_ring->is_sorted = false;
    dht_sort_ring();

    pthread_mutex_unlock(&g_dht_mutex);
    return 0;
}

int dht_remove_node(const char *node_id) {
    if (!g_initialized || !node_id) return -1;

    pthread_mutex_lock(&g_dht_mutex);

    // Remove all virtual nodes for this physical node
    int removed = 0;
    for (int i = 0; i < g_ring->vnode_count; ) {
        if (strcmp(g_ring->vnodes[i]->node_id, node_id) == 0) {
            free(g_ring->vnodes[i]);

            // Shift array
            for (int j = i; j < g_ring->vnode_count - 1; j++) {
                g_ring->vnodes[j] = g_ring->vnodes[j + 1];
            }

            g_ring->vnode_count--;
            removed++;
        } else {
            i++;
        }
    }

    pthread_mutex_unlock(&g_dht_mutex);
    return removed;
}

int dht_rebuild_ring(void) {
    if (!g_initialized) return -1;

    pthread_mutex_lock(&g_dht_mutex);

    // Just re-sort the ring
    g_ring->is_sorted = false;
    dht_sort_ring();

    pthread_mutex_unlock(&g_dht_mutex);
    return 0;
}

/*
 * ============================================================================
 * FILE PLACEMENT
 * ============================================================================
 */

VirtualNode *dht_find_vnode(const DHTHash *hash) {
    if (!g_initialized || !g_ring || g_ring->vnode_count == 0) {
        return NULL;
    }

    // Binary search for successor
    int left = 0;
    int right = g_ring->vnode_count - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        int cmp = dht_hash_compare(hash, &g_ring->vnodes[mid]->hash);

        if (cmp == 0) {
            return g_ring->vnodes[mid];
        } else if (cmp < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    // If not found, wrap around to first vnode
    if (left >= g_ring->vnode_count) {
        return g_ring->vnodes[0];
    }

    return g_ring->vnodes[left];
}

Node *dht_find_successor(const DHTHash *hash) {
    VirtualNode *vnode = dht_find_vnode(hash);
    if (!vnode) return NULL;

    // Get the actual node from cluster
    return cluster_find_node(vnode->node_id);
}

Node **dht_find_replicas(const DHTHash *hash, int count) {
    if (!g_initialized || !g_ring || g_ring->vnode_count == 0 || count <= 0) {
        return NULL;
    }

    pthread_mutex_lock(&g_dht_mutex);

    Node **replicas = calloc(count, sizeof(Node *));
    if (!replicas) {
        pthread_mutex_unlock(&g_dht_mutex);
        return NULL;
    }

    // Find first vnode
    VirtualNode *vnode = dht_find_vnode(hash);
    if (!vnode) {
        free(replicas);
        pthread_mutex_unlock(&g_dht_mutex);
        return NULL;
    }

    // Find index of this vnode
    int start_idx = -1;
    for (int i = 0; i < g_ring->vnode_count; i++) {
        if (g_ring->vnodes[i] == vnode) {
            start_idx = i;
            break;
        }
    }

    if (start_idx == -1) {
        free(replicas);
        pthread_mutex_unlock(&g_dht_mutex);
        return NULL;
    }

    // Collect unique physical nodes
    int replica_count = 0;
    char *seen_nodes[count];
    int seen_count = 0;

    for (int i = 0; i < g_ring->vnode_count && replica_count < count; i++) {
        int idx = (start_idx + i) % g_ring->vnode_count;
        VirtualNode *current = g_ring->vnodes[idx];

        // Check if we've already added this physical node
        bool already_seen = false;
        for (int j = 0; j < seen_count; j++) {
            if (strcmp(seen_nodes[j], current->node_id) == 0) {
                already_seen = true;
                break;
            }
        }

        if (!already_seen) {
            Node *node = cluster_find_node(current->node_id);
            if (node && node->state == NODE_STATE_ALIVE) {
                replicas[replica_count] = node;
                seen_nodes[seen_count] = current->node_id;
                seen_count++;
                replica_count++;
            }
        }
    }

    pthread_mutex_unlock(&g_dht_mutex);

    // If we couldn't find enough replicas, return what we have
    if (replica_count == 0) {
        free(replicas);
        return NULL;
    }

    return replicas;
}

FileLocation *dht_locate_file(const char *file_path) {
    if (!g_initialized || !file_path) return NULL;

    FileLocation *location = calloc(1, sizeof(FileLocation));
    if (!location) return NULL;

    strncpy(location->file_path, file_path, sizeof(location->file_path) - 1);
    location->hash = dht_hash_string(file_path);

    // Find replica nodes
    location->replicas = dht_find_replicas(&location->hash, g_config.replication_factor);

    if (location->replicas) {
        // Count non-NULL replicas
        location->replica_count = 0;
        for (int i = 0; i < g_config.replication_factor; i++) {
            if (location->replicas[i]) {
                location->replica_count++;
            }
        }
    } else {
        location->replica_count = 0;
    }

    return location;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

DHTStats dht_get_stats(void) {
    DHTStats stats = {0};

    if (!g_initialized || !g_ring) {
        return stats;
    }

    pthread_mutex_lock(&g_dht_mutex);

    stats.total_vnodes = g_ring->vnode_count;
    stats.physical_nodes = g_ring->vnode_count / g_ring->vnodes_per_node;

    // Calculate load distribution (simplified)
    if (stats.physical_nodes > 0) {
        stats.avg_load = (double)g_ring->vnode_count / stats.physical_nodes;
        stats.max_load = g_ring->vnodes_per_node;
        stats.min_load = g_ring->vnodes_per_node;
    }

    pthread_mutex_unlock(&g_dht_mutex);

    return stats;
}

/*
 * ============================================================================
 * REBALANCING
 * ============================================================================
 */

int dht_rebalance(void) {
    // For now, just rebuild the ring
    // In a real implementation, this would move data between nodes
    return dht_rebuild_ring();
}

void dht_get_key_range(const char *node_id, DHTHash *start, DHTHash *end) {
    if (!g_initialized || !node_id || !start || !end) return;

    pthread_mutex_lock(&g_dht_mutex);

    // Find first and last vnode for this physical node
    VirtualNode *first = NULL;
    VirtualNode *last = NULL;

    for (int i = 0; i < g_ring->vnode_count; i++) {
        if (strcmp(g_ring->vnodes[i]->node_id, node_id) == 0) {
            if (!first) first = g_ring->vnodes[i];
            last = g_ring->vnodes[i];
        }
    }

    if (first && last) {
        *start = first->hash;
        *end = last->hash;
    }

    pthread_mutex_unlock(&g_dht_mutex);
}
