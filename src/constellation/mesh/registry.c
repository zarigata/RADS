/*
 * RADS Constellation - Service Registry Implementation
 * Phase 5: Service Mesh
 */

#define _GNU_SOURCE

#include "registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static ServiceEndpoint **g_services = NULL;
static int g_service_count = 0;
static RegistryConfig g_config;
static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_health_thread;
static pthread_t g_cleanup_thread;
static bool g_initialized = false;
static bool g_running = false;
static uint64_t g_registration_counter = 0;
static uint64_t g_deregistration_counter = 0;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static void generate_service_id(char *out_id, size_t size, const char *service_name) {
    snprintf(out_id, size, "%s-%lu-%d", service_name, time(NULL), rand() % 10000);
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int registry_init(RegistryConfig *config) {
    pthread_mutex_lock(&g_registry_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_registry_mutex);
        return 1;
    }

    // Set configuration
    if (config) {
        g_config = *config;
    } else {
        g_config.max_services = 1000;
        g_config.heartbeat_interval_ms = 5000;
        g_config.heartbeat_timeout_ms = 15000;
        g_config.enable_health_checks = true;
        g_config.health_check_interval_ms = 10000;
    }

    g_services = NULL;
    g_service_count = 0;
    g_initialized = true;
    g_running = true;
    g_registration_counter = 0;
    g_deregistration_counter = 0;

    // Start background threads
    pthread_create(&g_health_thread, NULL, registry_health_check_thread, NULL);
    pthread_create(&g_cleanup_thread, NULL, registry_cleanup_thread, NULL);

    pthread_mutex_unlock(&g_registry_mutex);
    return 0;
}

int registry_shutdown(void) {
    pthread_mutex_lock(&g_registry_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_registry_mutex);
        return -1;
    }

    g_running = false;
    g_initialized = false;

    pthread_mutex_unlock(&g_registry_mutex);

    // Wait for threads
    pthread_join(g_health_thread, NULL);
    pthread_join(g_cleanup_thread, NULL);

    pthread_mutex_lock(&g_registry_mutex);

    // Free all services
    for (int i = 0; i < g_service_count; i++) {
        free(g_services[i]);
    }
    free(g_services);
    g_services = NULL;
    g_service_count = 0;

    pthread_mutex_unlock(&g_registry_mutex);
    return 0;
}

bool registry_is_running(void) {
    pthread_mutex_lock(&g_registry_mutex);
    bool running = g_initialized && g_running;
    pthread_mutex_unlock(&g_registry_mutex);
    return running;
}

/*
 * ============================================================================
 * SERVICE REGISTRATION
 * ============================================================================
 */

ServiceEndpoint *registry_register(
    const char *service_name,
    const char *endpoint,
    const char **tags,
    int tag_count
) {
    if (!g_initialized || !service_name || !endpoint) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    // Create service endpoint
    ServiceEndpoint *service = calloc(1, sizeof(ServiceEndpoint));
    if (!service) {
        pthread_mutex_unlock(&g_registry_mutex);
        return NULL;
    }

    generate_service_id(service->service_id, sizeof(service->service_id), service_name);
    strncpy(service->service_name, service_name, sizeof(service->service_name) - 1);
    strncpy(service->endpoint, endpoint, sizeof(service->endpoint) - 1);

    // Add tags
    if (tags && tag_count > 0) {
        service->tag_count = tag_count < MAX_SERVICE_TAGS ? tag_count : MAX_SERVICE_TAGS;
        for (int i = 0; i < service->tag_count; i++) {
            strncpy(service->tags[i], tags[i], MAX_TAG_LENGTH - 1);
        }
    }

    service->health = SERVICE_HEALTH_HEALTHY;
    service->last_heartbeat = time(NULL);
    service->registered_at = time(NULL);
    service->active = true;
    service->node_id = -1;  // TODO: Get from cluster

    // Add to registry
    g_services = realloc(g_services, sizeof(ServiceEndpoint *) * (g_service_count + 1));
    g_services[g_service_count] = service;
    g_service_count++;
    g_registration_counter++;

    pthread_mutex_unlock(&g_registry_mutex);
    return service;
}

