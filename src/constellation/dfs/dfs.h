/*
 * RADS Constellation - Distributed Filesystem
 * Phase 4: Distributed Filesystem
 *
 * Main distributed filesystem interface
 */

#ifndef CONSTELLATION_DFS_H
#define CONSTELLATION_DFS_H

#include "../constellation.h"
#include "dht.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * DFS TYPES
 * ============================================================================
 */

// File metadata
typedef struct {
    char path[512];           // File path
    uint64_t size;            // File size in bytes
    time_t created_at;        // Creation time
    time_t modified_at;       // Last modification time
    time_t accessed_at;       // Last access time
    uint32_t version;         // File version
    DHTHash hash;             // Content hash
    int replica_count;        // Number of replicas
    char **replica_nodes;     // Node IDs holding replicas
    bool is_directory;        // Is this a directory?
    uint32_t permissions;     // Unix-style permissions
} FileMetadata;

// File handle (for open files)
typedef struct {
    char path[512];           // File path
    int flags;                // Open flags (O_RDONLY, O_WRONLY, O_RDWR)
    uint64_t offset;          // Current read/write offset
    FileMetadata *metadata;   // File metadata
    void *cache;              // Cache handle
    bool is_dirty;            // Has uncommitted changes?
} FileHandle;

// Consistency modes
typedef enum {
    DFS_CONSISTENCY_STRONG,    // Strong consistency (wait for all replicas)
    DFS_CONSISTENCY_EVENTUAL   // Eventual consistency (async replication)
} ConsistencyMode;

// Conflict resolution strategies
typedef enum {
    DFS_CONFLICT_LAST_WRITE_WINS,  // Last write wins
    DFS_CONFLICT_MANUAL,            // Manual resolution required
    DFS_CONFLICT_CRDT               // CRDT-based automatic merge
} ConflictStrategy;

// DFS configuration
typedef struct {
    int replication_factor;        // Number of replicas (default: 3)
    ConsistencyMode consistency;   // Consistency mode
    ConflictStrategy conflict_res; // Conflict resolution strategy
    size_t cache_size_mb;          // Cache size in MB (default: 1024)
    int sync_interval_ms;          // Sync interval for eventual consistency
} DFSConfig;

// Directory entry
typedef struct {
    char name[256];           // Entry name
    bool is_directory;        // Is directory?
    uint64_t size;            // Size (0 for directories)
    time_t modified_at;       // Last modification
} DirectoryEntry;

/*
 * ============================================================================
 * DFS API
 * ============================================================================
 */

// Lifecycle
int dfs_init(DFSConfig *config);
int dfs_shutdown(void);
bool dfs_is_running(void);

// File operations
FileHandle *dfs_open(const char *path, int flags);
int dfs_close(FileHandle *handle);
ssize_t dfs_read(FileHandle *handle, void *buf, size_t count);
ssize_t dfs_write(FileHandle *handle, const void *buf, size_t count);
int dfs_seek(FileHandle *handle, off_t offset, int whence);
int dfs_flush(FileHandle *handle);

// File management
int dfs_create(const char *path, uint32_t permissions);
int dfs_delete(const char *path);
int dfs_rename(const char *old_path, const char *new_path);
int dfs_copy(const char *src_path, const char *dest_path);

// Directory operations
int dfs_mkdir(const char *path, uint32_t permissions);
int dfs_rmdir(const char *path);
DirectoryEntry **dfs_listdir(const char *path, int *out_count);
void dfs_free_dirlist(DirectoryEntry **entries, int count);

// Metadata
FileMetadata *dfs_stat(const char *path);
int dfs_chmod(const char *path, uint32_t permissions);
int dfs_touch(const char *path);
void dfs_free_metadata(FileMetadata *metadata);

// Synchronization
int dfs_sync(const char *path);      // Sync file to all replicas
int dfs_sync_all(void);               // Sync all files

// File watching
typedef void (*FileWatchCallback)(const char *path, const char *event, void *userdata);

int dfs_watch(const char *path, FileWatchCallback callback, void *userdata);
int dfs_unwatch(const char *path);

// Replication
int dfs_set_replication(const char *path, int count);
int dfs_get_replication(const char *path);
Node **dfs_get_replicas(const char *path, int *out_count);

// Sharing
int dfs_share(const char *path, const char *node_id);
int dfs_unshare(const char *path, const char *node_id);
bool dfs_is_shared(const char *path);

// Statistics
typedef struct {
    uint64_t total_files;
    uint64_t total_directories;
    uint64_t total_size_bytes;
    uint64_t replicated_size_bytes;
    double cache_hit_rate;
    uint64_t reads;
    uint64_t writes;
} DFSStats;

DFSStats dfs_get_stats(void);

/*
 * ============================================================================
 * INTERNAL API
 * ============================================================================
 */

// Replication helpers
int dfs_replicate_file(const char *path, Node **nodes, int node_count);
int dfs_verify_replicas(const char *path);

// Cache management
int dfs_cache_put(const char *path, const void *data, size_t size);
int dfs_cache_get(const char *path, void *buf, size_t size);
int dfs_cache_invalidate(const char *path);
int dfs_cache_flush(void);

// Conflict detection
bool dfs_has_conflict(const char *path);
int dfs_resolve_conflict(const char *path, ConflictStrategy strategy);

#endif /* CONSTELLATION_DFS_H */
