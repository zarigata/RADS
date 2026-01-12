/*
 * RADS Constellation - Scheduler Implementation
 * Phase 3: Resource Orchestration
 *
 * Distributed resource scheduling and placement
 */

#define _GNU_SOURCE

#include "scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static SchedulerState *g_scheduler = NULL;
static pthread_mutex_t g_scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

static uint64_t get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void generate_id(char *out_id, size_t size) {
    snprintf(out_id, size, "res-%lu-%d", get_current_time_ms(), rand() % 10000);
}

/*
 * ============================================================================
 * SCHEDULER LIFECYCLE
 * ============================================================================
 */

int scheduler_init(void) {
    pthread_mutex_lock(&g_scheduler_mutex);

    if (g_scheduler != NULL) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return 1; // Already initialized
    }

    g_scheduler = calloc(1, sizeof(SchedulerState));
    if (!g_scheduler) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -1;
    }

    g_scheduler->node_resources = NULL;
    g_scheduler->node_count = 0;
    g_scheduler->quotas = NULL;
    g_scheduler->quota_count = 0;
    g_scheduler->reservations = NULL;
    g_scheduler->reservation_count = 0;

    g_scheduler->total_scheduled = 0;
    g_scheduler->total_failed = 0;
    g_scheduler->total_preempted = 0;

    g_scheduler->max_scheduling_attempts = 10;
    g_scheduler->scheduling_timeout_ms = 5000;

    g_scheduler->is_initialized = true;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return 0;
}

int scheduler_shutdown(void) {
    pthread_mutex_lock(&g_scheduler_mutex);

    if (!g_scheduler) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -1;
    }

    // Free node resources
    for (int i = 0; i < g_scheduler->node_count; i++) {
        NodeResources *res = g_scheduler->node_resources[i];
        if (res) {
            // Free labels
            for (int j = 0; j < res->label_count; j++) {
                free(res->labels[j]);
            }
            free(res->labels);

            // Free taints
            for (int j = 0; j < res->taint_count; j++) {
                free(res->taints[j]);
            }
            free(res->taints);

            free(res);
        }
    }
    free(g_scheduler->node_resources);

    // Free quotas
    for (int i = 0; i < g_scheduler->quota_count; i++) {
        free(g_scheduler->quotas[i]);
    }
    free(g_scheduler->quotas);

    // Free reservations
    for (int i = 0; i < g_scheduler->reservation_count; i++) {
        free(g_scheduler->reservations[i]);
    }
    free(g_scheduler->reservations);

    free(g_scheduler);
    g_scheduler = NULL;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return 0;
}

bool scheduler_is_running(void) {
    pthread_mutex_lock(&g_scheduler_mutex);
    bool running = (g_scheduler != NULL && g_scheduler->is_initialized);
    pthread_mutex_unlock(&g_scheduler_mutex);
    return running;
}

/*
 * ============================================================================
 * NODE RESOURCE MANAGEMENT
 * ============================================================================
 */

int scheduler_register_node(Node *node) {
    if (!g_scheduler || !node) return -1;

    pthread_mutex_lock(&g_scheduler_mutex);

    // Check if node already registered
    for (int i = 0; i < g_scheduler->node_count; i++) {
        if (strcmp(g_scheduler->node_resources[i]->labels[0], node->id) == 0) {
            pthread_mutex_unlock(&g_scheduler_mutex);
            return 1; // Already registered
        }
    }

    // Create node resources
    NodeResources *res = calloc(1, sizeof(NodeResources));
    if (!res) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -1;
    }

    res->cpu_total = node->cpu_total;
    res->cpu_available = node->cpu_available;
    res->cpu_allocated = 0;

    res->ram_total_mb = node->ram_total_mb;
    res->ram_available_mb = node->ram_available_mb;
    res->ram_allocated_mb = 0;

    res->disk_total_mb = 0;  // TODO: Get from node
    res->disk_available_mb = 0;
    res->disk_allocated_mb = 0;

    res->instance_count = node->current_instances;
    res->instance_capacity = node->max_instances;

    // Initialize labels with node ID
    res->labels = calloc(1, sizeof(char*));
    res->labels[0] = calloc(256, sizeof(char));
    snprintf(res->labels[0], 256, "node.id=%s", node->id);
    res->label_count = 1;

    res->taints = NULL;
    res->taint_count = 0;

    // Add to scheduler
    g_scheduler->node_resources = realloc(
        g_scheduler->node_resources,
        sizeof(NodeResources*) * (g_scheduler->node_count + 1)
    );

    g_scheduler->node_resources[g_scheduler->node_count] = res;
    g_scheduler->node_count++;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return 0;
}

