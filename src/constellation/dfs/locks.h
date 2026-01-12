/*
 * RADS Constellation - Distributed Lock Manager
 * Phase 4: Distributed Filesystem
 *
 * Distributed locks for file coordination
 */

#ifndef CONSTELLATION_LOCKS_H
#define CONSTELLATION_LOCKS_H

#include "../constellation.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * LOCK TYPES
 * ============================================================================
 */

// Lock modes
typedef enum {
    LOCK_MODE_SHARED,        // Multiple readers
    LOCK_MODE_EXCLUSIVE      // Single writer
} LockMode;

// Lock handle
typedef struct {
    char resource[512];      // Resource being locked (file path, key, etc.)
    char lock_id[64];        // Unique lock ID
    char owner_id[64];       // Node ID that owns this lock
    LockMode mode;           // Lock mode
    time_t acquired_at;      // When lock was acquired
    time_t expires_at;       // When lock expires (0 = no expiration)
    bool is_acquired;        // Has this lock been acquired?
} Lock;

// Lock configuration
typedef struct {
    int default_timeout_ms;  // Default lock timeout (default: 30000ms)
    int max_wait_ms;         // Max time to wait for lock (default: 60000ms)
    int heartbeat_interval_ms; // Heartbeat interval to keep lock alive
} LockConfig;

/*
 * ============================================================================
 * LOCK API
 * ============================================================================
 */

// Lifecycle
int locks_init(LockConfig *config);
int locks_shutdown(void);
bool locks_is_running(void);

// Lock operations
Lock *lock_acquire(const char *resource, LockMode mode, int timeout_ms);
int lock_release(Lock *lock);
int lock_renew(Lock *lock, int additional_ms);
bool lock_is_held(const char *resource);
Lock *lock_get_current(const char *resource);

// Try-lock (non-blocking)
Lock *lock_try_acquire(const char *resource, LockMode mode);

// Convenience macros
Lock *lock_shared(const char *resource);
Lock *lock_exclusive(const char *resource);

// Lock information
typedef struct {
    int total_locks;
    int shared_locks;
    int exclusive_locks;
    int waiting_locks;
} LockStats;

LockStats lock_get_stats(void);

// Deadlock detection
bool lock_has_deadlock(void);
int lock_break_deadlock(void);

// Force unlock (admin operation)
int lock_force_unlock(const char *resource);

/*
 * ============================================================================
 * INTERNAL API
 * ============================================================================
 */

// Lock management
int lock_register(Lock *lock);
int lock_unregister(const char *lock_id);
Lock *lock_find(const char *lock_id);

// Cleanup expired locks
int lock_cleanup_expired(void);

// Heartbeat thread (keeps locks alive)
void *lock_heartbeat_thread(void *arg);

#endif /* CONSTELLATION_LOCKS_H */
