/*
 * RADS Constellation - Load Balancer Implementation
 * Phase 5: Service Mesh
 */

#define _GNU_SOURCE

#include "loadbalancer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static LoadBalancer **g_balancers = NULL;
static int g_balancer_count = 0;
static pthread_mutex_t g_lb_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_initialized = false;
static uint64_t g_total_requests = 0;
static uint64_t g_successful_requests = 0;
static uint64_t g_failed_requests = 0;
static uint64_t g_retried_requests = 0;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static uint64_t hash_string(const char *str) {
    uint64_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int lb_init(void) {
    pthread_mutex_lock(&g_lb_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_lb_mutex);
        return 1;
    }

    g_balancers = NULL;
    g_balancer_count = 0;
    g_initialized = true;
    g_total_requests = 0;
    g_successful_requests = 0;
    g_failed_requests = 0;
    g_retried_requests = 0;

    pthread_mutex_unlock(&g_lb_mutex);
    return 0;
}

int lb_shutdown(void) {
    pthread_mutex_lock(&g_lb_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_lb_mutex);
        return -1;
    }

    // Free all load balancers
    for (int i = 0; i < g_balancer_count; i++) {
        if (g_balancers[i]->endpoint_counters) {
            free(g_balancers[i]->endpoint_counters);
        }
        free(g_balancers[i]);
    }
    free(g_balancers);

    g_balancers = NULL;
    g_balancer_count = 0;
    g_initialized = false;

    pthread_mutex_unlock(&g_lb_mutex);
    return 0;
}

bool lb_is_running(void) {
    pthread_mutex_lock(&g_lb_mutex);
    bool running = g_initialized;
    pthread_mutex_unlock(&g_lb_mutex);
    return running;
}

/*
 * ============================================================================
 * LOAD BALANCER MANAGEMENT
 * ============================================================================
 */

LoadBalancer *lb_create(const char *service_name, LBConfig *config) {
    if (!g_initialized || !service_name) return NULL;

    pthread_mutex_lock(&g_lb_mutex);

    // Check if already exists
    for (int i = 0; i < g_balancer_count; i++) {
        if (strcmp(g_balancers[i]->name, service_name) == 0) {
            pthread_mutex_unlock(&g_lb_mutex);
            return g_balancers[i];
        }
    }

    // Create new load balancer
    LoadBalancer *lb = calloc(1, sizeof(LoadBalancer));
    if (!lb) {
        pthread_mutex_unlock(&g_lb_mutex);
        return NULL;
    }

    strncpy(lb->name, service_name, sizeof(lb->name) - 1);

    if (config) {
        lb->config = *config;
    } else {
        lb->config.algorithm = LB_ALGORITHM_ROUND_ROBIN;
        lb->config.health_check_enabled = true;
        lb->config.max_retries = 3;
        lb->config.retry_delay_ms = 100;
        lb->config.sticky_sessions = false;
        lb->config.session_timeout_ms = 300000;  // 5 minutes
    }

    lb->endpoint_counters = NULL;
    lb->endpoint_count = 0;
    lb->created_at = time(NULL);
    lb->total_requests = 0;
    lb->failed_requests = 0;

    // Add to list
    g_balancers = realloc(g_balancers, sizeof(LoadBalancer *) * (g_balancer_count + 1));
    g_balancers[g_balancer_count] = lb;
    g_balancer_count++;

    pthread_mutex_unlock(&g_lb_mutex);
    return lb;
}