int scheduler_unregister_node(const char *node_id) {
    if (!g_scheduler || !node_id) return -1;

    pthread_mutex_lock(&g_scheduler_mutex);

    for (int i = 0; i < g_scheduler->node_count; i++) {
        NodeResources *res = g_scheduler->node_resources[i];

        // Check if this is the node (look for node.id label)
        char label_to_find[256];
        snprintf(label_to_find, sizeof(label_to_find), "node.id=%s", node_id);

        if (strcmp(res->labels[0], label_to_find) == 0) {
            // Found it - free and remove
            for (int j = 0; j < res->label_count; j++) {
                free(res->labels[j]);
            }
            free(res->labels);

            for (int j = 0; j < res->taint_count; j++) {
                free(res->taints[j]);
            }
            free(res->taints);

            free(res);

            // Shift array
            for (int j = i; j < g_scheduler->node_count - 1; j++) {
                g_scheduler->node_resources[j] = g_scheduler->node_resources[j + 1];
            }

            g_scheduler->node_count--;

            pthread_mutex_unlock(&g_scheduler_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_scheduler_mutex);
    return -1; // Not found
}

NodeResources *scheduler_get_node_resources(const char *node_id) {
    if (!g_scheduler || !node_id) return NULL;

    pthread_mutex_lock(&g_scheduler_mutex);

    for (int i = 0; i < g_scheduler->node_count; i++) {
        NodeResources *res = g_scheduler->node_resources[i];

        char label_to_find[256];
        snprintf(label_to_find, sizeof(label_to_find), "node.id=%s", node_id);

        if (strcmp(res->labels[0], label_to_find) == 0) {
            pthread_mutex_unlock(&g_scheduler_mutex);
            return res;
        }
    }

    pthread_mutex_unlock(&g_scheduler_mutex);
    return NULL;
}

int scheduler_allocate_resources(const char *node_id, const ResourceRequest *request) {
    if (!g_scheduler || !node_id || !request) return -1;

    pthread_mutex_lock(&g_scheduler_mutex);

    NodeResources *res = scheduler_get_node_resources(node_id);
    if (!res) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -1;
    }

    // Check if resources available
    if (res->cpu_available < request->cpu_cores ||
        res->ram_available_mb < request->ram_mb) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -2; // Insufficient resources
    }

    // Allocate
    res->cpu_allocated += request->cpu_cores;
    res->cpu_available -= request->cpu_cores;
    res->ram_allocated_mb += request->ram_mb;
    res->ram_available_mb -= request->ram_mb;
    res->disk_allocated_mb += request->disk_mb;
    res->disk_available_mb -= request->disk_mb;
    res->instance_count++;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return 0;
}

int scheduler_release_resources(const char *node_id, const ResourceRequest *request) {
    if (!g_scheduler || !node_id || !request) return -1;

    pthread_mutex_lock(&g_scheduler_mutex);

    NodeResources *res = scheduler_get_node_resources(node_id);
    if (!res) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -1;
    }

    // Release
    res->cpu_allocated -= request->cpu_cores;
    res->cpu_available += request->cpu_cores;
    res->ram_allocated_mb -= request->ram_mb;
    res->ram_available_mb += request->ram_mb;
    res->disk_allocated_mb -= request->disk_mb;
    res->disk_available_mb += request->disk_mb;
    res->instance_count--;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return 0;
}

