/*
 * RADS Constellation - Circuit Breaker Implementation
 * Phase 5: Service Mesh
 */

#define _GNU_SOURCE

#include "circuit.h"
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

static CircuitBreaker **g_circuits = NULL;
static int g_circuit_count = 0;
static pthread_mutex_t g_circuit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_state_thread;
static bool g_initialized = false;
static bool g_running = false;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int circuit_init(void) {
    pthread_mutex_lock(&g_circuit_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_circuit_mutex);
        return 1;
    }

    g_circuits = NULL;
    g_circuit_count = 0;
    g_initialized = true;
    g_running = true;

    // Start state management thread
    pthread_create(&g_state_thread, NULL, circuit_state_thread, NULL);

    pthread_mutex_unlock(&g_circuit_mutex);
    return 0;
}

int circuit_shutdown(void) {
    pthread_mutex_lock(&g_circuit_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_circuit_mutex);
        return -1;
    }

    g_running = false;
    g_initialized = false;

    pthread_mutex_unlock(&g_circuit_mutex);

    // Wait for thread
    pthread_join(g_state_thread, NULL);

    pthread_mutex_lock(&g_circuit_mutex);

    // Free all circuits
    for (int i = 0; i < g_circuit_count; i++) {
        free(g_circuits[i]);
    }
    free(g_circuits);

    g_circuits = NULL;
    g_circuit_count = 0;

    pthread_mutex_unlock(&g_circuit_mutex);
    return 0;
}

bool circuit_is_running(void) {
    pthread_mutex_lock(&g_circuit_mutex);
    bool running = g_initialized && g_running;
    pthread_mutex_unlock(&g_circuit_mutex);
    return running;
}

/*
 * ============================================================================
 * CIRCUIT MANAGEMENT
 * ============================================================================
 */

CircuitBreaker *circuit_create(const char *name, CircuitConfig *config) {
    if (!g_initialized || !name) return NULL;

    pthread_mutex_lock(&g_circuit_mutex);

    // Check if already exists
    for (int i = 0; i < g_circuit_count; i++) {
        if (strcmp(g_circuits[i]->name, name) == 0) {
            pthread_mutex_unlock(&g_circuit_mutex);
            return g_circuits[i];
        }
    }

    // Create new circuit breaker
    CircuitBreaker *circuit = calloc(1, sizeof(CircuitBreaker));
    if (!circuit) {
        pthread_mutex_unlock(&g_circuit_mutex);
        return NULL;
    }

    strncpy(circuit->name, name, sizeof(circuit->name) - 1);
    circuit->state = CIRCUIT_STATE_CLOSED;

    if (config) {
        circuit->config = *config;
    } else {
        circuit->config.failure_threshold = 5;
        circuit->config.success_threshold = 2;
        circuit->config.timeout_ms = 60000;  // 1 minute
        circuit->config.window_size_ms = 10000;  // 10 seconds
        circuit->config.error_rate_threshold = 0.5;  // 50%
    }

    circuit->total_requests = 0;
    circuit->successful_requests = 0;
    circuit->failed_requests = 0;
    circuit->rejected_requests = 0;
    circuit->consecutive_failures = 0;
    circuit->consecutive_successes = 0;
    circuit->state_changed_at = time(NULL);
    circuit->window_start = time(NULL);
    circuit->enabled = true;

    // Add to list
    g_circuits = realloc(g_circuits, sizeof(CircuitBreaker *) * (g_circuit_count + 1));
    g_circuits[g_circuit_count] = circuit;
    g_circuit_count++;

    pthread_mutex_unlock(&g_circuit_mutex);
    return circuit;
}

