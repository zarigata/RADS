#include "gc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// ADVANCED GARBAGE COLLECTION - Generational GC with Write Barriers
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// ============================================================================
// IMPLEMENTATION
// ============================================================================

/**
 * Initialize the advanced garbage collector
 */
GCState* gc_init(VM* vm) {
    GCState* state = (GCState*)malloc(sizeof(GCState));
    if (state == NULL) {
        fprintf(stderr, "Error: Out of memory for GC state\n");
        return NULL;
    }

    state->vm = vm;
    state->generational_enabled = true;
    state->write_barriers_enabled = true;

    // Initialize generation sizes
    state->young_gen_size = 16 * 1024 * 1024;  // 16 MB young generation
    state->old_gen_size = 32 * 1024 * 1024;    // 32 MB old generation
    state->young_gen_used = 0;
    state->old_gen_used = 0;

    // Set collection thresholds
    state->gc_threshold_young = (int)(state->young_gen_size * 0.7);  // Collect at 70%
    state->gc_threshold_old = (int)(state->old_gen_size * 0.8);     // Collect at 80%

    // Initialize statistics
    memset(&state->stats, 0, sizeof(GCStats));

    // Allocate young generation heap
    printf("✓ Generational GC initialized\n");
    printf("  → Young generation: %zu MB\n", state->young_gen_size / (1024 * 1024));
    printf("  → Old generation: %zu MB\n", state->old_gen_size / (1024 * 1024));
    printf("  → Write barriers: enabled\n");

    return state;
}

/**
 * Allocate memory with write barrier tracking
 */
void* gc_allocate_with_barrier(
    GCState* state,
    size_t size,
    WriteBarrierType type
) {
    if (!state) {
        fprintf(stderr, "Error: Invalid GC state\n");
        return NULL;
    }

    void* memory = malloc(size);
    if (memory == NULL) {
        fprintf(stderr, "Error: Out of memory (allocation failed)\n");
        return NULL;
    }

    state->young_gen_used += size;

    // Write barrier logging
    if (state->write_barriers_enabled) {
        switch (type) {
            case WRITE_BARRIER_FIELD:
                break;
            case WRITE_BARRIER_ARRAY_ELEMENT:
                break;
            case WRITE_BARRIER_GLOBAL:
                break;
            default:
                break;
        }
    }

    return memory;
}

/**
 * Perform a write barrier operation
 */
void gc_write_barrier(
    GCState* state,
    void* object,
    void* field,
    Value value
) {
    if (!state || !state->write_barriers_enabled) {
        return;
    }

    // Check if object is in young generation and needs promotion
    // TODO: Implement actual write barrier logic
}

/**
 * Collect young generation
 */
static size_t gc_collect_young(GCState* state) {
    size_t collected = 0;

    // TODO: Implement young generation collection
    // 1. Scan young generation for live objects
    // 2. Promote live objects to old generation
    // 3. Free dead objects
    // 4. Update generation used sizes

    printf("  → Young generation collected %zu objects\n", collected);
    state->stats.collections_young++;

    return collected;
}

/**
 * Collect old generation
 */
static size_t gc_collect_old(GCState* state) {
    size_t collected = 0;

    // TODO: Implement old generation collection
    // 1. Scan old generation for live objects
    // 2. Free dead objects
    // 3. Update old generation used size

    printf("  → Old generation collected %zu objects\n", collected);
    state->stats.collections_old++;

    return collected;
}

/**
 * Trigger garbage collection
 */
