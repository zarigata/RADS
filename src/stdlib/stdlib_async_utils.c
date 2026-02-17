#include "../core/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

// ============================================================================
// ASYNC UTILITIES - parallel, series, retry, timeout
// RADS v0.0.7 "DARK MOON"
// ============================================================================

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void value_free(Value* value);
extern Value interpreter_execute_callback(Value func_val, int argc, Value* args);

typedef struct {
    Value callback;
    int index;
    int remaining;
    Array* results;
    Value done_callback;
    struct Interpreter* interp;
} ParallelContext;

typedef struct {
    Value callback;
    int index;
    int total;
    Array* functions;
    Array* results;
    Value done_callback;
    struct Interpreter* interp;
} SeriesContext;

typedef struct {
    Value callback;
    int retries_left;
    int delay_ms;
    int original_delay;
    Value done_callback;
    struct Interpreter* interp;
    uv_timer_t timer;
} RetryContext;

typedef struct {
    uv_timer_t timer;
    Value callback;
    Value done_callback;
    struct Interpreter* interp;
    bool completed;
} TimeoutContext;

static void parallel_callback(uv_work_t* req, int status) {
    (void)status;
    free(req);
}

static void parallel_work(uv_work_t* req) {
    (void)req;
}

Value stdlib_async_parallel(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: async.parallel() requires 2 arguments (array of functions, done callback)\n");
        return make_null();
    }

    Value funcs_val = args[0];
    Value done_callback = args[1];

    if (funcs_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: async.parallel() first argument must be an array of functions\n");
        return make_null();
    }

    Array* funcs = funcs_val.array_val;
    Array* results = array_create(funcs->count);
    
    for (size_t i = 0; i < funcs->count; i++) {
        Value func = funcs->items[i];
        if (func.type == VAL_FUNCTION) {
            Value result = interpreter_execute_callback(func, 0, NULL);
            array_push(results, result);
        } else {
            array_push(results, make_null());
        }
    }

    Value cb_args[1];
    cb_args[0].type = VAL_ARRAY;
    cb_args[0].array_val = results;
    interpreter_execute_callback(done_callback, 1, cb_args);

    return make_null();
}

Value stdlib_async_series(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: async.series() requires 2 arguments (array of functions, done callback)\n");
        return make_null();
    }

    Value funcs_val = args[0];
    Value done_callback = args[1];

    if (funcs_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: async.series() first argument must be an array of functions\n");
        return make_null();
    }

    Array* funcs = funcs_val.array_val;
    Array* results = array_create(funcs->count);
    
    for (size_t i = 0; i < funcs->count; i++) {
        Value func = funcs->items[i];
        if (func.type == VAL_FUNCTION) {
            Value result = interpreter_execute_callback(func, 0, NULL);
            array_push(results, result);
        } else {
            array_push(results, make_null());
        }
    }

    Value cb_args[1];
    cb_args[0].type = VAL_ARRAY;
    cb_args[0].array_val = results;
    interpreter_execute_callback(done_callback, 1, cb_args);

    return make_null();
}

Value stdlib_async_retry(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3) {
        fprintf(stderr, "Error: async.retry() requires 3 arguments (function, retry count, done callback)\n");
        return make_null();
    }

    Value func_val = args[0];
    Value retries_val = args[1];
    Value done_callback = args[2];

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: async.retry() first argument must be a function\n");
        return make_null();
    }

    if (retries_val.type != VAL_INT) {
        fprintf(stderr, "Error: async.retry() second argument must be an integer\n");
        return make_null();
    }

    int retries = (int)retries_val.int_val;
    Value result = make_null();
    bool success = false;

    for (int i = 0; i <= retries && !success; i++) {
        result = interpreter_execute_callback(func_val, 0, NULL);
        
        if (result.type == VAL_BOOL && result.bool_val) {
            success = true;
        } else if (result.type == VAL_INT && result.int_val != 0) {
            success = true;
        } else if (result.type != VAL_NULL) {
            success = true;
        }
        
        if (!success && i < retries) {
            uv_sleep((unsigned long)(100 * (i + 1)));
        }
    }

    Value cb_args[2] = { 
        make_bool(success),
        result
    };
    interpreter_execute_callback(done_callback, 2, cb_args);

    return make_null();
}