/*
 * ============================================================================
 * SCHEDULING ALGORITHMS
 * ============================================================================
 */

// Check if node has sufficient resources
bool node_has_resources(NodeResources *resources, const ResourceRequest *request) {
    if (!resources || !request) return false;

    return (resources->cpu_available >= request->cpu_cores &&
            resources->ram_available_mb >= request->ram_mb &&
            resources->disk_available_mb >= request->disk_mb &&
            resources->instance_count < resources->instance_capacity);
}

// Calculate CPU utilization percentage
double calculate_cpu_utilization(NodeResources *resources) {
    if (!resources || resources->cpu_total == 0) return 0.0;
    return (resources->cpu_allocated / resources->cpu_total) * 100.0;
}

// Calculate RAM utilization percentage
double calculate_ram_utilization(NodeResources *resources) {
    if (!resources || resources->ram_total_mb == 0) return 0.0;
    return ((double)resources->ram_allocated_mb / resources->ram_total_mb) * 100.0;
}

// Calculate overall utilization
double calculate_overall_utilization(NodeResources *resources) {
    if (!resources) return 0.0;

    double cpu_util = calculate_cpu_utilization(resources);
    double ram_util = calculate_ram_utilization(resources);

    // Weighted average (CPU 50%, RAM 50%)
    return (cpu_util + ram_util) / 2.0;
}

// Bin-packing scoring (prefer fuller nodes)
int score_node_binpack(Node *node, NodeResources *resources, const ResourceRequest *request) {
    if (!node || !resources || !request) return 0;

    // Higher score for nodes that will be more fully utilized after placement
    double current_util = calculate_overall_utilization(resources);

    // Simulate allocation
    double cpu_after = (resources->cpu_allocated + request->cpu_cores) / resources->cpu_total * 100.0;
    double ram_after = (resources->ram_allocated_mb + request->ram_mb) / (double)resources->ram_total_mb * 100.0;
    double util_after = (cpu_after + ram_after) / 2.0;

    // Score is the utilization after placement (0-100)
    return (int)util_after;
}

// Spread scoring (prefer emptier nodes)
int score_node_spread(Node *node, NodeResources *resources, const ResourceRequest *request) {
    if (!node || !resources || !request) return 0;

    // Higher score for nodes with lower utilization
    double util = calculate_overall_utilization(resources);

    // Invert the score (lower utilization = higher score)
    return (int)(100.0 - util);
}

// Overall scoring
int score_node_overall(Node *node, NodeResources *resources,
                       const ResourceRequest *request,
                       const PlacementPreferences *prefs) {
    if (!node || !resources || !request) return 0;

    int score = 0;

    // Base score based on strategy
    if (!prefs || prefs->strategy == SCHED_STRATEGY_BINPACK) {
        score = score_node_binpack(node, resources, request);
    } else if (prefs->strategy == SCHED_STRATEGY_SPREAD) {
        score = score_node_spread(node, resources, request);
    } else if (prefs->strategy == SCHED_STRATEGY_RANDOM) {
        score = rand() % 100;
    }

    // TODO: Add constraint scoring
    // TODO: Add affinity scoring

    return score;
}

