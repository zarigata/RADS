#ifndef RADS_DEBUG_ENHANCED_H
#define RADS_DEBUG_ENHANCED_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct VM VM;
typedef struct Value Value;

// ============================================================================
// ENHANCED DEBUG PROTOCOL - Conditional Breakpoints, Watch Expressions, Call Stack
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// Conditional breakpoint types
typedef enum {
    BREAKPOINT_TYPE_SIMPLE = 0,
    BREAKPOINT_TYPE_CONDITIONAL = 1,
    BREAKPOINT_TYPE_TEMPORARY = 2
} BreakpointType;

// Conditional breakpoint
typedef struct Breakpoint {
    int id;
    char* function_name;
    int line_number;
    char* condition;
    int hit_count;
    int ignore_count;
    bool enabled;
    struct Breakpoint* next;
} Breakpoint;

// Watch expression
typedef struct Watch {
    char* expression;
    Value* last_value;
    int watch_id;
    struct Watch* next;
} Watch;

// Call stack frame
typedef struct StackFrame {
    int frame_id;
    char* function_name;
    int line_number;
    int instruction_pointer;
    Value* locals;
    struct StackFrame* next;
} StackFrame;

// Debug state
typedef struct {
    Breakpoint* breakpoints;
    Watch* watches;
    StackFrame* stack_frames;
    int current_frame;
    bool step_mode;
    bool exception_break_enabled;
} DebugState;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize enhanced debugger
 *
 * @param vm Pointer to the virtual machine
 * @return Debug state pointer on success, NULL on failure
 */
DebugState* debug_init(VM* vm);

/**
 * Set conditional breakpoint
 *
 * @param state Debug state
 * @param function_name Function name
 * @param line_number Line number
 * @param condition Conditional expression (can be NULL)
 * @return Breakpoint ID, or -1 on failure
 */
int debug_set_conditional_breakpoint(
    DebugState* state,
    const char* function_name,
    int line_number,
    const char* condition
);

/**
 * Remove breakpoint
 *
 * @param state Debug state
 * @param breakpoint_id Breakpoint ID to remove
 * @return true on success
 */
bool debug_remove_breakpoint(DebugState* state, int breakpoint_id);

/**
 * Add watch expression
 *
 * @param state Debug state
 * @param expression Expression to watch
 * @return Watch ID, or -1 on failure
 */
int debug_add_watch(DebugState* state, const char* expression);

/**
 * Remove watch expression
 *
 * @param state Debug state
 * @param watch_id Watch ID to remove
 * @return true on success
 */
bool debug_remove_watch(DebugState* state, int watch_id);

/**
 * Step into function
 *
 * @param state Debug state
 * @return true on success
 */
bool debug_step_into(DebugState* state);

/**
 * Step over current statement
 *
 * @param state Debug state
 * @return true on success
 */
bool debug_step_over(DebugState* state);

/**
 * Step out of current function
 *
 * @param state Debug state
 * @return true on success
 */
bool debug_step_out(DebugState* state);

/**
 * Get current call stack
 *
 * @param state Debug state
 * @param[out] frame_count Number of stack frames
 * @return Array of stack frames, or NULL on failure
 */
StackFrame** debug_get_stack_trace(DebugState* state, int* frame_count);

/**
 * Select stack frame
 *
 * @param state Debug state
 * @param frame_id Frame ID to select
 * @return true on success
 */
bool debug_select_frame(DebugState* state, int frame_id);

/**
 * Enable/disable exception break
 *
 * @param state Debug state
 * @param enabled true to break on exceptions
 */
void debug_set_exception_break(DebugState* state, bool enabled);

/**
 * Cleanup and free debug state
 *
 * @param state Debug state to free
 */
void debug_cleanup(DebugState* state);

#endif // RADS_DEBUG_ENHANCED_H
