/*
 * RADS Constellation - Scheduler Header
 * Phase 3: Resource Orchestration
 *
 * Distributed resource scheduling and placement
 * Implements bin-packing, spread, and constraint-based scheduling
 */

#ifndef CONSTELLATION_SCHEDULER_H
#define CONSTELLATION_SCHEDULER_H

#include "../constellation.h"
#include "../cluster/cluster.h"
#include <stdbool.h>

/*
 * ============================================================================
 * SCHEDULING TYPES
 * ============================================================================
 */

// Scheduling strategies
typedef enum {
    SCHED_STRATEGY_BINPACK,      // Pack instances tightly (minimize nodes)
    SCHED_STRATEGY_SPREAD,       // Spread instances across nodes (high availability)
    SCHED_STRATEGY_RANDOM,       // Random placement
    SCHED_STRATEGY_AFFINITY      // Based on affinity rules
} SchedulingStrategy;

// Constraint types
typedef enum {
    CONSTRAINT_REQUIRED,         // Must match (hard constraint)
    CONSTRAINT_PREFERRED         // Should match (soft constraint)
} ConstraintType;

// Constraint operators
typedef enum {
    CONSTRAINT_OP_EQUALS,        // key=value
    CONSTRAINT_OP_NOT_EQUALS,    // key!=value
    CONSTRAINT_OP_IN,            // key in (value1,value2,...)
    CONSTRAINT_OP_NOT_IN,        // key not in (value1,value2,...)
    CONSTRAINT_OP_EXISTS,        // key exists
    CONSTRAINT_OP_NOT_EXISTS     // key does not exist
} ConstraintOperator;

// Placement constraint
typedef struct {
    char key[128];               // Constraint key (e.g., "node.type", "region")
    ConstraintOperator op;       // Operator
    char **values;               // Values to match
    int value_count;             // Number of values
    ConstraintType type;         // Required or preferred
    int weight;                  // Weight for soft constraints (0-100)
} PlacementConstraint;

// Affinity rule (prefer/avoid placing near certain instances)
typedef struct {
    char service_name[256];      // Service to be affine/anti-affine with
    bool anti_affinity;          // True = avoid, False = prefer
    int weight;                  // Weight (0-100)
} AffinityRule;

// Placement preferences
typedef struct {
    SchedulingStrategy strategy; // Primary strategy

    // Constraints
    PlacementConstraint **constraints;
    int constraint_count;

    // Affinity rules
    AffinityRule **affinity_rules;
    int affinity_rule_count;

    // Tolerations (allow scheduling on tainted nodes)
    char **tolerations;
    int toleration_count;

    // Resource preferences
    bool prefer_cpu_optimized;   // Prefer nodes with more CPU
    bool prefer_memory_optimized; // Prefer nodes with more RAM

    // Spreading
    char spread_across_key[128]; // Spread across this label (e.g., "zone")
} PlacementPreferences;

// Scheduling decision
typedef struct {
    Node *selected_node;         // Node to place instance on
    int score;                   // Placement score (higher = better)
    char reason[512];            // Human-readable reason for placement
    bool success;                // Was scheduling successful?
    char error[512];             // Error message if failed
} SchedulingDecision;

// Resource request (what the instance needs)
typedef struct {
    double cpu_cores;            // CPU cores requested
    uint64_t ram_mb;             // RAM requested in MB
    uint64_t disk_mb;            // Disk requested in MB
    uint64_t network_mbps;       // Network bandwidth requested
    bool needs_gpu;              // Requires GPU
    int gpu_count;               // Number of GPUs
} ResourceRequest;

// Node resources (availability)
typedef struct {
    double cpu_total;            // Total CPU cores
    double cpu_available;        // Available CPU cores
    double cpu_allocated;        // Allocated CPU cores

    uint64_t ram_total_mb;       // Total RAM
    uint64_t ram_available_mb;   // Available RAM
    uint64_t ram_allocated_mb;   // Allocated RAM

    uint64_t disk_total_mb;      // Total disk
    uint64_t disk_available_mb;  // Available disk
    uint64_t disk_allocated_mb;  // Allocated disk

    int instance_count;          // Current instance count
    int instance_capacity;       // Maximum instances

    // Node labels (for constraints)
    char **labels;               // key=value pairs
    int label_count;

    // Node taints (prevent scheduling unless tolerated)
    char **taints;
    int taint_count;
} NodeResources;

/*
 * ============================================================================
 * QUOTA MANAGEMENT
 * ============================================================================
 */

// Resource quota
typedef struct {
    char namespace_name[256];    // Namespace/team name

    // Hard limits
    double max_cpu_cores;        // Maximum CPU
    uint64_t max_ram_mb;         // Maximum RAM
    uint64_t max_disk_mb;        // Maximum disk
    int max_instances;           // Maximum instance count

    // Current usage
    double used_cpu_cores;
    uint64_t used_ram_mb;
    uint64_t used_disk_mb;
    int used_instances;

    // Soft limits (can burst if cluster has capacity)
    bool allow_cpu_burst;
    bool allow_ram_burst;
    double max_burst_percent;    // e.g., 150 = can use 150% of quota

    time_t created_at;
    time_t updated_at;
} ResourceQuota;

/*
 * ============================================================================
 * RESOURCE RESERVATION
 * ============================================================================
 */