// Main scheduling function
SchedulingDecision *scheduler_schedule(
    const ResourceRequest *request,
    const PlacementPreferences *preferences) {

    if (!g_scheduler || !request) return NULL;

    pthread_mutex_lock(&g_scheduler_mutex);

    SchedulingDecision *decision = calloc(1, sizeof(SchedulingDecision));
    if (!decision) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return NULL;
    }

    decision->selected_node = NULL;
    decision->score = -1;
    decision->success = false;

    // Get all cluster nodes
    int node_count = 0;
    Node **nodes = cluster_get_nodes(&node_count);

    if (node_count == 0) {
        snprintf(decision->error, sizeof(decision->error),
                 "No nodes available in cluster");
        g_scheduler->total_failed++;
        pthread_mutex_unlock(&g_scheduler_mutex);
        return decision;
    }

    // Score each node
    int best_score = -1;
    Node *best_node = NULL;

    for (int i = 0; i < node_count; i++) {
        Node *node = nodes[i];

        // Skip dead/suspect nodes
        if (node->state != NODE_STATE_ALIVE) {
            continue;
        }

        // Get node resources
        NodeResources *res = scheduler_get_node_resources(node->id);
        if (!res) {
            continue;
        }

        // Check if node has resources
        if (!node_has_resources(res, request)) {
            continue;
        }

        // Score the node
        int score = score_node_overall(node, res, request, preferences);

        if (score > best_score) {
            best_score = score;
            best_node = node;
        }
    }

    if (best_node) {
        decision->selected_node = best_node;
        decision->score = best_score;
        decision->success = true;
        snprintf(decision->reason, sizeof(decision->reason),
                 "Selected node %s (score: %d)", best_node->name, best_score);

        g_scheduler->total_scheduled++;
    } else {
        snprintf(decision->error, sizeof(decision->error),
                 "No suitable node found for placement");
        g_scheduler->total_failed++;
    }

    pthread_mutex_unlock(&g_scheduler_mutex);
    return decision;
}

void scheduler_decision_free(SchedulingDecision *decision) {
    if (decision) {
        free(decision);
    }
}

/*
 * ============================================================================
 * QUOTA MANAGEMENT
 * ============================================================================
 */

int scheduler_create_quota(const char *namespace_name, ResourceQuota *quota) {
    if (!g_scheduler || !namespace_name || !quota) return -1;

    pthread_mutex_lock(&g_scheduler_mutex);

    // Check if quota already exists
    for (int i = 0; i < g_scheduler->quota_count; i++) {
        if (strcmp(g_scheduler->quotas[i]->namespace_name, namespace_name) == 0) {
            pthread_mutex_unlock(&g_scheduler_mutex);
            return 1; // Already exists
        }
    }

    // Create new quota
    ResourceQuota *new_quota = calloc(1, sizeof(ResourceQuota));
    if (!new_quota) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return -1;
    }

    memcpy(new_quota, quota, sizeof(ResourceQuota));
    strncpy(new_quota->namespace_name, namespace_name, sizeof(new_quota->namespace_name) - 1);
    new_quota->created_at = time(NULL);
    new_quota->updated_at = time(NULL);

    // Add to scheduler
    g_scheduler->quotas = realloc(
        g_scheduler->quotas,
        sizeof(ResourceQuota*) * (g_scheduler->quota_count + 1)
    );

    g_scheduler->quotas[g_scheduler->quota_count] = new_quota;
    g_scheduler->quota_count++;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return 0;
}

ResourceQuota *scheduler_get_quota(const char *namespace_name) {
    if (!g_scheduler || !namespace_name) return NULL;

    pthread_mutex_lock(&g_scheduler_mutex);

    for (int i = 0; i < g_scheduler->quota_count; i++) {
        if (strcmp(g_scheduler->quotas[i]->namespace_name, namespace_name) == 0) {
            pthread_mutex_unlock(&g_scheduler_mutex);
            return g_scheduler->quotas[i];
        }
    }

    pthread_mutex_unlock(&g_scheduler_mutex);
    return NULL;
}

bool scheduler_check_quota(const char *namespace_name, const ResourceRequest *request) {
    if (!namespace_name || !request) return false;

    ResourceQuota *quota = scheduler_get_quota(namespace_name);
    if (!quota) {
        return true; // No quota = unlimited
    }

    pthread_mutex_lock(&g_scheduler_mutex);

    // Check hard limits
    bool within_limits = (
        quota->used_cpu_cores + request->cpu_cores <= quota->max_cpu_cores &&
        quota->used_ram_mb + request->ram_mb <= quota->max_ram_mb &&
        quota->used_disk_mb + request->disk_mb <= quota->max_disk_mb &&
        quota->used_instances + 1 <= quota->max_instances
    );

    pthread_mutex_unlock(&g_scheduler_mutex);
    return within_limits;
}

