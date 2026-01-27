#include "debug_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DEBUG_MAX_BREAKPOINTS 128
#define DEBUG_MAX_STACK 256
#define DEBUG_MAX_VARS 256

void debug_init(DebugContext* ctx) {
    ctx->breakpoints = (Breakpoint*)malloc(DEBUG_MAX_BREAKPOINTS * sizeof(Breakpoint));
    ctx->breakpoint_count = 0;
    ctx->breakpoint_capacity = DEBUG_MAX_BREAKPOINTS;
    ctx->is_paused = false;
    ctx->is_running = false;

    ctx->call_stack = (DebugStackFrame*)malloc(DEBUG_MAX_STACK * sizeof(DebugStackFrame));
    ctx->stack_capacity = DEBUG_MAX_STACK;
    ctx->stack_depth = 0;

    ctx->on_breakpoint = NULL;
    ctx->on_step = NULL;
    ctx->user_data = NULL;
}

void debug_free(DebugContext* ctx) {
    if (!ctx) return;

    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].condition) free(ctx->breakpoints[i].condition);
        if (ctx->breakpoints[i].log_message) free(ctx->breakpoints[i].log_message);
    }

    free(ctx->breakpoints);
    free(ctx->call_stack);
}

void debug_set_breakpoint_handler(DebugContext* ctx, DebugBreakpointHitHandler handler, void* user_data) {
    if (!ctx) return;
    ctx->on_breakpoint = handler;
    ctx->user_data = user_data;
}

void debug_set_step_handler(DebugContext* ctx, DebugStepCompleteHandler handler, void* user_data) {
    if (!ctx) return;
    ctx->on_step = handler;
    ctx->user_data = user_data;
}

Breakpoint* debug_find_breakpoint(DebugContext* ctx, const char* file, int line) {
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].line == line && strcmp(ctx->breakpoints[i].file, file) == 0) {
            return &ctx->breakpoints[i];
        }
    }
    return NULL;
}

Breakpoint* debug_add_breakpoint(DebugContext* ctx, const char* file, int line) {
    if (ctx->breakpoint_count >= DEBUG_MAX_BREAKPOINTS) {
        return NULL;
    }

    Breakpoint* bp = &ctx->breakpoints[ctx->breakpoint_count];
    bp->file = strdup(file);
    bp->line = line;
    bp->column = 0;
    bp->line_end = line;
    bp->column_end = 0;
    bp->verified = false;
    bp->condition = NULL;
    bp->log_message = NULL;
    bp->hit_count = 0;
    bp->column = 0;
    bp->line_end = line;
    bp->column_end = 0;
    bp->verified = false;
    bp->condition = NULL;
    bp->log_message = NULL;
    bp->hit_count = 0;

    ctx->breakpoint_count++;
    return bp;
}

void debug_remove_breakpoint(DebugContext* ctx, const char* file, int line) {
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].line == line && strcmp(ctx->breakpoints[i].file, file) == 0) {
            if (ctx->breakpoints[i].condition) free(ctx->breakpoints[i].condition);
            if (ctx->breakpoints[i].log_message) free(ctx->breakpoints[i].log_message);

            for (int j = i; j < ctx->breakpoint_count - 1; j++) {
                ctx->breakpoints[j] = ctx->breakpoints[j + 1];
            }

            ctx->breakpoint_count--;
            break;
        }
    }
}

void debug_step_into(DebugContext* ctx) {
    if (!ctx || !ctx->is_paused) return;

    ctx->is_paused = false;
    printf("Step into\n");

    if (ctx->on_step) {
        ctx->on_step(ctx->user_data);
    }
}

void debug_step_over(DebugContext* ctx) {
    if (!ctx || !ctx->is_paused) return;

    ctx->is_paused = false;
    printf("Step over\n");

    if (ctx->on_step) {
        ctx->on_step(ctx->user_data);
    }
}

void debug_step_out(DebugContext* ctx) {
    if (!ctx || !ctx->is_paused) return;

    ctx->is_paused = false;
    printf("Step out\n");

    if (ctx->on_step) {
        ctx->on_step(ctx->user_data);
    }
}

void debug_continue(DebugContext* ctx) {
    if (!ctx || !ctx->is_paused) return;

    ctx->is_paused = false;
    printf("Continue\n");

    if (ctx->on_step) {
        ctx->on_step(ctx->user_data);
    }
}

void debug_pause(DebugContext* ctx) {
    if (!ctx) return;

    ctx->is_paused = true;
    printf("Paused\n");
}

DebugVariable* debug_get_locals(DebugContext* ctx, int* count) {
    *count = 0;
    return NULL;
}

DebugStackFrame* debug_get_stack_trace(DebugContext* ctx, int* count) {
    *count = ctx->stack_depth;
    return ctx->call_stack;
}

char* debug_evaluate(DebugContext* ctx, const char* expression) {
    return strdup(expression);
}

void debug_clear_all_breakpoints(DebugContext* ctx) {
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].condition) free(ctx->breakpoints[i].condition);
        if (ctx->breakpoints[i].log_message) free(ctx->breakpoints[i].log_message);
    }

    ctx->breakpoint_count = 0;
}

void debug_clear_call_stack(DebugContext* ctx) {
    ctx->stack_depth = 0;
}
