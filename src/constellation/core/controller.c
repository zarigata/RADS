/*
 * RADS Constellation - Core Controller
 *
 * Main controller for managing instances across the cluster
 */

#include "../constellation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#define CONSTELLATION_VERSION "0.0.4-alpha"
#define DEFAULT_MAX_INSTANCES 1000

// Global controller state
typedef struct {
    bool initialized;
    ControllerConfig config;
    Instance **instances;
    int instance_count;
    int instance_capacity;
    unsigned long id_counter;
} Controller;

static Controller g_controller = {0};

/*
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

// Generate unique instance ID (simple counter-based for now)
static void generate_instance_id(char *id_out, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long timestamp = tv.tv_sec * 1000000 + tv.tv_usec;
    g_controller.id_counter++;
    snprintf(id_out, size, "inst-%lu-%lu", timestamp, g_controller.id_counter);
}

// Find instance by ID
static int find_instance_index(const char *instance_id) {
    for (int i = 0; i < g_controller.instance_count; i++) {
        if (strcmp(g_controller.instances[i]->id, instance_id) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * ============================================================================
 * CONTROLLER LIFECYCLE
 * ============================================================================
 */

int constellation_init(ControllerConfig *config) {
    if (g_controller.initialized) {
        fprintf(stderr, "[CONSTELLATION] Already initialized\n");
        return -1;
    }

    printf("\033[1;36m╔════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║  🌟 RADS CONSTELLATION v%s              ║\033[0m\n", CONSTELLATION_VERSION);
    printf("\033[1;36m║  Distributed Computing Platform                ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════╝\033[0m\n\n");

    // Copy configuration (or use defaults)
    if (config) {
        memcpy(&g_controller.config, config, sizeof(ControllerConfig));
    } else {
        // Default configuration
        snprintf(g_controller.config.node_name, sizeof(g_controller.config.node_name),
                 "node-1");
        snprintf(g_controller.config.cluster_name, sizeof(g_controller.config.cluster_name),
                 "default");
        g_controller.config.control_port = 7946;
        g_controller.config.data_port = 7947;
        g_controller.config.max_instances = DEFAULT_MAX_INSTANCES;
        g_controller.config.enable_clustering = false;
    }

    // Allocate instance array
    g_controller.instance_capacity = g_controller.config.max_instances;
    g_controller.instances = calloc(g_controller.instance_capacity, sizeof(Instance*));
    if (!g_controller.instances) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to allocate instance array\n");
        return -1;
    }

    g_controller.instance_count = 0;
    g_controller.initialized = true;

    printf("\033[1;32m[INIT]\033[0m Controller initialized\n");
    printf("  Node: %s\n", g_controller.config.node_name);
    printf("  Cluster: %s\n", g_controller.config.cluster_name);
    printf("  Max Instances: %u\n", g_controller.config.max_instances);
    printf("  Clustering: %s\n\n",
           g_controller.config.enable_clustering ? "enabled" : "disabled");

    return 0;
}

int constellation_shutdown(void) {
    if (!g_controller.initialized) {
        return -1;
    }

    printf("\033[1;33m[SHUTDOWN]\033[0m Stopping all instances...\n");

    // Stop all running instances
    for (int i = 0; i < g_controller.instance_count; i++) {
        Instance *inst = g_controller.instances[i];
        if (inst && inst->state == INSTANCE_STATE_RUNNING) {
            constellation_instance_stop(inst);
        }
    }

    // Clean up instances
    for (int i = 0; i < g_controller.instance_count; i++) {
        if (g_controller.instances[i]) {
            constellation_instance_destroy(g_controller.instances[i]);
        }
    }

    free(g_controller.instances);
    g_controller.instances = NULL;
    g_controller.instance_count = 0;
    g_controller.initialized = false;

    printf("\033[1;32m[SHUTDOWN]\033[0m Controller stopped\n\n");
    return 0;
}

bool constellation_is_running(void) {
    return g_controller.initialized;
}

/*
 * ============================================================================
 * INSTANCE MANAGEMENT
 * ============================================================================
 */

ConstellationError constellation_instance_create(
    const InstanceConfig *config,
    Instance **out_instance
) {
    if (!g_controller.initialized) {
        return CONSTELLATION_ERR_PERMISSION_DENIED;
    }

    if (!config || !out_instance) {
        return CONSTELLATION_ERR_INVALID_PARAM;
    }

    // Check capacity
    if (g_controller.instance_count >= g_controller.instance_capacity) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Maximum instances reached\n");
        return CONSTELLATION_ERR_RESOURCE_EXHAUSTED;
    }

    // Allocate instance
    Instance *instance = calloc(1, sizeof(Instance));
    if (!instance) {
        return CONSTELLATION_ERR_NO_MEMORY;
    }

    // Generate ID
    generate_instance_id(instance->id, sizeof(instance->id));

    // Copy configuration
    strncpy(instance->name, config->name, sizeof(instance->name) - 1);
    memcpy(&instance->limits, &config->limits, sizeof(ResourceLimits));

    // Initialize state
    instance->state = INSTANCE_STATE_PENDING;
    instance->created_at = time(NULL);
    instance->pid = 0;

    // Add to registry
    g_controller.instances[g_controller.instance_count++] = instance;

    printf("\033[1;34m[CREATE]\033[0m Instance created: %s\n", instance->name);
    printf("  ID: %s\n", instance->id);
    printf("  Limits: CPU=%u%%, RAM=%luMB\n",
           instance->limits.cpu_percent,
           instance->limits.ram_mb);

    *out_instance = instance;
    return CONSTELLATION_OK;
}