/*
 * ============================================================================
 * RESOURCE RESERVATION
 * ============================================================================
 */

ResourceReservation *scheduler_reserve_resources(
    const char *namespace_name,
    const ResourceRequest *request,
    int duration_seconds,
    int priority) {

    if (!g_scheduler || !namespace_name || !request) return NULL;

    pthread_mutex_lock(&g_scheduler_mutex);

    // Check quota
    if (!scheduler_check_quota(namespace_name, request)) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return NULL;
    }

    // Create reservation
    ResourceReservation *reservation = calloc(1, sizeof(ResourceReservation));
    if (!reservation) {
        pthread_mutex_unlock(&g_scheduler_mutex);
        return NULL;
    }

    generate_id(reservation->id, sizeof(reservation->id));
    strncpy(reservation->namespace_name, namespace_name, sizeof(reservation->namespace_name) - 1);
    memcpy(&reservation->resources, request, sizeof(ResourceRequest));
    reservation->reserved_on_node = NULL; // Any node
    reservation->created_at = time(NULL);
    reservation->expires_at = time(NULL) + duration_seconds;
    reservation->priority = priority;
    reservation->in_use = false;

    // Add to scheduler
    g_scheduler->reservations = realloc(
        g_scheduler->reservations,
        sizeof(ResourceReservation*) * (g_scheduler->reservation_count + 1)
    );

    g_scheduler->reservations[g_scheduler->reservation_count] = reservation;
    g_scheduler->reservation_count++;

    pthread_mutex_unlock(&g_scheduler_mutex);
    return reservation;
}

