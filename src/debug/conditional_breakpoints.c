#include "debug_enhanced.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// ENHANCED DEBUG PROTOCOL - Conditional Breakpoints, Watch Expressions, Call Stack
// RADS v0.0.7 "DARK MOON"
// ============================================================================

static int next_breakpoint_id = 1;
static int next_watch_id = 1;
static int next_frame_id = 1;

/**
 * Initialize enhanced debugger
 */
DebugState* debug_init(VM* vm) {
    DebugState* state = (DebugState*)malloc(sizeof(DebugState));
    if (state == NULL) {
        fprintf(stderr, "Error: Out of memory for debug state\n");
        return NULL;
    }

    state->breakpoints = NULL;
    state->watches = NULL;
    state->stack_frames = NULL;
    state->current_frame = 0;
    state->step_mode = false;
    state->exception_break_enabled = false;

    printf("✓ Enhanced debugger initialized\n");
    return state;
}

/**
 * Set conditional breakpoint
 */
int debug_set_conditional_breakpoint(
    DebugState* state,
    const char* function_name,
    int line_number,
    const char* condition
) {
    if (!state || !function_name) {
        fprintf(stderr, "Error: Invalid parameters for breakpoint\n");
        return -1;
    }

    // Create new breakpoint
    Breakpoint* bp = (Breakpoint*)malloc(sizeof(Breakpoint));
    if (bp == NULL) {
        fprintf(stderr, "Error: Out of memory for breakpoint\n");
        return -1;
    }

    bp->id = next_breakpoint_id++;
    bp->function_name = strdup(function_name);
    bp->line_number = line_number;
    bp->condition = condition ? strdup(condition) : NULL;
    bp->hit_count = 0;
    bp->ignore_count = 0;
    bp->enabled = true;
    bp->next = state->breakpoints;

    state->breakpoints = bp;

    printf("  → Conditional breakpoint set: %s:%d (condition: %s)\n",
           function_name, line_number, condition ? condition : "always");

    return bp->id;
}

/**
 * Remove breakpoint
 */
bool debug_remove_breakpoint(DebugState* state, int breakpoint_id) {
    if (!state) {
        return false;
    }

    Breakpoint* prev = NULL;
    Breakpoint* current = state->breakpoints;

    while (current != NULL) {
        if (current->id == breakpoint_id) {
            if (prev) {
                prev->next = current->next;
            } else {
                state->breakpoints = current->next;
            }
            free(current->function_name);
            if (current->condition) free(current->condition);
            free(current);
            printf("✓ Breakpoint removed: ID %d\n", breakpoint_id);
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}

/**
 * Add watch expression
 */
int debug_add_watch(DebugState* state, const char* expression) {
    if (!state || !expression) {
        fprintf(stderr, "Error: Invalid parameters for watch\n");
        return -1;
    }

    Watch* watch = (Watch*)malloc(sizeof(Watch));
    if (watch == NULL) {
        fprintf(stderr, "Error: Out of memory for watch\n");
        return -1;
    }

    watch->expression = strdup(expression);
    watch->watch_id = next_watch_id++;
    watch->next = state->watches;

    state->watches = watch;

    printf("  → Watch added: %s (ID: %d)\n", expression, watch->watch_id);
    return watch->watch_id;
}

/**
 * Remove watch expression
 */
bool debug_remove_watch(DebugState* state, int watch_id) {
    if (!state) {
        return false;
    }

    Watch* prev = NULL;
    Watch* current = state->watches;

    while (current != NULL) {
        if (current->watch_id == watch_id) {
            if (prev) {
                prev->next = current->next;
            } else {
                state->watches = current->next;
            }
            free(current->expression);
            free(current);
            printf("✓ Watch removed: ID %d\n", watch_id);
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}

/**
 * Step into function
 */
bool debug_step_into(DebugState* state) {
    if (!state) {
        return false;
    }

    printf("  → Step into\n");
    // TODO: Implement actual step logic
    return true;
}

/**
 * Step over current statement
 */
bool debug_step_over(DebugState* state) {
    if (!state) {
        return false;
    }

    printf("  → Step over\n");
    // TODO: Implement actual step logic
    return true;
}

/**
 * Step out of current function
 */
bool debug_step_out(DebugState* state) {
    if (!state) {
        return false;
    }

    printf("  → Step out\n");
    // TODO: Implement actual step logic
    return true;
}

/**
 * Get current call stack
 */
StackFrame** debug_get_stack_trace(DebugState* state, int* frame_count) {
    if (!state || !frame_count) {
        if (frame_count) *frame_count = 0;
        return NULL;
    }

    // Count frames
    StackFrame* current = state->stack_frames;
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    *frame_count = count;

    // Allocate array
    StackFrame** frames = (StackFrame**)malloc(sizeof(StackFrame*) * count);
    if (frames == NULL) {
        fprintf(stderr, "Error: Out of memory for stack trace\n");
        return NULL;
    }

    // Copy frames
    current = state->stack_frames;
    for (int i = 0; i < count; i++) {
        frames[i] = current;
        current = current->next;
    }

    return frames;
}

/**
 * Select stack frame
 */
bool debug_select_frame(DebugState* state, int frame_id) {
    if (!state) {
        return false;
    }

    printf("  → Selected frame: %d\n", frame_id);
    state->current_frame = frame_id;
    // TODO: Update locals display
    return true;
}

/**
 * Enable/disable exception break
 */
void debug_set_exception_break(DebugState* state, bool enabled) {
    if (!state) {
        return;
    }

    state->exception_break_enabled = enabled;
    printf("✓ Exception break: %s\n", enabled ? "enabled" : "disabled");
}

/**
 * Cleanup and free debug state
 */
void debug_cleanup(DebugState* state) {
    if (!state) {
        return;
    }

    // Free breakpoints
    Breakpoint* bp = state->breakpoints;
    while (bp != NULL) {
        Breakpoint* next = bp->next;
        free(bp->function_name);
        if (bp->condition) free(bp->condition);
        free(bp);
        bp = next;
    }

    // Free watches
    Watch* watch = state->watches;
    while (watch != NULL) {
        Watch* next = watch->next;
        free(watch->expression);
        free(watch);
        watch = next;
    }

    free(state);
    printf("✓ Enhanced debugger shutdown complete\n");
}
