/*
 * RADS Constellation - Load Balancer
 * Phase 5: Service Mesh
 */

#ifndef CONSTELLATION_LOADBALANCER_H
#define CONSTELLATION_LOADBALANCER_H

#include "registry.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================================
 * LOAD BALANCER
 * ============================================================================
 */

// Load balancing algorithms
typedef enum {
    LB_ALGORITHM_ROUND_ROBIN = 0,
    LB_ALGORITHM_LEAST_CONNECTIONS = 1,
    LB_ALGORITHM_RANDOM = 2,
    LB_ALGORITHM_WEIGHTED_ROUND_ROBIN = 3,
    LB_ALGORITHM_LEAST_RESPONSE_TIME = 4,
    LB_ALGORITHM_IP_HASH = 5
} LBAlgorithm;

// Load balancer configuration
typedef struct {
    LBAlgorithm algorithm;
    bool health_check_enabled;
    int max_retries;
    int retry_delay_ms;
    bool sticky_sessions;
    int session_timeout_ms;
} LBConfig;

// Load balancer instance
typedef struct {
    char name[MAX_SERVICE_NAME];
    LBConfig config;
    uint64_t *endpoint_counters;  // For round-robin tracking
    int endpoint_count;
    time_t created_at;
    uint64_t total_requests;
    uint64_t failed_requests;
} LoadBalancer;

// Load balancer statistics
typedef struct {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t retried_requests;
    double avg_response_time_ms;
    int active_balancers;
} LBStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

// Initialize load balancer system
int lb_init(void);

// Shutdown load balancer system
int lb_shutdown(void);

// Check if load balancer is running
bool lb_is_running(void);

/*
 * ============================================================================
 * LOAD BALANCER MANAGEMENT
 * ============================================================================
 */

// Create a load balancer for a service
LoadBalancer *lb_create(const char *service_name, LBConfig *config);

// Destroy a load balancer
int lb_destroy(const char *service_name);

// Get load balancer by name
LoadBalancer *lb_get(const char *service_name);

/*
 * ============================================================================
 * ENDPOINT SELECTION
 * ============================================================================
 */

// Select an endpoint using the configured algorithm
ServiceEndpoint *lb_select_endpoint(
    LoadBalancer *lb,
    ServiceEndpoint **endpoints,
    int endpoint_count,
    const char *client_id  // For sticky sessions or IP hash
);

// Select endpoint with round-robin
ServiceEndpoint *lb_select_round_robin(
    LoadBalancer *lb,
    ServiceEndpoint **endpoints,
    int endpoint_count
);

// Select endpoint with least connections
ServiceEndpoint *lb_select_least_connections(
    ServiceEndpoint **endpoints,
    int endpoint_count
);

// Select endpoint randomly
ServiceEndpoint *lb_select_random(
    ServiceEndpoint **endpoints,
    int endpoint_count
);

// Select endpoint with least response time
ServiceEndpoint *lb_select_least_response_time(
    ServiceEndpoint **endpoints,
    int endpoint_count
);

// Select endpoint with IP hash
ServiceEndpoint *lb_select_ip_hash(
    ServiceEndpoint **endpoints,
    int endpoint_count,
    const char *client_id
);

/*
 * ============================================================================
 * REQUEST ROUTING
 * ============================================================================
 */

// Route a request to a service
ServiceEndpoint *lb_route_request(
    const char *service_name,
    const char *client_id,
    const char *tag  // Optional filter
);

// Report request completion
int lb_report_request(
    const char *service_name,
    const char *endpoint_id,
    bool success,
    double response_time_ms
);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

// Get load balancer statistics
LBStats lb_get_stats(void);

// Get load balancer instance statistics
void lb_get_instance_stats(LoadBalancer *lb, LBStats *stats);

#endif // CONSTELLATION_LOADBALANCER_H