int registry_deregister(const char *service_id) {
    if (!g_initialized || !service_id) return -1;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            free(g_services[i]);

            // Shift array
            for (int j = i; j < g_service_count - 1; j++) {
                g_services[j] = g_services[j + 1];
            }

            g_service_count--;
            g_deregistration_counter++;

            pthread_mutex_unlock(&g_registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return -1;
}

int registry_set_metadata(const char *service_id, const char *key, const char *value) {
    if (!service_id || !key || !value) return -1;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            ServiceEndpoint *service = g_services[i];

            // Find or add metadata entry
            int idx = -1;
            for (int j = 0; j < service->metadata_count; j++) {
                if (strcmp(service->metadata[j].key, key) == 0) {
                    idx = j;
                    break;
                }
            }

            if (idx == -1 && service->metadata_count < MAX_METADATA_ENTRIES) {
                idx = service->metadata_count++;
            }

            if (idx != -1) {
                strncpy(service->metadata[idx].key, key, MAX_METADATA_KEY - 1);
                strncpy(service->metadata[idx].value, value, MAX_METADATA_VALUE - 1);
            }

            pthread_mutex_unlock(&g_registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return -1;
}

int registry_set_health(const char *service_id, ServiceHealth health) {
    if (!service_id) return -1;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            g_services[i]->health = health;
            pthread_mutex_unlock(&g_registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return -1;
}

int registry_heartbeat(const char *service_id) {
    if (!service_id) return -1;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            g_services[i]->last_heartbeat = time(NULL);
            pthread_mutex_unlock(&g_registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return -1;
}

/*
 * ============================================================================
 * SERVICE DISCOVERY
 * ============================================================================
 */

ServiceEndpoint **registry_find_by_name(const char *service_name, int *count) {
    if (!g_initialized || !service_name || !count) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    ServiceEndpoint **results = NULL;
    *count = 0;

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_name, service_name) == 0 && g_services[i]->active) {
            results = realloc(results, sizeof(ServiceEndpoint *) * (*count + 1));
            results[*count] = g_services[i];
            (*count)++;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return results;
}

ServiceEndpoint **registry_find_by_tag(const char *tag, int *count) {
    if (!g_initialized || !tag || !count) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    ServiceEndpoint **results = NULL;
    *count = 0;

    for (int i = 0; i < g_service_count; i++) {
        if (!g_services[i]->active) continue;

        for (int j = 0; j < g_services[i]->tag_count; j++) {
            if (strcmp(g_services[i]->tags[j], tag) == 0) {
                results = realloc(results, sizeof(ServiceEndpoint *) * (*count + 1));
                results[*count] = g_services[i];
                (*count)++;
                break;
            }
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return results;
}

ServiceEndpoint **registry_find(ServiceQuery *query, int *count) {
    if (!g_initialized || !query || !count) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    ServiceEndpoint **results = NULL;
    *count = 0;

    for (int i = 0; i < g_service_count; i++) {
        ServiceEndpoint *service = g_services[i];
        bool matches = true;

        // Check active filter
        if (query->only_active && !service->active) {
            matches = false;
        }

        // Check service name
        if (matches && query->service_name[0] != '\0') {
            if (strcmp(service->service_name, query->service_name) != 0) {
                matches = false;
            }
        }

        // Check tag
        if (matches && query->tag[0] != '\0') {
            bool has_tag = false;
            for (int j = 0; j < service->tag_count; j++) {
                if (strcmp(service->tags[j], query->tag) == 0) {
                    has_tag = true;
                    break;
                }
            }
            if (!has_tag) matches = false;
        }

        // Check health
        if (matches && service->health < query->min_health) {
            matches = false;
        }

        // Check node
        if (matches && query->node_id >= 0 && service->node_id != query->node_id) {
            matches = false;
        }

        if (matches) {
            results = realloc(results, sizeof(ServiceEndpoint *) * (*count + 1));
            results[*count] = service;
            (*count)++;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return results;
}

ServiceEndpoint *registry_get(const char *service_id) {
    if (!g_initialized || !service_id) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            ServiceEndpoint *service = g_services[i];
            pthread_mutex_unlock(&g_registry_mutex);
            return service;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return NULL;
}

ServiceEndpoint **registry_list_all(int *count) {
    if (!g_initialized || !count) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    ServiceEndpoint **results = NULL;
    *count = 0;

    for (int i = 0; i < g_service_count; i++) {
        results = realloc(results, sizeof(ServiceEndpoint *) * (*count + 1));
        results[*count] = g_services[i];
        (*count)++;
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return results;
}

/*
 * ============================================================================
 * HEALTH MONITORING
 * ============================================================================
 */

ServiceHealth registry_check_health(const char *service_id) {
    if (!service_id) return SERVICE_HEALTH_UNKNOWN;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            ServiceHealth health = g_services[i]->health;
            pthread_mutex_unlock(&g_registry_mutex);
            return health;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return SERVICE_HEALTH_UNKNOWN;
}

int registry_mark_healthy(const char *service_id) {
    return registry_set_health(service_id, SERVICE_HEALTH_HEALTHY);
}

int registry_mark_unhealthy(const char *service_id) {
    return registry_set_health(service_id, SERVICE_HEALTH_UNHEALTHY);
}

int registry_update_metrics(
    const char *service_id,
    uint64_t requests,
    uint64_t errors,
    double avg_response_ms
) {
    if (!service_id) return -1;

    pthread_mutex_lock(&g_registry_mutex);

    for (int i = 0; i < g_service_count; i++) {
        if (strcmp(g_services[i]->service_id, service_id) == 0) {
            g_services[i]->request_count = requests;
            g_services[i]->error_count = errors;
            g_services[i]->avg_response_time_ms = avg_response_ms;
            pthread_mutex_unlock(&g_registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return -1;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

RegistryStats registry_get_stats(void) {
    RegistryStats stats = {0};

    if (!g_initialized) return stats;

    pthread_mutex_lock(&g_registry_mutex);

    stats.total_services = g_service_count;
    stats.total_registrations = g_registration_counter;
    stats.total_deregistrations = g_deregistration_counter;

    for (int i = 0; i < g_service_count; i++) {
        ServiceEndpoint *service = g_services[i];

        if (!service->active) {
            stats.inactive_services++;
            continue;
        }

        switch (service->health) {
            case SERVICE_HEALTH_HEALTHY:
                stats.healthy_services++;
                break;
            case SERVICE_HEALTH_DEGRADED:
                stats.degraded_services++;
                break;
            case SERVICE_HEALTH_UNHEALTHY:
                stats.unhealthy_services++;
                break;
            default:
                break;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);
    return stats;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *registry_health_check_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks for quick exit
        for (int i = 0; i < g_config.health_check_interval_ms / 100 && g_running; i++) {
            usleep(100000);  // 100ms
        }

        if (!g_running || !g_config.enable_health_checks) continue;

        pthread_mutex_lock(&g_registry_mutex);

        time_t now = time(NULL);
        int timeout_sec = g_config.heartbeat_timeout_ms / 1000;

        for (int i = 0; i < g_service_count; i++) {
            ServiceEndpoint *service = g_services[i];

            if (!service->active) continue;

            // Check heartbeat timeout
            if (now - service->last_heartbeat > timeout_sec) {
                service->health = SERVICE_HEALTH_UNHEALTHY;
                service->active = false;
            }
        }

        pthread_mutex_unlock(&g_registry_mutex);
    }

    return NULL;
}

void *registry_cleanup_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks for quick exit
        for (int i = 0; i < 30000 / 100 && g_running; i++) {  // 30 sec
            usleep(100000);  // 100ms
        }

        if (!g_running) continue;

        pthread_mutex_lock(&g_registry_mutex);

        time_t now = time(NULL);

        // Remove inactive services older than 5 minutes
        for (int i = 0; i < g_service_count; ) {
            ServiceEndpoint *service = g_services[i];

            if (!service->active && (now - service->last_heartbeat > 300)) {
                free(service);

                for (int j = i; j < g_service_count - 1; j++) {
                    g_services[j] = g_services[j + 1];
                }

                g_service_count--;
            } else {
                i++;
            }
        }

        pthread_mutex_unlock(&g_registry_mutex);
    }

    return NULL;
}
