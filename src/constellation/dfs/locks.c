/*
 * RADS Constellation - Distributed Lock Manager Implementation
 * Phase 4: Distributed Filesystem
 */

#define _GNU_SOURCE

#include "locks.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static Lock **g_locks = NULL;
static int g_lock_count = 0;
static LockConfig g_config;
static pthread_mutex_t g_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_lock_cond = PTHREAD_COND_INITIALIZER;
static pthread_t g_heartbeat_thread;
static bool g_initialized = false;
static bool g_running = false;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static uint64_t get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void generate_lock_id(char *out_id, size_t size) {
    snprintf(out_id, size, "lock-%lu-%d", get_current_time_ms(), rand() % 10000);
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int locks_init(LockConfig *config) {
    pthread_mutex_lock(&g_lock_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_lock_mutex);
        return 1;
    }

    // Set configuration
    if (config) {
        g_config = *config;
    } else {
        g_config.default_timeout_ms = 30000;
        g_config.max_wait_ms = 60000;
        g_config.heartbeat_interval_ms = 10000;
    }

    g_locks = NULL;
    g_lock_count = 0;
    g_initialized = true;
    g_running = true;

    // Start heartbeat thread
    pthread_create(&g_heartbeat_thread, NULL, lock_heartbeat_thread, NULL);

    pthread_mutex_unlock(&g_lock_mutex);
    return 0;
}

int locks_shutdown(void) {
    pthread_mutex_lock(&g_lock_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_lock_mutex);
        return -1;
    }

    g_running = false;
    g_initialized = false;
    pthread_cond_broadcast(&g_lock_cond);

    pthread_mutex_unlock(&g_lock_mutex);

    // Wait for heartbeat thread
    pthread_join(g_heartbeat_thread, NULL);

    pthread_mutex_lock(&g_lock_mutex);

    // Free all locks
    for (int i = 0; i < g_lock_count; i++) {
        free(g_locks[i]);
    }
    free(g_locks);
    g_locks = NULL;
    g_lock_count = 0;

    g_initialized = false;

    pthread_mutex_unlock(&g_lock_mutex);
    return 0;
}

bool locks_is_running(void) {
    pthread_mutex_lock(&g_lock_mutex);
    bool running = g_initialized && g_running;
    pthread_mutex_unlock(&g_lock_mutex);
    return running;
}

/*
 * ============================================================================
 * LOCK OPERATIONS
 * ============================================================================
 */

Lock *lock_acquire(const char *resource, LockMode mode, int timeout_ms) {
    if (!g_initialized || !resource) return NULL;

    pthread_mutex_lock(&g_lock_mutex);

    // Check if resource is already locked
    bool is_locked = false;
    bool compatible = true;

    for (int i = 0; i < g_lock_count; i++) {
        if (strcmp(g_locks[i]->resource, resource) == 0 && g_locks[i]->is_acquired) {
            is_locked = true;

            // Check compatibility
            if (mode == LOCK_MODE_EXCLUSIVE || g_locks[i]->mode == LOCK_MODE_EXCLUSIVE) {
                compatible = false;
                break;
            }
        }
    }

    // If not compatible, wait or fail
    if (is_locked && !compatible) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&g_lock_mutex);
            return NULL; // Non-blocking, can't acquire
        }

        // Wait for lock to be released
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;

        int wait_result = pthread_cond_timedwait(&g_lock_cond, &g_lock_mutex, &ts);

        if (wait_result != 0) {
            pthread_mutex_unlock(&g_lock_mutex);
            return NULL; // Timeout
        }
    }

    // Create lock
    Lock *lock = calloc(1, sizeof(Lock));
    if (!lock) {
        pthread_mutex_unlock(&g_lock_mutex);
        return NULL;
    }

    strncpy(lock->resource, resource, sizeof(lock->resource) - 1);
    generate_lock_id(lock->lock_id, sizeof(lock->lock_id));
    strncpy(lock->owner_id, "local", sizeof(lock->owner_id) - 1); // TODO: Use actual node ID
    lock->mode = mode;
    lock->acquired_at = time(NULL);

    if (timeout_ms > 0) {
        lock->expires_at = lock->acquired_at + (timeout_ms / 1000);
    } else {
        lock->expires_at = 0; // No expiration
    }

    lock->is_acquired = true;

    // Register lock
    g_locks = realloc(g_locks, sizeof(Lock *) * (g_lock_count + 1));
    g_locks[g_lock_count] = lock;
    g_lock_count++;

    pthread_mutex_unlock(&g_lock_mutex);
    return lock;
}

