/*
 * RADS Constellation - Circuit Breaker
 * Phase 5: Service Mesh
 */

#ifndef CONSTELLATION_CIRCUIT_H
#define CONSTELLATION_CIRCUIT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * CIRCUIT BREAKER
 * ============================================================================
 */

#define MAX_CIRCUIT_NAME 128

// Circuit breaker states
typedef enum {
    CIRCUIT_STATE_CLOSED = 0,      // Normal operation
    CIRCUIT_STATE_OPEN = 1,        // Failing, reject requests
    CIRCUIT_STATE_HALF_OPEN = 2    // Testing recovery
} CircuitState;

// Circuit breaker configuration
typedef struct {
    int failure_threshold;         // Number of failures before opening
    int success_threshold;         // Number of successes to close from half-open
    int timeout_ms;                // Time before moving from open to half-open
    int window_size_ms;            // Time window for counting failures
    double error_rate_threshold;   // Error rate to trigger open (0.0-1.0)
} CircuitConfig;

// Circuit breaker instance
typedef struct {
    char name[MAX_CIRCUIT_NAME];
    CircuitState state;
    CircuitConfig config;

    // Counters
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t rejected_requests;

    // State tracking
    int consecutive_failures;
    int consecutive_successes;
    time_t last_failure_time;
    time_t state_changed_at;

    // Window tracking
    uint64_t window_requests;
    uint64_t window_failures;
    time_t window_start;

    bool enabled;
} CircuitBreaker;

// Circuit breaker statistics
typedef struct {
    int total_circuits;
    int closed_circuits;
    int open_circuits;
    int half_open_circuits;
    uint64_t total_requests;
    uint64_t total_failures;
    uint64_t total_rejections;
} CircuitStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

// Initialize circuit breaker system
int circuit_init(void);

// Shutdown circuit breaker system
int circuit_shutdown(void);

// Check if circuit breaker system is running
bool circuit_is_running(void);

/*
 * ============================================================================
 * CIRCUIT MANAGEMENT
 * ============================================================================
 */

// Create a circuit breaker
CircuitBreaker *circuit_create(const char *name, CircuitConfig *config);

// Destroy a circuit breaker
int circuit_destroy(const char *name);

// Get circuit breaker by name
CircuitBreaker *circuit_get(const char *name);

// Enable/disable circuit breaker
int circuit_set_enabled(const char *name, bool enabled);

/*
 * ============================================================================
 * CIRCUIT OPERATIONS
 * ============================================================================
 */

// Check if request is allowed
bool circuit_allow_request(CircuitBreaker *circuit);

// Record successful request
int circuit_record_success(CircuitBreaker *circuit);

// Record failed request
int circuit_record_failure(CircuitBreaker *circuit);

// Manually open circuit
int circuit_open(CircuitBreaker *circuit);

// Manually close circuit
int circuit_close(CircuitBreaker *circuit);

// Reset circuit breaker
int circuit_reset(CircuitBreaker *circuit);

/*
 * ============================================================================
 * STATE MANAGEMENT
 * ============================================================================
 */

// Get current state
CircuitState circuit_get_state(CircuitBreaker *circuit);

// Get error rate
double circuit_get_error_rate(CircuitBreaker *circuit);

// Check if circuit is healthy
bool circuit_is_healthy(CircuitBreaker *circuit);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

// Get circuit breaker statistics
CircuitStats circuit_get_stats(void);

// Get individual circuit statistics
void circuit_get_instance_stats(CircuitBreaker *circuit, CircuitStats *stats);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

// State management thread
void *circuit_state_thread(void *arg);

#endif // CONSTELLATION_CIRCUIT_H
