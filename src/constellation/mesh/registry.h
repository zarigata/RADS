/*
 * RADS Constellation - Service Registry
 * Phase 5: Service Mesh
 */

#ifndef CONSTELLATION_REGISTRY_H
#define CONSTELLATION_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * SERVICE REGISTRY
 * ============================================================================
 */

#define MAX_SERVICE_NAME 128
#define MAX_SERVICE_ENDPOINT 256
#define MAX_SERVICE_TAGS 8
#define MAX_TAG_LENGTH 64
#define MAX_METADATA_ENTRIES 16
#define MAX_METADATA_KEY 64
#define MAX_METADATA_VALUE 256

// Service health status
typedef enum {
    SERVICE_HEALTH_UNKNOWN = 0,
    SERVICE_HEALTH_HEALTHY = 1,
    SERVICE_HEALTH_DEGRADED = 2,
    SERVICE_HEALTH_UNHEALTHY = 3
} ServiceHealth;

// Service metadata entry
typedef struct {
    char key[MAX_METADATA_KEY];
    char value[MAX_METADATA_VALUE];
} MetadataEntry;

// Service endpoint
typedef struct {
    char service_id[MAX_SERVICE_NAME];
    char service_name[MAX_SERVICE_NAME];
    char endpoint[MAX_SERVICE_ENDPOINT];  // host:port or socket path

    // Metadata
    char tags[MAX_SERVICE_TAGS][MAX_TAG_LENGTH];
    int tag_count;
    MetadataEntry metadata[MAX_METADATA_ENTRIES];
    int metadata_count;

    // Health tracking
    ServiceHealth health;
    time_t last_heartbeat;
    time_t registered_at;

    // Load tracking
    uint64_t request_count;
    uint64_t error_count;
    double avg_response_time_ms;

    // Node information
    int node_id;
    char version[32];

    bool active;
} ServiceEndpoint;

// Service registry configuration
typedef struct {
    int max_services;
    int heartbeat_interval_ms;
    int heartbeat_timeout_ms;
    bool enable_health_checks;
    int health_check_interval_ms;
} RegistryConfig;

// Service query filter
typedef struct {
    char service_name[MAX_SERVICE_NAME];
    char tag[MAX_TAG_LENGTH];
    ServiceHealth min_health;
    int node_id;  // -1 for any node
    bool only_active;
} ServiceQuery;

// Registry statistics
typedef struct {
    int total_services;
    int healthy_services;
    int degraded_services;
    int unhealthy_services;
    int inactive_services;
    uint64_t total_registrations;
    uint64_t total_deregistrations;
} RegistryStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

// Initialize service registry
int registry_init(RegistryConfig *config);

// Shutdown service registry
int registry_shutdown(void);

// Check if registry is running
bool registry_is_running(void);

/*
 * ============================================================================
 * SERVICE REGISTRATION
 * ============================================================================
 */

// Register a service
ServiceEndpoint *registry_register(
    const char *service_name,
    const char *endpoint,
    const char **tags,
    int tag_count
);

// Deregister a service by ID
int registry_deregister(const char *service_id);

// Update service metadata
int registry_set_metadata(const char *service_id, const char *key, const char *value);

// Update service health
int registry_set_health(const char *service_id, ServiceHealth health);

// Send heartbeat for service
int registry_heartbeat(const char *service_id);

/*
 * ============================================================================
 * SERVICE DISCOVERY
 * ============================================================================
 */

// Find services by name
ServiceEndpoint **registry_find_by_name(const char *service_name, int *count);

// Find services by tag
ServiceEndpoint **registry_find_by_tag(const char *tag, int *count);

// Find services matching query
ServiceEndpoint **registry_find(ServiceQuery *query, int *count);

// Get service by ID
ServiceEndpoint *registry_get(const char *service_id);

// List all services
ServiceEndpoint **registry_list_all(int *count);

/*
 * ============================================================================
 * HEALTH MONITORING
 * ============================================================================
 */

// Check service health
ServiceHealth registry_check_health(const char *service_id);

// Mark service as healthy
int registry_mark_healthy(const char *service_id);

// Mark service as unhealthy
int registry_mark_unhealthy(const char *service_id);

// Update service metrics
int registry_update_metrics(
    const char *service_id,
    uint64_t requests,
    uint64_t errors,
    double avg_response_ms
);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

// Get registry statistics
RegistryStats registry_get_stats(void);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

// Health check thread
void *registry_health_check_thread(void *arg);

// Cleanup thread
void *registry_cleanup_thread(void *arg);

#endif // CONSTELLATION_REGISTRY_H
