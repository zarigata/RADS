#ifndef RADS_GC_H
#define RADS_GC_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct VM VM;
typedef struct Value Value;

// ============================================================================
// ADVANCED GARBAGE COLLECTION - Generational GC with Write Barriers
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// Generation identifiers
typedef enum {
    GC_GEN_YOUNG = 0,
    GC_GEN_OLD = 1,
    GC_GEN_COUNT = 2
} GCGeneration;

// GC pause time statistics
typedef struct {
    long young_gen_pause_us;
    long old_gen_pause_us;
    size_t collections_young;
    size_t collections_old;
    size_t objects_collected;
    size_t bytes_freed;
} GCStats;

// GC write barrier types
typedef enum {
    WRITE_BARRIER_NONE = 0,
    WRITE_BARRIER_FIELD,
    WRITE_BARRIER_ARRAY_ELEMENT,
    WRITE_BARRIER_GLOBAL
} WriteBarrierType;

// GC state
typedef struct {
    VM* vm;
    bool generational_enabled;
    bool write_barriers_enabled;
    GCStats stats;
    size_t young_gen_size;
    size_t old_gen_size;
    size_t young_gen_used;
    size_t old_gen_used;
    int gc_threshold_young;  // Bytes before young gen collection
    int gc_threshold_old;    // Bytes before old gen collection
} GCState;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize the advanced garbage collector
 *
 * @param vm Pointer to the virtual machine
 * @return GC state pointer on success, NULL on failure
 */
GCState* gc_init(VM* vm);

/**
 * Allocate memory with write barrier tracking
 *
 * @param state GC state
 * @param size Number of bytes to allocate
 * @param type Write barrier type
 * @return Pointer to allocated memory, or NULL on failure
 */
void* gc_allocate_with_barrier(
    GCState* state,
    size_t size,
    WriteBarrierType type
);

/**
 * Perform a write barrier operation
 *
 * @param state GC state
 * @param object Target object
 * @param field Pointer to field being written
 * @param value Value being written
 */
void gc_write_barrier(
    GCState* state,
    void* object,
    void* field,
    Value value
);

/**
 * Trigger garbage collection
 *
 * @param state GC state
 * @param full_collection If true, collect all generations
 * @return Number of objects collected
 */
size_t gc_collect(GCState* state, bool full_collection);

/**
 * Force full garbage collection (both generations)
 *
 * @param state GC state
 * @return Number of objects collected
 */
size_t gc_collect_full(GCState* state);

/**
 * Get GC statistics
 *
 * @param state GC state
 * @param[out] stats GC statistics structure
 */
void gc_get_statistics(GCState* state, GCStats* stats);

/**
 * Enable/disable generational GC
 *
 * @param state GC state
 * @param enabled true to enable, false to disable
 */
void gc_set_generational(GCState* state, bool enabled);

/**
 * Enable/disable write barriers
 *
 * @param state GC state
 * @param enabled true to enable, false to disable
 */
void gc_set_write_barriers(GCState* state, bool enabled);

/**
 * Check if pointer is in old generation
 *
 * @param state GC state
 * @param ptr Pointer to check
 * @return true if pointer is in old generation
 */
bool gc_is_old_generation(GCState* state, void* ptr);

/**
 * Get current memory usage
 *
 * @param state GC state
 * @param[out] young_used Young generation bytes used
 * @param[out] old_used Old generation bytes used
 * @param[out] total_used Total bytes used
 */
void gc_get_memory_usage(
    GCState* state,
    size_t* young_used,
    size_t* old_used,
    size_t* total_used
);

/**
 * Cleanup and free GC state
 *
 * @param state GC state to free
 */
void gc_cleanup(GCState* state);

#endif // RADS_GC_H