ConstellationError constellation_instance_start(Instance *instance) {
    if (!instance) {
        return CONSTELLATION_ERR_INVALID_PARAM;
    }

    if (instance->state != INSTANCE_STATE_PENDING) {
        fprintf(stderr, "\033[1;33m[WARN]\033[0m Instance not in PENDING state\n");
        return CONSTELLATION_ERR_INVALID_PARAM;
    }

    printf("\033[1;34m[START]\033[0m Starting instance: %s\n", instance->name);

    instance->state = INSTANCE_STATE_STARTING;

    // TODO: Implement actual spawning with isolation
    // For now, just mark as running
    instance->state = INSTANCE_STATE_RUNNING;
    instance->started_at = time(NULL);

    printf("\033[1;32m[START]\033[0m Instance running: %s (PID: %d)\n",
           instance->name, instance->pid);

    return CONSTELLATION_OK;
}

ConstellationError constellation_instance_stop(Instance *instance) {
    if (!instance) {
        return CONSTELLATION_ERR_INVALID_PARAM;
    }

    if (instance->state != INSTANCE_STATE_RUNNING) {
        return CONSTELLATION_OK; // Already stopped
    }

    printf("\033[1;33m[STOP]\033[0m Stopping instance: %s\n", instance->name);

    instance->state = INSTANCE_STATE_STOPPING;

    // TODO: Implement actual stopping with signals
    instance->state = INSTANCE_STATE_STOPPED;
    instance->stopped_at = time(NULL);

    printf("\033[1;32m[STOP]\033[0m Instance stopped: %s\n", instance->name);

    return CONSTELLATION_OK;
}

ConstellationError constellation_instance_destroy(Instance *instance) {
    if (!instance) {
        return CONSTELLATION_ERR_INVALID_PARAM;
    }

    // Stop if running
    if (instance->state == INSTANCE_STATE_RUNNING) {
        constellation_instance_stop(instance);
    }

    // Remove from registry
    int index = find_instance_index(instance->id);
    if (index >= 0) {
        // Shift remaining instances
        for (int i = index; i < g_controller.instance_count - 1; i++) {
            g_controller.instances[i] = g_controller.instances[i + 1];
        }
        g_controller.instance_count--;
    }

    printf("\033[1;31m[DESTROY]\033[0m Instance destroyed: %s\n", instance->name);

    free(instance);
    return CONSTELLATION_OK;
}

/*
 * ============================================================================
 * INSTANCE QUERY
 * ============================================================================
 */

Instance *constellation_instance_get(const char *instance_id) {
    int index = find_instance_index(instance_id);
    if (index < 0) {
        return NULL;
    }
    return g_controller.instances[index];
}

Instance **constellation_instance_list(int *out_count) {
    if (out_count) {
        *out_count = g_controller.instance_count;
    }
    return g_controller.instances;
}

InstanceState constellation_instance_state(Instance *instance) {
    if (!instance) {
        return INSTANCE_STATE_UNKNOWN;
    }
    return instance->state;
}

/*
 * ============================================================================
 * ERROR HANDLING
 * ============================================================================
 */

const char *constellation_error_string(ConstellationError error) {
    switch (error) {
        case CONSTELLATION_OK: return "Success";
        case CONSTELLATION_ERR_INVALID_PARAM: return "Invalid parameter";
        case CONSTELLATION_ERR_NO_MEMORY: return "Out of memory";
        case CONSTELLATION_ERR_SPAWN_FAILED: return "Failed to spawn process";
        case CONSTELLATION_ERR_NOT_FOUND: return "Not found";
        case CONSTELLATION_ERR_ALREADY_EXISTS: return "Already exists";
        case CONSTELLATION_ERR_PERMISSION_DENIED: return "Permission denied";
        case CONSTELLATION_ERR_RESOURCE_EXHAUSTED: return "Resource exhausted";
        case CONSTELLATION_ERR_ISOLATION_FAILED: return "Isolation failed";
        case CONSTELLATION_ERR_CGROUP_FAILED: return "Cgroup operation failed";
        case CONSTELLATION_ERR_NAMESPACE_FAILED: return "Namespace operation failed";
        default: return "Unknown error";
    }
}
