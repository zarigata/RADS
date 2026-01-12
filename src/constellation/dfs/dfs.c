/*
 * RADS Constellation - Distributed Filesystem Implementation
 * Phase 4: Distributed Filesystem
 */

#define _GNU_SOURCE

#include "dfs.h"
#include "dht.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static DFSConfig g_config;
static pthread_mutex_t g_dfs_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_initialized = false;

// File metadata cache
typedef struct {
    char path[512];
    FileMetadata *metadata;
} MetadataCache;

static MetadataCache **g_metadata_cache = NULL;
static int g_metadata_cache_count = 0;

// Statistics
static DFSStats g_stats = {0};

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int dfs_init(DFSConfig *config) {
    pthread_mutex_lock(&g_dfs_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_dfs_mutex);
        return 1;
    }

    // Set configuration
    if (config) {
        g_config = *config;
    } else {
        // Defaults
        g_config.replication_factor = 3;
        g_config.consistency = DFS_CONSISTENCY_EVENTUAL;
        g_config.conflict_res = DFS_CONFLICT_LAST_WRITE_WINS;
        g_config.cache_size_mb = 1024;
        g_config.sync_interval_ms = 5000;
    }

    // Initialize DHT
    DHTConfig dht_config = {
        .vnodes_per_node = 150,
        .replication_factor = g_config.replication_factor
    };

    if (dht_init(&dht_config) != 0) {
        pthread_mutex_unlock(&g_dfs_mutex);
        return -1;
    }

    // Initialize metadata cache
    g_metadata_cache = NULL;
    g_metadata_cache_count = 0;

    // Reset statistics
    memset(&g_stats, 0, sizeof(DFSStats));

    g_initialized = true;

    pthread_mutex_unlock(&g_dfs_mutex);
    return 0;
}

int dfs_shutdown(void) {
    pthread_mutex_lock(&g_dfs_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_dfs_mutex);
        return -1;
    }

    // Free metadata cache
    for (int i = 0; i < g_metadata_cache_count; i++) {
        if (g_metadata_cache[i]) {
            dfs_free_metadata(g_metadata_cache[i]->metadata);
            free(g_metadata_cache[i]);
        }
    }
    free(g_metadata_cache);
    g_metadata_cache = NULL;
    g_metadata_cache_count = 0;

    // Shutdown DHT
    dht_shutdown();

    g_initialized = false;

    pthread_mutex_unlock(&g_dfs_mutex);
    return 0;
}

bool dfs_is_running(void) {
    pthread_mutex_lock(&g_dfs_mutex);
    bool running = g_initialized;
    pthread_mutex_unlock(&g_dfs_mutex);
    return running;
}

/*
 * ============================================================================
 * FILE OPERATIONS
 * ============================================================================
 */

FileHandle *dfs_open(const char *path, int flags) {
    if (!g_initialized || !path) return NULL;

    pthread_mutex_lock(&g_dfs_mutex);

    FileHandle *handle = calloc(1, sizeof(FileHandle));
    if (!handle) {
        pthread_mutex_unlock(&g_dfs_mutex);
        return NULL;
    }

    strncpy(handle->path, path, sizeof(handle->path) - 1);
    handle->flags = flags;
    handle->offset = 0;
    handle->is_dirty = false;

    // Get file metadata
    handle->metadata = dfs_stat(path);

    g_stats.reads++;

    pthread_mutex_unlock(&g_dfs_mutex);
    return handle;
}

int dfs_close(FileHandle *handle) {
    if (!handle) return -1;

    // Flush if dirty
    if (handle->is_dirty) {
        dfs_flush(handle);
    }

    // Free metadata
    if (handle->metadata) {
        dfs_free_metadata(handle->metadata);
    }

    free(handle);
    return 0;
}

ssize_t dfs_read(FileHandle *handle, void *buf, size_t count) {
    if (!handle || !buf) return -1;

    // For now, this is a stub - would read from replicas
    g_stats.reads++;
    return 0;
}

ssize_t dfs_write(FileHandle *handle, const void *buf, size_t count) {
    if (!handle || !buf) return -1;

    handle->is_dirty = true;

    // For now, this is a stub - would write to replicas
    g_stats.writes++;
    return count;
}

int dfs_seek(FileHandle *handle, off_t offset, int whence) {
    if (!handle) return -1;

    switch (whence) {
        case SEEK_SET:
            handle->offset = offset;
            break;
        case SEEK_CUR:
            handle->offset += offset;
            break;
        case SEEK_END:
            if (handle->metadata) {
                handle->offset = handle->metadata->size + offset;
            }
            break;
        default:
            return -1;
    }

    return 0;
}