int lock_release(Lock *lock) {
    if (!g_initialized || !lock) return -1;

    pthread_mutex_lock(&g_lock_mutex);

    // Find and remove lock
    for (int i = 0; i < g_lock_count; i++) {
        if (strcmp(g_locks[i]->lock_id, lock->lock_id) == 0) {
            free(g_locks[i]);

            // Shift array
            for (int j = i; j < g_lock_count - 1; j++) {
                g_locks[j] = g_locks[j + 1];
            }

            g_lock_count--;

            // Signal waiting threads
            pthread_cond_broadcast(&g_lock_cond);

            pthread_mutex_unlock(&g_lock_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return -1;
}

int lock_renew(Lock *lock, int additional_ms) {
    if (!lock || additional_ms <= 0) return -1;

    pthread_mutex_lock(&g_lock_mutex);

    if (lock->expires_at > 0) {
        lock->expires_at += (additional_ms / 1000);
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return 0;
}

Lock *lock_try_acquire(const char *resource, LockMode mode) {
    return lock_acquire(resource, mode, 0); // 0 timeout = non-blocking
}

Lock *lock_shared(const char *resource) {
    return lock_acquire(resource, LOCK_MODE_SHARED, g_config.default_timeout_ms);
}

Lock *lock_exclusive(const char *resource) {
    return lock_acquire(resource, LOCK_MODE_EXCLUSIVE, g_config.default_timeout_ms);
}

bool lock_is_held(const char *resource) {
    if (!g_initialized || !resource) return false;

    pthread_mutex_lock(&g_lock_mutex);

    for (int i = 0; i < g_lock_count; i++) {
        if (strcmp(g_locks[i]->resource, resource) == 0 && g_locks[i]->is_acquired) {
            pthread_mutex_unlock(&g_lock_mutex);
            return true;
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return false;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

LockStats lock_get_stats(void) {
    LockStats stats = {0};

    if (!g_initialized) return stats;

    pthread_mutex_lock(&g_lock_mutex);

    stats.total_locks = g_lock_count;

    for (int i = 0; i < g_lock_count; i++) {
        if (g_locks[i]->mode == LOCK_MODE_SHARED) {
            stats.shared_locks++;
        } else {
            stats.exclusive_locks++;
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return stats;
}

/*
 * ============================================================================
 * CLEANUP
 * ============================================================================
 */

int lock_cleanup_expired(void) {
    pthread_mutex_lock(&g_lock_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_lock_mutex);
        return 0;
    }

    time_t now = time(NULL);
    int cleaned = 0;

    for (int i = 0; i < g_lock_count; ) {
        if (g_locks[i]->expires_at > 0 && g_locks[i]->expires_at < now) {
            // Lock expired
            free(g_locks[i]);

            for (int j = i; j < g_lock_count - 1; j++) {
                g_locks[j] = g_locks[j + 1];
            }

            g_lock_count--;
            cleaned++;
        } else {
            i++;
        }
    }

    if (cleaned > 0) {
        pthread_cond_broadcast(&g_lock_cond);
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return cleaned;
}

/*
 * ============================================================================
 * HEARTBEAT THREAD
 * ============================================================================
 */

void *lock_heartbeat_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks so we can exit quickly
        for (int i = 0; i < g_config.heartbeat_interval_ms / 100 && g_running; i++) {
            usleep(100000); // 100ms
        }

        if (!g_running) break;

        // Cleanup expired locks
        lock_cleanup_expired();
    }

    return NULL;
}

/*
 * ============================================================================
 * ADMIN OPERATIONS
 * ============================================================================
 */

int lock_force_unlock(const char *resource) {
    if (!g_initialized || !resource) return -1;

    pthread_mutex_lock(&g_lock_mutex);

    int unlocked = 0;

    for (int i = 0; i < g_lock_count; ) {
        if (strcmp(g_locks[i]->resource, resource) == 0) {
            free(g_locks[i]);

            for (int j = i; j < g_lock_count - 1; j++) {
                g_locks[j] = g_locks[j + 1];
            }

            g_lock_count--;
            unlocked++;
        } else {
            i++;
        }
    }

    if (unlocked > 0) {
        pthread_cond_broadcast(&g_lock_cond);
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return unlocked;
}