int lb_destroy(const char *service_name) {
    if (!g_initialized || !service_name) return -1;

    pthread_mutex_lock(&g_lb_mutex);

    for (int i = 0; i < g_balancer_count; i++) {
        if (strcmp(g_balancers[i]->name, service_name) == 0) {
            if (g_balancers[i]->endpoint_counters) {
                free(g_balancers[i]->endpoint_counters);
            }
            free(g_balancers[i]);

            // Shift array
            for (int j = i; j < g_balancer_count - 1; j++) {
                g_balancers[j] = g_balancers[j + 1];
            }

            g_balancer_count--;

            pthread_mutex_unlock(&g_lb_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_lb_mutex);
    return -1;
}

LoadBalancer *lb_get(const char *service_name) {
    if (!g_initialized || !service_name) return NULL;

    pthread_mutex_lock(&g_lb_mutex);

    for (int i = 0; i < g_balancer_count; i++) {
        if (strcmp(g_balancers[i]->name, service_name) == 0) {
            LoadBalancer *lb = g_balancers[i];
            pthread_mutex_unlock(&g_lb_mutex);
            return lb;
        }
    }

    pthread_mutex_unlock(&g_lb_mutex);
    return NULL;
}

/*
 * ============================================================================
 * ENDPOINT SELECTION
 * ============================================================================
 */

ServiceEndpoint *lb_select_round_robin(
    LoadBalancer *lb,
    ServiceEndpoint **endpoints,
    int endpoint_count
) {
    if (!lb || !endpoints || endpoint_count == 0) return NULL;

    // Ensure counter array is sized correctly
    if (lb->endpoint_count != endpoint_count) {
        lb->endpoint_counters = realloc(lb->endpoint_counters, sizeof(uint64_t) * endpoint_count);
        for (int i = lb->endpoint_count; i < endpoint_count; i++) {
            lb->endpoint_counters[i] = 0;
        }
        lb->endpoint_count = endpoint_count;
    }

    // Find endpoint with lowest counter
    int selected = 0;
    uint64_t min_count = lb->endpoint_counters[0];

    for (int i = 1; i < endpoint_count; i++) {
        if (lb->endpoint_counters[i] < min_count) {
            min_count = lb->endpoint_counters[i];
            selected = i;
        }
    }

    lb->endpoint_counters[selected]++;
    return endpoints[selected];
}

ServiceEndpoint *lb_select_least_connections(
    ServiceEndpoint **endpoints,
    int endpoint_count
) {
    if (!endpoints || endpoint_count == 0) return NULL;

    // Find endpoint with lowest request count
    int selected = 0;
    uint64_t min_requests = endpoints[0]->request_count - endpoints[0]->error_count;

    for (int i = 1; i < endpoint_count; i++) {
        uint64_t active_requests = endpoints[i]->request_count - endpoints[i]->error_count;
        if (active_requests < min_requests) {
            min_requests = active_requests;
            selected = i;
        }
    }

    return endpoints[selected];
}

ServiceEndpoint *lb_select_random(
    ServiceEndpoint **endpoints,
    int endpoint_count
) {
    if (!endpoints || endpoint_count == 0) return NULL;

    int selected = rand() % endpoint_count;
    return endpoints[selected];
}

ServiceEndpoint *lb_select_least_response_time(
    ServiceEndpoint **endpoints,
    int endpoint_count
) {
    if (!endpoints || endpoint_count == 0) return NULL;

    // Find endpoint with lowest average response time
    int selected = 0;
    double min_response_time = endpoints[0]->avg_response_time_ms;

    for (int i = 1; i < endpoint_count; i++) {
        if (endpoints[i]->avg_response_time_ms < min_response_time) {
            min_response_time = endpoints[i]->avg_response_time_ms;
            selected = i;
        }
    }

    return endpoints[selected];
}

ServiceEndpoint *lb_select_ip_hash(
    ServiceEndpoint **endpoints,
    int endpoint_count,
    const char *client_id
) {
    if (!endpoints || endpoint_count == 0) return NULL;

    if (!client_id) {
        // Fall back to random
        return lb_select_random(endpoints, endpoint_count);
    }

    // Hash client ID to select endpoint
    uint64_t hash = hash_string(client_id);
    int selected = hash % endpoint_count;

    return endpoints[selected];
}

ServiceEndpoint *lb_select_endpoint(
    LoadBalancer *lb,
    ServiceEndpoint **endpoints,
    int endpoint_count,
    const char *client_id
) {
    if (!lb || !endpoints || endpoint_count == 0) return NULL;

    switch (lb->config.algorithm) {
        case LB_ALGORITHM_ROUND_ROBIN:
        case LB_ALGORITHM_WEIGHTED_ROUND_ROBIN:  // Treat as round-robin for now
            return lb_select_round_robin(lb, endpoints, endpoint_count);

        case LB_ALGORITHM_LEAST_CONNECTIONS:
            return lb_select_least_connections(endpoints, endpoint_count);

        case LB_ALGORITHM_RANDOM:
            return lb_select_random(endpoints, endpoint_count);

        case LB_ALGORITHM_LEAST_RESPONSE_TIME:
            return lb_select_least_response_time(endpoints, endpoint_count);

        case LB_ALGORITHM_IP_HASH:
            return lb_select_ip_hash(endpoints, endpoint_count, client_id);

        default:
            return endpoints[0];
    }
}

/*
 * ============================================================================
 * REQUEST ROUTING
 * ============================================================================
 */

ServiceEndpoint *lb_route_request(
    const char *service_name,
    const char *client_id,
    const char *tag
) {
    if (!g_initialized || !service_name) return NULL;

    // Get load balancer (or create default)
    LoadBalancer *lb = lb_get(service_name);
    if (!lb) {
        lb = lb_create(service_name, NULL);
        if (!lb) return NULL;
    }

    // Find service endpoints
    int endpoint_count = 0;
    ServiceEndpoint **endpoints = NULL;

    if (tag) {
        ServiceQuery query = {0};
        strncpy(query.service_name, service_name, sizeof(query.service_name) - 1);
        strncpy(query.tag, tag, sizeof(query.tag) - 1);
        query.min_health = SERVICE_HEALTH_HEALTHY;
        query.only_active = true;
        endpoints = registry_find(&query, &endpoint_count);
    } else {
        endpoints = registry_find_by_name(service_name, &endpoint_count);
    }

    if (!endpoints || endpoint_count == 0) {
        if (endpoints) free(endpoints);
        return NULL;
    }

    // Filter by health if enabled
    if (lb->config.health_check_enabled) {
        int healthy_count = 0;
        for (int i = 0; i < endpoint_count; i++) {
            if (endpoints[i]->health == SERVICE_HEALTH_HEALTHY && endpoints[i]->active) {
                endpoints[healthy_count++] = endpoints[i];
            }
        }
        endpoint_count = healthy_count;
    }

    if (endpoint_count == 0) {
        free(endpoints);
        return NULL;
    }

    // Select endpoint
    ServiceEndpoint *selected = lb_select_endpoint(lb, endpoints, endpoint_count, client_id);

    pthread_mutex_lock(&g_lb_mutex);
    lb->total_requests++;
    g_total_requests++;
    pthread_mutex_unlock(&g_lb_mutex);

    free(endpoints);
    return selected;
}

int lb_report_request(
    const char *service_name,
    const char *endpoint_id,
    bool success,
    double response_time_ms
) {
    if (!g_initialized || !service_name) return -1;

    pthread_mutex_lock(&g_lb_mutex);

    LoadBalancer *lb = NULL;
    for (int i = 0; i < g_balancer_count; i++) {
        if (strcmp(g_balancers[i]->name, service_name) == 0) {
            lb = g_balancers[i];
            break;
        }
    }

    if (success) {
        g_successful_requests++;
    } else {
        g_failed_requests++;
        if (lb) lb->failed_requests++;
    }

    pthread_mutex_unlock(&g_lb_mutex);

    // Update endpoint metrics (via registry)
    if (endpoint_id) {
        ServiceEndpoint *endpoint = registry_get(endpoint_id);
        if (endpoint) {
            if (success) {
                registry_update_metrics(endpoint_id,
                    endpoint->request_count + 1,
                    endpoint->error_count,
                    response_time_ms);
            } else {
                registry_update_metrics(endpoint_id,
                    endpoint->request_count + 1,
                    endpoint->error_count + 1,
                    response_time_ms);
            }
        }
    }

    return 0;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

LBStats lb_get_stats(void) {
    LBStats stats = {0};

    if (!g_initialized) return stats;

    pthread_mutex_lock(&g_lb_mutex);

    stats.total_requests = g_total_requests;
    stats.successful_requests = g_successful_requests;
    stats.failed_requests = g_failed_requests;
    stats.retried_requests = g_retried_requests;
    stats.active_balancers = g_balancer_count;

    pthread_mutex_unlock(&g_lb_mutex);
    return stats;
}

void lb_get_instance_stats(LoadBalancer *lb, LBStats *stats) {
    if (!lb || !stats) return;

    stats->total_requests = lb->total_requests;
    stats->failed_requests = lb->failed_requests;
    stats->successful_requests = lb->total_requests - lb->failed_requests;
    stats->active_balancers = 1;
}