int dfs_flush(FileHandle *handle) {
    if (!handle) return -1;

    if (!handle->is_dirty) {
        return 0; // Nothing to flush
    }

    // Sync to replicas
    int result = dfs_sync(handle->path);

    if (result == 0) {
        handle->is_dirty = false;
    }

    return result;
}

/*
 * ============================================================================
 * FILE MANAGEMENT
 * ============================================================================
 */

int dfs_create(const char *path, uint32_t permissions) {
    if (!g_initialized || !path) return -1;

    pthread_mutex_lock(&g_dfs_mutex);

    // Create metadata
    FileMetadata *metadata = calloc(1, sizeof(FileMetadata));
    if (!metadata) {
        pthread_mutex_unlock(&g_dfs_mutex);
        return -1;
    }

    strncpy(metadata->path, path, sizeof(metadata->path) - 1);
    metadata->size = 0;
    metadata->created_at = time(NULL);
    metadata->modified_at = metadata->created_at;
    metadata->accessed_at = metadata->created_at;
    metadata->version = 1;
    metadata->hash = dht_hash_string(path);
    metadata->replica_count = 0;
    metadata->is_directory = false;
    metadata->permissions = permissions;

    // Store in cache
    MetadataCache *cache = calloc(1, sizeof(MetadataCache));
    if (cache) {
        strncpy(cache->path, path, sizeof(cache->path) - 1);
        cache->metadata = metadata;

        g_metadata_cache = realloc(g_metadata_cache,
                                   sizeof(MetadataCache *) * (g_metadata_cache_count + 1));
        g_metadata_cache[g_metadata_cache_count] = cache;
        g_metadata_cache_count++;
    }

    g_stats.total_files++;

    pthread_mutex_unlock(&g_dfs_mutex);
    return 0;
}

int dfs_delete(const char *path) {
    if (!g_initialized || !path) return -1;

    pthread_mutex_lock(&g_dfs_mutex);

    // Find and remove from cache
    for (int i = 0; i < g_metadata_cache_count; i++) {
        if (strcmp(g_metadata_cache[i]->path, path) == 0) {
            dfs_free_metadata(g_metadata_cache[i]->metadata);
            free(g_metadata_cache[i]);

            // Shift array
            for (int j = i; j < g_metadata_cache_count - 1; j++) {
                g_metadata_cache[j] = g_metadata_cache[j + 1];
            }

            g_metadata_cache_count--;
            g_stats.total_files--;
            break;
        }
    }

    pthread_mutex_unlock(&g_dfs_mutex);
    return 0;
}

int dfs_rename(const char *old_path, const char *new_path) {
    if (!g_initialized || !old_path || !new_path) return -1;

    pthread_mutex_lock(&g_dfs_mutex);

    // Find in cache and update path
    for (int i = 0; i < g_metadata_cache_count; i++) {
        if (strcmp(g_metadata_cache[i]->path, old_path) == 0) {
            strncpy(g_metadata_cache[i]->path, new_path, sizeof(g_metadata_cache[i]->path) - 1);
            strncpy(g_metadata_cache[i]->metadata->path, new_path,
                    sizeof(g_metadata_cache[i]->metadata->path) - 1);
            break;
        }
    }

    pthread_mutex_unlock(&g_dfs_mutex);
    return 0;
}

/*
 * ============================================================================
 * DIRECTORY OPERATIONS
 * ============================================================================
 */

int dfs_mkdir(const char *path, uint32_t permissions) {
    if (!g_initialized || !path) return -1;

    pthread_mutex_lock(&g_dfs_mutex);

    // Create directory metadata (similar to file)
    FileMetadata *metadata = calloc(1, sizeof(FileMetadata));
    if (!metadata) {
        pthread_mutex_unlock(&g_dfs_mutex);
        return -1;
    }

    strncpy(metadata->path, path, sizeof(metadata->path) - 1);
    metadata->size = 0;
    metadata->created_at = time(NULL);
    metadata->modified_at = metadata->created_at;
    metadata->accessed_at = metadata->created_at;
    metadata->version = 1;
    metadata->hash = dht_hash_string(path);
    metadata->is_directory = true;
    metadata->permissions = permissions;

    // Store in cache
    MetadataCache *cache = calloc(1, sizeof(MetadataCache));
    if (cache) {
        strncpy(cache->path, path, sizeof(cache->path) - 1);
        cache->metadata = metadata;

        g_metadata_cache = realloc(g_metadata_cache,
                                   sizeof(MetadataCache *) * (g_metadata_cache_count + 1));
        g_metadata_cache[g_metadata_cache_count] = cache;
        g_metadata_cache_count++;
    }

    g_stats.total_directories++;

    pthread_mutex_unlock(&g_dfs_mutex);
    return 0;
}