static void timeout_timer_cb(uv_timer_t* timer) {
    TimeoutContext* ctx = (TimeoutContext*)timer->data;
    
    if (!ctx->completed) {
        ctx->completed = true;
        Value cb_args[1] = { make_bool(false) };
        interpreter_execute_callback(ctx->done_callback, 1, cb_args);
    }
    
    free(ctx);
}

Value stdlib_async_timeout(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3) {
        fprintf(stderr, "Error: async.timeout() requires 3 arguments (function, timeout ms, done callback)\n");
        return make_null();
    }

    Value func_val = args[0];
    Value timeout_val = args[1];
    Value done_callback = args[2];

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: async.timeout() first argument must be a function\n");
        return make_null();
    }

    if (timeout_val.type != VAL_INT) {
        fprintf(stderr, "Error: async.timeout() second argument must be an integer\n");
        return make_null();
    }

    uint64_t timeout_ms = (uint64_t)timeout_val.int_val;

    TimeoutContext* ctx = malloc(sizeof(TimeoutContext));
    ctx->callback = func_val;
    ctx->done_callback = done_callback;
    ctx->interp = interp;
    ctx->completed = false;

    uv_loop_t* loop = interpreter_init_event_loop();
    uv_timer_init(loop, &ctx->timer);
    ctx->timer.data = ctx;
    uv_timer_start(&ctx->timer, timeout_timer_cb, timeout_ms, 0);

    Value result = interpreter_execute_callback(func_val, 0, NULL);
    
    if (!ctx->completed) {
        ctx->completed = true;
        uv_timer_stop(&ctx->timer);
        
        Value cb_args[2] = { 
            make_bool(true),
            result
        };
        interpreter_execute_callback(done_callback, 2, cb_args);
        free(ctx);
    }

    return make_null();
}

Value stdlib_async_delay(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: async.delay() requires 2 arguments (milliseconds, callback)\n");
        return make_null();
    }

    Value delay_val = args[0];
    Value callback = args[1];

    if (delay_val.type != VAL_INT) {
        fprintf(stderr, "Error: async.delay() first argument must be an integer\n");
        return make_null();
    }

    if (callback.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: async.delay() second argument must be a function\n");
        return make_null();
    }

    uv_sleep((unsigned long)delay_val.int_val);
    interpreter_execute_callback(callback, 0, NULL);

    return make_null();
}

Value stdlib_async_each(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3) {
        fprintf(stderr, "Error: async.each() requires 3 arguments (array, iterator function, done callback)\n");
        return make_null();
    }

    Value arr_val = args[0];
    Value iterator_fn = args[1];
    Value done_callback = args[2];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: async.each() first argument must be an array\n");
        return make_null();
    }

    if (iterator_fn.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: async.each() second argument must be a function\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;
    
    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[2] = { 
            arr->items[i],
            make_int((long long)i)
        };
        interpreter_execute_callback(iterator_fn, 2, call_args);
    }

    if (done_callback.type == VAL_FUNCTION) {
        interpreter_execute_callback(done_callback, 0, NULL);
    }

    return make_null();
}

extern void register_native(const char* name, NativeFn fn);

void stdlib_async_utils_register(void) {
    register_native("async_utils.parallel", (NativeFn)stdlib_async_parallel);
    register_native("async_utils.series", (NativeFn)stdlib_async_series);
    register_native("async_utils.retry", (NativeFn)stdlib_async_retry);
    register_native("async_utils.timeout", (NativeFn)stdlib_async_timeout);
    register_native("async_utils.delay", (NativeFn)stdlib_async_delay);
    register_native("async_utils.each", (NativeFn)stdlib_async_each);
}