size_t gc_collect(GCState* state, bool full_collection) {
    if (!state) {
        return 0;
    }

    size_t total_collected = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (full_collection) {
        // Collect both generations
        total_collected += gc_collect_old(state);
        total_collected += gc_collect_young(state);
    } else if (state->generational_enabled) {
        // Check if young generation needs collection
        if (state->young_gen_used >= (size_t)state->gc_threshold_young) {
            total_collected += gc_collect_young(state);
        }

        // Check if old generation needs collection
        if (state->old_gen_used >= (size_t)state->gc_threshold_old) {
            total_collected += gc_collect_old(state);
        }
    } else {
        // Non-generational: simple mark-sweep
        total_collected += gc_collect_young(state);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long duration_us = (end.tv_sec - start.tv_sec) * 1000000 +
                         (end.tv_nsec - start.tv_nsec) / 1000;

    if (state->generational_enabled) {
        state->stats.young_gen_pause_us += duration_us;
    } else {
        state->stats.old_gen_pause_us += duration_us;
    }

    state->stats.objects_collected += total_collected;

    return total_collected;
}

/**
 * Force full garbage collection
 */
size_t gc_collect_full(GCState* state) {
    if (!state) {
        return 0;
    }

    printf("✓ Full garbage collection triggered\n");
    return gc_collect(state, true);
}

/**
 * Get GC statistics
 */
void gc_get_statistics(GCState* state, GCStats* stats) {
    if (!state || !stats) {
        return;
    }

    memcpy(stats, &state->stats, sizeof(GCStats));

    printf("=== GC Statistics ===\n");
    printf("Collections (young): %zu\n", state->stats.collections_young);
    printf("Collections (old): %zu\n", state->stats.collections_old);
    printf("Objects collected: %zu\n", state->stats.objects_collected);
    printf("Bytes freed: %zu\n", state->stats.bytes_freed);
    printf("Pause time (young avg): %ld us\n",
           state->stats.collections_young > 0 ?
               state->stats.young_gen_pause_us / state->stats.collections_young : 0);
    printf("Pause time (old avg): %ld us\n",
           state->stats.collections_old > 0 ?
               state->stats.old_gen_pause_us / state->stats.collections_old : 0);
    printf("=== End GC Stats ===\n");
}

/**
 * Enable/disable generational GC
 */
void gc_set_generational(GCState* state, bool enabled) {
    if (!state) {
        return;
    }

    state->generational_enabled = enabled;
    printf("✓ Generational GC: %s\n", enabled ? "enabled" : "disabled");
}

/**
 * Enable/disable write barriers
 */
void gc_set_write_barriers(GCState* state, bool enabled) {
    if (!state) {
        return;
    }

    state->write_barriers_enabled = enabled;
    printf("✓ Write barriers: %s\n", enabled ? "enabled" : "disabled");
}

/**
 * Check if pointer is in old generation
 */
bool gc_is_old_generation(GCState* state, void* ptr) {
    // TODO: Implement old generation tracking
    if (!state || !state->generational_enabled) {
        return false;
    }

    return false;
}

/**
 * Get current memory usage
 */
void gc_get_memory_usage(
    GCState* state,
    size_t* young_used,
    size_t* old_used,
    size_t* total_used
) {
    if (!state) {
        *young_used = 0;
        *old_used = 0;
        *total_used = 0;
        return;
    }

    *young_used = state->young_gen_used;
    *old_used = state->old_gen_used;
    *total_used = state->young_gen_used + state->old_gen_used;

    printf("=== Memory Usage ===\n");
    printf("Young generation: %zu / %zu bytes\n", state->young_gen_used, state->young_gen_size);
    printf("Old generation: %zu / %zu bytes\n", state->old_gen_used, state->old_gen_size);
    printf("Total: %zu / %zu bytes (%.1f%%)\n",
           *total_used, state->young_gen_size + state->old_gen_size,
           100.0 * (double)*total_used / (double)(state->young_gen_size + state->old_gen_size));
    printf("=== End Memory ===\n");
}

/**
 * Cleanup and free GC state
 */
void gc_cleanup(GCState* state) {
    if (!state) {
        return;
    }

    // Print final statistics
    printf("=== GC Final Statistics ===\n");
    GCStats stats;
    gc_get_statistics(state, &stats);

    free(state);
    printf("✓ Generational GC shutdown complete\n");
}
