#ifndef RADS_DEBUG_H
#define RADS_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DEBUG_STEP_INTO,
    DEBUG_STEP_OVER,
    DEBUG_STEP_OUT,
    DEBUG_CONTINUE,
    DEBUG_PAUSE,
    DEBUG_SET_BREAKPOINT,
    DEBUG_REMOVE_BREAKPOINT,
    DEBUG_GET_VARIABLES,
    DEBUG_GET_STACK_TRACE,
    DEBUG_EVALUATE_EXPRESSION,
    DEBUG_TERMINATE
} DebugCommand;

typedef struct {
    char* file;
    int line;
    int column;
    int line_end;
    int column_end;
    bool verified;
    char* condition;
    char* log_message;
    int hit_count;
} Breakpoint;

typedef struct {
    char* name;
    int frame_index;
    int line;
} DebugStackFrame;

typedef struct {
    char* name;
    int scope;
    char* value;
    int line;
} DebugVariable;

typedef struct {
    char* file;
    int line;
    int column;
    char* message;
} DebugStopReason;

typedef struct {
    DebugCommand command;
    Breakpoint* breakpoint;
    char* expression;
    char* variable_name;
    int frame_depth;
    bool should_continue;
} DebugRequest;

typedef struct {
    bool success;
    char* error_message;
    Breakpoint* breakpoint;
    DebugVariable* variables;
    int variable_count;
    DebugStackFrame* stack_trace;
    int stack_depth;
    char* expression_result;
    DebugStopReason* stop_reason;
    bool stopped;
    DebugVariable* variable;
} DebugResponse;

typedef void (*DebugBreakpointHitHandler)(Breakpoint* bp, void* user_data);
typedef void (*DebugStepCompleteHandler)(void* user_data);

typedef struct {
    Breakpoint* breakpoints;
    int breakpoint_count;
    int breakpoint_capacity;

    bool is_paused;
    bool is_running;

    DebugBreakpointHitHandler on_breakpoint;
    DebugStepCompleteHandler on_step;
    void* user_data;

    DebugStackFrame* call_stack;
    int stack_capacity;
    int stack_depth;
} DebugContext;

void debug_init(DebugContext* ctx);
void debug_free(DebugContext* ctx);

void debug_set_breakpoint_handler(DebugContext* ctx, DebugBreakpointHitHandler handler, void* user_data);
void debug_set_step_handler(DebugContext* ctx, DebugStepCompleteHandler handler, void* user_data);

Breakpoint* debug_add_breakpoint(DebugContext* ctx, const char* file, int line);
void debug_remove_breakpoint(DebugContext* ctx, const char* file, int line);
Breakpoint* debug_find_breakpoint(DebugContext* ctx, const char* file, int line);

void debug_step_into(DebugContext* ctx);
void debug_step_over(DebugContext* ctx);
void debug_step_out(DebugContext* ctx);
void debug_continue(DebugContext* ctx);
void debug_pause(DebugContext* ctx);

DebugVariable* debug_get_locals(DebugContext* ctx, int* count);
DebugStackFrame* debug_get_stack_trace(DebugContext* ctx, int* count);
char* debug_evaluate(DebugContext* ctx, const char* expression);

void debug_clear_all_breakpoints(DebugContext* ctx);
void debug_clear_call_stack(DebugContext* ctx);

#endif