// Resource reservation
typedef struct {
    char id[64];                 // Reservation ID
    char namespace_name[256];    // Owner namespace

    ResourceRequest resources;   // Reserved resources
    Node *reserved_on_node;      // Which node (NULL = any)

    time_t created_at;
    time_t expires_at;           // Expiration time
    int priority;                // Priority (1-10)

    bool in_use;                 // Currently being used?
    char instance_id[64];        // Instance using this reservation
} ResourceReservation;

/*
 * ============================================================================
 * SCHEDULER STATE
 * ============================================================================
 */

// Global scheduler state
typedef struct {
    // Node resources (indexed by node ID)
    NodeResources **node_resources;
    int node_count;

    // Quotas (indexed by namespace)
    ResourceQuota **quotas;
    int quota_count;

    // Reservations
    ResourceReservation **reservations;
    int reservation_count;

    // Scheduling statistics
    uint64_t total_scheduled;
    uint64_t total_failed;
    uint64_t total_preempted;

    // Configuration
    int max_scheduling_attempts;
    int scheduling_timeout_ms;

    bool is_initialized;
} SchedulerState;

/*
 * ============================================================================
 * SCHEDULER API
 * ============================================================================
 */

// Lifecycle
int scheduler_init(void);
int scheduler_shutdown(void);
bool scheduler_is_running(void);

// Scheduling
SchedulingDecision *scheduler_schedule(
    const ResourceRequest *request,
    const PlacementPreferences *preferences
);

SchedulingDecision *scheduler_reschedule(
    Instance *instance,
    const PlacementPreferences *preferences
);

void scheduler_decision_free(SchedulingDecision *decision);

// Node resource management
int scheduler_register_node(Node *node);
int scheduler_unregister_node(const char *node_id);
int scheduler_update_node_resources(const char *node_id, NodeResources *resources);
NodeResources *scheduler_get_node_resources(const char *node_id);

// Resource allocation tracking
int scheduler_allocate_resources(const char *node_id, const ResourceRequest *request);
int scheduler_release_resources(const char *node_id, const ResourceRequest *request);

// Quota management
int scheduler_create_quota(const char *namespace_name, ResourceQuota *quota);
int scheduler_update_quota(const char *namespace_name, ResourceQuota *quota);
int scheduler_delete_quota(const char *namespace_name);
ResourceQuota *scheduler_get_quota(const char *namespace_name);
bool scheduler_check_quota(const char *namespace_name, const ResourceRequest *request);

// Resource reservation
ResourceReservation *scheduler_reserve_resources(
    const char *namespace_name,
    const ResourceRequest *request,
    int duration_seconds,
    int priority
);

int scheduler_release_reservation(const char *reservation_id);
Instance *scheduler_create_from_reservation(
    const char *reservation_id,
    const InstanceConfig *config
);

// Placement constraints
PlacementConstraint *placement_constraint_create(
    const char *key,
    ConstraintOperator op,
    const char **values,
    int value_count,
    ConstraintType type
);

void placement_constraint_free(PlacementConstraint *constraint);

// Affinity rules
AffinityRule *affinity_rule_create(
    const char *service_name,
    bool anti_affinity,
    int weight
);

void affinity_rule_free(AffinityRule *rule);

// Placement preferences
PlacementPreferences *placement_preferences_create(SchedulingStrategy strategy);
void placement_preferences_add_constraint(
    PlacementPreferences *prefs,
    PlacementConstraint *constraint
);

void placement_preferences_add_affinity(
    PlacementPreferences *prefs,
    AffinityRule *rule
);

void placement_preferences_free(PlacementPreferences *prefs);

/*
 * ============================================================================
 * INTERNAL SCHEDULER FUNCTIONS
 * ============================================================================
 */

// Scoring functions
int score_node_binpack(Node *node, NodeResources *resources, const ResourceRequest *request);
int score_node_spread(Node *node, NodeResources *resources, const ResourceRequest *request);
int score_node_constraints(Node *node, NodeResources *resources, const PlacementPreferences *prefs);
int score_node_affinity(Node *node, NodeResources *resources, const PlacementPreferences *prefs);
int score_node_overall(Node *node, NodeResources *resources,
                       const ResourceRequest *request,
                       const PlacementPreferences *prefs);

// Feasibility checks
bool node_is_feasible(Node *node, NodeResources *resources, const ResourceRequest *request);
bool node_has_resources(NodeResources *resources, const ResourceRequest *request);
bool node_matches_constraints(Node *node, NodeResources *resources, const PlacementPreferences *prefs);
bool node_tolerates_taints(NodeResources *resources, const PlacementPreferences *prefs);

// Resource calculation
double calculate_cpu_utilization(NodeResources *resources);
double calculate_ram_utilization(NodeResources *resources);
double calculate_overall_utilization(NodeResources *resources);

// Node label management
int node_add_label(NodeResources *resources, const char *key, const char *value);
int node_remove_label(NodeResources *resources, const char *key);
const char *node_get_label(NodeResources *resources, const char *key);
bool node_has_label(NodeResources *resources, const char *key, const char *value);

// Node taint management
int node_add_taint(NodeResources *resources, const char *taint);
int node_remove_taint(NodeResources *resources, const char *taint);
bool node_has_taint(NodeResources *resources, const char *taint);

#endif /* CONSTELLATION_SCHEDULER_H */