int circuit_destroy(const char *name) {
    if (!g_initialized || !name) return -1;

    pthread_mutex_lock(&g_circuit_mutex);

    for (int i = 0; i < g_circuit_count; i++) {
        if (strcmp(g_circuits[i]->name, name) == 0) {
            free(g_circuits[i]);

            // Shift array
            for (int j = i; j < g_circuit_count - 1; j++) {
                g_circuits[j] = g_circuits[j + 1];
            }

            g_circuit_count--;

            pthread_mutex_unlock(&g_circuit_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_circuit_mutex);
    return -1;
}

CircuitBreaker *circuit_get(const char *name) {
    if (!g_initialized || !name) return NULL;

    pthread_mutex_lock(&g_circuit_mutex);

    for (int i = 0; i < g_circuit_count; i++) {
        if (strcmp(g_circuits[i]->name, name) == 0) {
            CircuitBreaker *circuit = g_circuits[i];
            pthread_mutex_unlock(&g_circuit_mutex);
            return circuit;
        }
    }

    pthread_mutex_unlock(&g_circuit_mutex);
    return NULL;
}

int circuit_set_enabled(const char *name, bool enabled) {
    CircuitBreaker *circuit = circuit_get(name);
    if (!circuit) return -1;

    pthread_mutex_lock(&g_circuit_mutex);
    circuit->enabled = enabled;
    pthread_mutex_unlock(&g_circuit_mutex);

    return 0;
}

/*
 * ============================================================================
 * CIRCUIT OPERATIONS
 * ============================================================================
 */

bool circuit_allow_request(CircuitBreaker *circuit) {
    if (!circuit || !circuit->enabled) return true;

    pthread_mutex_lock(&g_circuit_mutex);

    time_t now = time(NULL);

    // Check if we should transition from OPEN to HALF_OPEN
    if (circuit->state == CIRCUIT_STATE_OPEN) {
        int elapsed_ms = (now - circuit->state_changed_at) * 1000;
        if (elapsed_ms >= circuit->config.timeout_ms) {
            circuit->state = CIRCUIT_STATE_HALF_OPEN;
            circuit->state_changed_at = now;
            circuit->consecutive_successes = 0;
        } else {
            circuit->rejected_requests++;
            pthread_mutex_unlock(&g_circuit_mutex);
            return false;
        }
    }

    // In HALF_OPEN, allow limited requests
    if (circuit->state == CIRCUIT_STATE_HALF_OPEN) {
        // Allow request, will test the service
        pthread_mutex_unlock(&g_circuit_mutex);
        return true;
    }

    // CLOSED state - allow request
    pthread_mutex_unlock(&g_circuit_mutex);
    return true;
}

int circuit_record_success(CircuitBreaker *circuit) {
    if (!circuit) return -1;

    pthread_mutex_lock(&g_circuit_mutex);

    circuit->total_requests++;
    circuit->successful_requests++;
    circuit->window_requests++;
    circuit->consecutive_failures = 0;
    circuit->consecutive_successes++;

    // If HALF_OPEN, check if we can close
    if (circuit->state == CIRCUIT_STATE_HALF_OPEN) {
        if (circuit->consecutive_successes >= circuit->config.success_threshold) {
            circuit->state = CIRCUIT_STATE_CLOSED;
            circuit->state_changed_at = time(NULL);
            circuit->consecutive_successes = 0;
        }
    }

    pthread_mutex_unlock(&g_circuit_mutex);
    return 0;
}

int circuit_record_failure(CircuitBreaker *circuit) {
    if (!circuit) return -1;

    pthread_mutex_lock(&g_circuit_mutex);

    time_t now = time(NULL);

    circuit->total_requests++;
    circuit->failed_requests++;
    circuit->window_requests++;
    circuit->window_failures++;
    circuit->consecutive_failures++;
    circuit->consecutive_successes = 0;
    circuit->last_failure_time = now;

    // Check if we should open the circuit
    bool should_open = false;

    // Check consecutive failures
    if (circuit->consecutive_failures >= circuit->config.failure_threshold) {
        should_open = true;
    }

    // Check error rate in window
    if (circuit->window_requests > 0) {
        double error_rate = (double)circuit->window_failures / circuit->window_requests;
        if (error_rate >= circuit->config.error_rate_threshold) {
            should_open = true;
        }
    }

    // If HALF_OPEN, any failure reopens the circuit
    if (circuit->state == CIRCUIT_STATE_HALF_OPEN) {
        should_open = true;
    }

    if (should_open && circuit->state != CIRCUIT_STATE_OPEN) {
        circuit->state = CIRCUIT_STATE_OPEN;
        circuit->state_changed_at = now;
    }

    pthread_mutex_unlock(&g_circuit_mutex);
    return 0;
}

int circuit_open(CircuitBreaker *circuit) {
    if (!circuit) return -1;

    pthread_mutex_lock(&g_circuit_mutex);
    circuit->state = CIRCUIT_STATE_OPEN;
    circuit->state_changed_at = time(NULL);
    pthread_mutex_unlock(&g_circuit_mutex);

    return 0;
}

int circuit_close(CircuitBreaker *circuit) {
    if (!circuit) return -1;

    pthread_mutex_lock(&g_circuit_mutex);
    circuit->state = CIRCUIT_STATE_CLOSED;
    circuit->state_changed_at = time(NULL);
    circuit->consecutive_failures = 0;
    circuit->consecutive_successes = 0;
    pthread_mutex_unlock(&g_circuit_mutex);

    return 0;
}

int circuit_reset(CircuitBreaker *circuit) {
    if (!circuit) return -1;

    pthread_mutex_lock(&g_circuit_mutex);

    circuit->state = CIRCUIT_STATE_CLOSED;
    circuit->total_requests = 0;
    circuit->successful_requests = 0;
    circuit->failed_requests = 0;
    circuit->rejected_requests = 0;
    circuit->consecutive_failures = 0;
    circuit->consecutive_successes = 0;
    circuit->window_requests = 0;
    circuit->window_failures = 0;
    circuit->window_start = time(NULL);
    circuit->state_changed_at = time(NULL);

    pthread_mutex_unlock(&g_circuit_mutex);
    return 0;
}

/*
 * ============================================================================
 * STATE MANAGEMENT
 * ============================================================================
 */

CircuitState circuit_get_state(CircuitBreaker *circuit) {
    if (!circuit) return CIRCUIT_STATE_CLOSED;

    pthread_mutex_lock(&g_circuit_mutex);
    CircuitState state = circuit->state;
    pthread_mutex_unlock(&g_circuit_mutex);

    return state;
}

double circuit_get_error_rate(CircuitBreaker *circuit) {
    if (!circuit) return 0.0;

    pthread_mutex_lock(&g_circuit_mutex);

    double error_rate = 0.0;
    if (circuit->window_requests > 0) {
        error_rate = (double)circuit->window_failures / circuit->window_requests;
    }

    pthread_mutex_unlock(&g_circuit_mutex);
    return error_rate;
}

bool circuit_is_healthy(CircuitBreaker *circuit) {
    if (!circuit) return true;

    pthread_mutex_lock(&g_circuit_mutex);
    bool healthy = (circuit->state == CIRCUIT_STATE_CLOSED || circuit->state == CIRCUIT_STATE_HALF_OPEN);
    pthread_mutex_unlock(&g_circuit_mutex);

    return healthy;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

CircuitStats circuit_get_stats(void) {
    CircuitStats stats = {0};

    if (!g_initialized) return stats;

    pthread_mutex_lock(&g_circuit_mutex);

    stats.total_circuits = g_circuit_count;

    for (int i = 0; i < g_circuit_count; i++) {
        CircuitBreaker *circuit = g_circuits[i];

        switch (circuit->state) {
            case CIRCUIT_STATE_CLOSED:
                stats.closed_circuits++;
                break;
            case CIRCUIT_STATE_OPEN:
                stats.open_circuits++;
                break;
            case CIRCUIT_STATE_HALF_OPEN:
                stats.half_open_circuits++;
                break;
        }

        stats.total_requests += circuit->total_requests;
        stats.total_failures += circuit->failed_requests;
        stats.total_rejections += circuit->rejected_requests;
    }

    pthread_mutex_unlock(&g_circuit_mutex);
    return stats;
}

void circuit_get_instance_stats(CircuitBreaker *circuit, CircuitStats *stats) {
    if (!circuit || !stats) return;

    pthread_mutex_lock(&g_circuit_mutex);

    stats->total_circuits = 1;
    stats->total_requests = circuit->total_requests;
    stats->total_failures = circuit->failed_requests;
    stats->total_rejections = circuit->rejected_requests;

    switch (circuit->state) {
        case CIRCUIT_STATE_CLOSED:
            stats->closed_circuits = 1;
            break;
        case CIRCUIT_STATE_OPEN:
            stats->open_circuits = 1;
            break;
        case CIRCUIT_STATE_HALF_OPEN:
            stats->half_open_circuits = 1;
            break;
    }

    pthread_mutex_unlock(&g_circuit_mutex);
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *circuit_state_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks for quick exit
        for (int i = 0; i < 10000 / 100 && g_running; i++) {  // 10 sec
            usleep(100000);  // 100ms
        }

        if (!g_running) continue;

        pthread_mutex_lock(&g_circuit_mutex);

        time_t now = time(NULL);

        for (int i = 0; i < g_circuit_count; i++) {
            CircuitBreaker *circuit = g_circuits[i];

            // Reset window if expired
            int window_elapsed_ms = (now - circuit->window_start) * 1000;
            if (window_elapsed_ms >= circuit->config.window_size_ms) {
                circuit->window_requests = 0;
                circuit->window_failures = 0;
                circuit->window_start = now;
            }

            // Check if OPEN circuit should move to HALF_OPEN
            if (circuit->state == CIRCUIT_STATE_OPEN) {
                int elapsed_ms = (now - circuit->state_changed_at) * 1000;
                if (elapsed_ms >= circuit->config.timeout_ms) {
                    circuit->state = CIRCUIT_STATE_HALF_OPEN;
                    circuit->state_changed_at = now;
                    circuit->consecutive_successes = 0;
                }
            }
        }

        pthread_mutex_unlock(&g_circuit_mutex);
    }

    return NULL;
}