int scheduler_release_reservation(const char *reservation_id) {
    if (!g_scheduler || !reservation_id) return -1;

    pthread_mutex_lock(&g_scheduler_mutex);

    for (int i = 0; i < g_scheduler->reservation_count; i++) {
        if (strcmp(g_scheduler->reservations[i]->id, reservation_id) == 0) {
            free(g_scheduler->reservations[i]);

            // Shift array
            for (int j = i; j < g_scheduler->reservation_count - 1; j++) {
                g_scheduler->reservations[j] = g_scheduler->reservations[j + 1];
            }

            g_scheduler->reservation_count--;
            pthread_mutex_unlock(&g_scheduler_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_scheduler_mutex);
    return -1;
}

/*
 * ============================================================================
 * PLACEMENT CONSTRAINTS AND AFFINITY
 * ============================================================================
 */

PlacementConstraint *placement_constraint_create(
    const char *key,
    ConstraintOperator op,
    const char **values,
    int value_count,
    ConstraintType type) {

    if (!key) return NULL;

    PlacementConstraint *constraint = calloc(1, sizeof(PlacementConstraint));
    if (!constraint) return NULL;

    strncpy(constraint->key, key, sizeof(constraint->key) - 1);
    constraint->op = op;
    constraint->type = type;
    constraint->weight = (type == CONSTRAINT_REQUIRED) ? 100 : 50;

    if (values && value_count > 0) {
        constraint->values = calloc(value_count, sizeof(char*));
        for (int i = 0; i < value_count; i++) {
            constraint->values[i] = strdup(values[i]);
        }
        constraint->value_count = value_count;
    } else {
        constraint->values = NULL;
        constraint->value_count = 0;
    }

    return constraint;
}

void placement_constraint_free(PlacementConstraint *constraint) {
    if (!constraint) return;

    if (constraint->values) {
        for (int i = 0; i < constraint->value_count; i++) {
            free(constraint->values[i]);
        }
        free(constraint->values);
    }

    free(constraint);
}

AffinityRule *affinity_rule_create(
    const char *service_name,
    bool anti_affinity,
    int weight) {

    if (!service_name) return NULL;

    AffinityRule *rule = calloc(1, sizeof(AffinityRule));
    if (!rule) return NULL;

    strncpy(rule->service_name, service_name, sizeof(rule->service_name) - 1);
    rule->anti_affinity = anti_affinity;
    rule->weight = weight;

    return rule;
}

void affinity_rule_free(AffinityRule *rule) {
    if (rule) {
        free(rule);
    }
}

PlacementPreferences *placement_preferences_create(SchedulingStrategy strategy) {
    PlacementPreferences *prefs = calloc(1, sizeof(PlacementPreferences));
    if (!prefs) return NULL;

    prefs->strategy = strategy;
    prefs->constraints = NULL;
    prefs->constraint_count = 0;
    prefs->affinity_rules = NULL;
    prefs->affinity_rule_count = 0;
    prefs->tolerations = NULL;
    prefs->toleration_count = 0;

    return prefs;
}

void placement_preferences_add_constraint(
    PlacementPreferences *prefs,
    PlacementConstraint *constraint) {

    if (!prefs || !constraint) return;

    prefs->constraints = realloc(
        prefs->constraints,
        sizeof(PlacementConstraint*) * (prefs->constraint_count + 1)
    );

    prefs->constraints[prefs->constraint_count] = constraint;
    prefs->constraint_count++;
}

void placement_preferences_add_affinity(
    PlacementPreferences *prefs,
    AffinityRule *rule) {

    if (!prefs || !rule) return;

    prefs->affinity_rules = realloc(
        prefs->affinity_rules,
        sizeof(AffinityRule*) * (prefs->affinity_rule_count + 1)
    );

    prefs->affinity_rules[prefs->affinity_rule_count] = rule;
    prefs->affinity_rule_count++;
}

void placement_preferences_free(PlacementPreferences *prefs) {
    if (!prefs) return;

    // Free constraints
    for (int i = 0; i < prefs->constraint_count; i++) {
        placement_constraint_free(prefs->constraints[i]);
    }
    free(prefs->constraints);

    // Free affinity rules
    for (int i = 0; i < prefs->affinity_rule_count; i++) {
        affinity_rule_free(prefs->affinity_rules[i]);
    }
    free(prefs->affinity_rules);

    // Free tolerations
    for (int i = 0; i < prefs->toleration_count; i++) {
        free(prefs->tolerations[i]);
    }
    free(prefs->tolerations);

    free(prefs);
}

/*
 * ============================================================================
 * NODE LABEL MANAGEMENT
 * ============================================================================
 */

int node_add_label(NodeResources *resources, const char *key, const char *value) {
    if (!resources || !key || !value) return -1;

    char *label = calloc(256, sizeof(char));
    snprintf(label, 256, "%s=%s", key, value);

    resources->labels = realloc(
        resources->labels,
        sizeof(char*) * (resources->label_count + 1)
    );

    resources->labels[resources->label_count] = label;
    resources->label_count++;

    return 0;
}

const char *node_get_label(NodeResources *resources, const char *key) {
    if (!resources || !key) return NULL;

    for (int i = 0; i < resources->label_count; i++) {
        if (strncmp(resources->labels[i], key, strlen(key)) == 0 &&
            resources->labels[i][strlen(key)] == '=') {
            return resources->labels[i] + strlen(key) + 1;
        }
    }

    return NULL;
}

bool node_has_label(NodeResources *resources, const char *key, const char *value) {
    const char *label_value = node_get_label(resources, key);
    if (!label_value) return false;
    return strcmp(label_value, value) == 0;
}

/*
 * ============================================================================
 * NODE TAINT MANAGEMENT
 * ============================================================================
 */

int node_add_taint(NodeResources *resources, const char *taint) {
    if (!resources || !taint) return -1;

    resources->taints = realloc(
        resources->taints,
        sizeof(char*) * (resources->taint_count + 1)
    );

    resources->taints[resources->taint_count] = strdup(taint);
    resources->taint_count++;

    return 0;
}

bool node_has_taint(NodeResources *resources, const char *taint) {
    if (!resources || !taint) return false;

    for (int i = 0; i < resources->taint_count; i++) {
        if (strcmp(resources->taints[i], taint) == 0) {
            return true;
        }
    }

    return false;
}