DirectoryEntry **dfs_listdir(const char *path, int *out_count) {
    if (!g_initialized || !path || !out_count) return NULL;

    pthread_mutex_lock(&g_dfs_mutex);

    // Count matching entries
    int count = 0;
    size_t path_len = strlen(path);

    for (int i = 0; i < g_metadata_cache_count; i++) {
        const char *entry_path = g_metadata_cache[i]->path;

        // Check if entry is in this directory
        if (strncmp(entry_path, path, path_len) == 0 &&
            entry_path[path_len] == '/' &&
            strchr(entry_path + path_len + 1, '/') == NULL) {
            count++;
        }
    }

    if (count == 0) {
        *out_count = 0;
        pthread_mutex_unlock(&g_dfs_mutex);
        return NULL;
    }

    // Allocate entries
    DirectoryEntry **entries = calloc(count, sizeof(DirectoryEntry *));
    int entry_idx = 0;

    for (int i = 0; i < g_metadata_cache_count; i++) {
        const char *entry_path = g_metadata_cache[i]->path;

        if (strncmp(entry_path, path, path_len) == 0 &&
            entry_path[path_len] == '/' &&
            strchr(entry_path + path_len + 1, '/') == NULL) {

            DirectoryEntry *entry = calloc(1, sizeof(DirectoryEntry));
            const char *name = entry_path + path_len + 1;
            strncpy(entry->name, name, sizeof(entry->name) - 1);
            entry->is_directory = g_metadata_cache[i]->metadata->is_directory;
            entry->size = g_metadata_cache[i]->metadata->size;
            entry->modified_at = g_metadata_cache[i]->metadata->modified_at;

            entries[entry_idx++] = entry;
        }
    }

    *out_count = count;

    pthread_mutex_unlock(&g_dfs_mutex);
    return entries;
}

void dfs_free_dirlist(DirectoryEntry **entries, int count) {
    if (!entries) return;

    for (int i = 0; i < count; i++) {
        free(entries[i]);
    }
    free(entries);
}

/*
 * ============================================================================
 * METADATA
 * ============================================================================
 */

FileMetadata *dfs_stat(const char *path) {
    if (!g_initialized || !path) return NULL;

    pthread_mutex_lock(&g_dfs_mutex);

    // Search cache
    for (int i = 0; i < g_metadata_cache_count; i++) {
        if (strcmp(g_metadata_cache[i]->path, path) == 0) {
            // Duplicate metadata
            FileMetadata *metadata = calloc(1, sizeof(FileMetadata));
            if (metadata) {
                memcpy(metadata, g_metadata_cache[i]->metadata, sizeof(FileMetadata));
            }
            pthread_mutex_unlock(&g_dfs_mutex);
            return metadata;
        }
    }

    pthread_mutex_unlock(&g_dfs_mutex);
    return NULL;
}

void dfs_free_metadata(FileMetadata *metadata) {
    if (!metadata) return;

    if (metadata->replica_nodes) {
        for (int i = 0; i < metadata->replica_count; i++) {
            free(metadata->replica_nodes[i]);
        }
        free(metadata->replica_nodes);
    }

    free(metadata);
}

/*
 * ============================================================================
 * SYNCHRONIZATION
 * ============================================================================
 */

int dfs_sync(const char *path) {
    if (!g_initialized || !path) return -1;

    // Get file location
    FileLocation *location = dht_locate_file(path);
    if (!location) return -1;

    // Replicate to all nodes
    int result = dfs_replicate_file(path, location->replicas, location->replica_count);

    free(location->replicas);
    free(location);

    return result;
}

int dfs_sync_all(void) {
    if (!g_initialized) return -1;

    pthread_mutex_lock(&g_dfs_mutex);

    int synced = 0;
    for (int i = 0; i < g_metadata_cache_count; i++) {
        if (dfs_sync(g_metadata_cache[i]->path) == 0) {
            synced++;
        }
    }

    pthread_mutex_unlock(&g_dfs_mutex);
    return synced;
}

/*
 * ============================================================================
 * REPLICATION
 * ============================================================================
 */

int dfs_replicate_file(const char *path, Node **nodes, int node_count) {
    if (!path || !nodes || node_count == 0) return -1;

    // Stub: Would actually copy file data to replica nodes
    // For now, just return success
    return 0;
}

Node **dfs_get_replicas(const char *path, int *out_count) {
    if (!path || !out_count) return NULL;

    FileLocation *location = dht_locate_file(path);
    if (!location) return NULL;

    *out_count = location->replica_count;
    Node **replicas = location->replicas;

    free(location);
    return replicas;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

DFSStats dfs_get_stats(void) {
    pthread_mutex_lock(&g_dfs_mutex);
    DFSStats stats = g_stats;
    pthread_mutex_unlock(&g_dfs_mutex);
    return stats;
}
