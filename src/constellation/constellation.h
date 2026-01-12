/*
 * RADS Constellation - Core Header
 *
 * Distributed computing and orchestration platform for RADS
 * Version: 0.0.4 "Constellation"
 */

#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

/*
 * ============================================================================
 * CORE TYPES
 * ============================================================================
 */

// Instance states
typedef enum {
    INSTANCE_STATE_PENDING,      // Created but not started
    INSTANCE_STATE_STARTING,     // Starting up
    INSTANCE_STATE_RUNNING,      // Running normally
    INSTANCE_STATE_STOPPING,     // Shutting down
    INSTANCE_STATE_STOPPED,      // Stopped cleanly
    INSTANCE_STATE_FAILED,       // Failed/crashed
    INSTANCE_STATE_UNKNOWN       // Unknown state
} InstanceState;

// Resource limits
typedef struct {
    uint32_t cpu_percent;        // CPU percentage (0-100 per core)
    uint64_t ram_mb;             // RAM limit in MB
    uint64_t disk_mb;            // Disk quota in MB
    uint32_t network_mbps;       // Network bandwidth limit
} ResourceLimits;

// Resource usage (current)
typedef struct {
    double cpu_percent;          // Current CPU usage
    uint64_t ram_mb;             // Current RAM usage
    uint64_t disk_mb;            // Current disk usage
    uint64_t network_in_mbps;    // Network in
    uint64_t network_out_mbps;   // Network out
    time_t last_update;          // Last update time
} ResourceUsage;

// Instance configuration
typedef struct {
    char name[256];              // Instance name
    char script_path[512];       // Path to RADS script
    char **env_vars;             // Environment variables (NULL-terminated)
    int env_count;               // Number of env vars
    ResourceLimits limits;       // Resource limits
    bool enable_networking;      // Enable network access
    bool enable_filesystem;      // Enable filesystem access
} InstanceConfig;

// Instance metadata
typedef struct {
    char id[64];                 // Unique instance ID
    char name[256];              // Instance name
    pid_t pid;                   // Process ID
    InstanceState state;         // Current state
    time_t created_at;           // Creation timestamp
    time_t started_at;           // Start timestamp
    time_t stopped_at;           // Stop timestamp
    int exit_code;               // Exit code (if stopped)
    ResourceLimits limits;       // Configured limits
    ResourceUsage usage;         // Current usage
    void *isolation_context;     // Isolation context (namespaces, cgroups)
} Instance;

// Controller configuration
typedef struct {
    char node_name[256];         // This node's name
    char cluster_name[256];      // Cluster name
    uint16_t control_port;       // Control plane port
    uint16_t data_port;          // Data plane port
    uint32_t max_instances;      // Maximum instances
    bool enable_clustering;      // Enable multi-node
} ControllerConfig;

// Error codes
typedef enum {
    CONSTELLATION_OK = 0,
    CONSTELLATION_ERR_INVALID_PARAM = 1,
    CONSTELLATION_ERR_NO_MEMORY = 2,
    CONSTELLATION_ERR_SPAWN_FAILED = 3,
    CONSTELLATION_ERR_NOT_FOUND = 4,
    CONSTELLATION_ERR_ALREADY_EXISTS = 5,
    CONSTELLATION_ERR_PERMISSION_DENIED = 6,
    CONSTELLATION_ERR_RESOURCE_EXHAUSTED = 7,
    CONSTELLATION_ERR_ISOLATION_FAILED = 8,
    CONSTELLATION_ERR_CGROUP_FAILED = 9,
    CONSTELLATION_ERR_NAMESPACE_FAILED = 10,
    CONSTELLATION_ERR_UNKNOWN = 99
} ConstellationError;

/*
 * ============================================================================
 * CORE API
 * ============================================================================
 */

// Controller lifecycle
int constellation_init(ControllerConfig *config);
int constellation_shutdown(void);
bool constellation_is_running(void);

// Instance management
ConstellationError constellation_instance_create(
    const InstanceConfig *config,
    Instance **out_instance
);

ConstellationError constellation_instance_start(Instance *instance);
ConstellationError constellation_instance_stop(Instance *instance);
ConstellationError constellation_instance_destroy(Instance *instance);

// Instance query
Instance *constellation_instance_get(const char *instance_id);
Instance **constellation_instance_list(int *out_count);
InstanceState constellation_instance_state(Instance *instance);

// Resource monitoring
ConstellationError constellation_instance_update_stats(Instance *instance);
ResourceUsage constellation_instance_get_usage(Instance *instance);

// Error handling
const char *constellation_error_string(ConstellationError error);

/*
 * ============================================================================
 * INTERNAL API (not exposed to RADS)
 * ============================================================================
 */

// Isolation
int isolation_setup_namespaces(Instance *instance);
int isolation_teardown_namespaces(Instance *instance);

// Resource control
int resources_setup_cgroups(Instance *instance);
int resources_update_limits(Instance *instance, ResourceLimits *limits);
int resources_collect_stats(Instance *instance, ResourceUsage *usage);
int resources_teardown_cgroups(Instance *instance);

// Process management
pid_t process_spawn(Instance *instance);
int process_wait(pid_t pid, int *exit_code);
int process_kill(pid_t pid, int signal);

// Registry
int registry_add_instance(Instance *instance);
int registry_remove_instance(const char *instance_id);
Instance *registry_find_instance(const char *instance_id);
Instance **registry_list_instances(int *out_count);

#endif /* CONSTELLATION_H */
