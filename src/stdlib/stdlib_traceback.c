#define _POSIX_C_SOURCE 200809L
#include "stdlib_traceback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <execinfo.h>
#include <unistd.h>

/* ── traceback Module ──
 * Stack trace and error context utilities.
 * Uses glibc backtrace() on POSIX for native stack unwinding.
 * Provides formatted output and structured trace data.
 */

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ── traceback.stack(skip=0) → array of frame strings ── */
Value native_traceback_stack(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int skip = 2; /* skip this function + backtrace itself */
    if (argc >= 1 && args[0].type == VAL_INT) {
        skip += (int)args[0].int_val;
        if (skip < 0) skip = 0;
    }

    void* buf[64];
    int nframes = backtrace(buf, 64);
    if (nframes <= skip) return make_null();

    char** symbols = backtrace_symbols(buf, nframes);
    if (!symbols) return make_null();

    Array* arr = array_create((size_t)(nframes - skip));
    for (int i = skip; i < nframes; i++) {
        array_push(arr, make_string(symbols[i]));
    }
    free(symbols);

    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── traceback.format(skip=0) → formatted string ── */
Value native_traceback_format(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int skip = 2;
    if (argc >= 1 && args[0].type == VAL_INT) {
        skip += (int)args[0].int_val;
        if (skip < 0) skip = 0;
    }

    void* buf[64];
    int nframes = backtrace(buf, 64);
    if (nframes <= skip) return make_string("(no frames)");

    char** symbols = backtrace_symbols(buf, nframes);
    if (!symbols) return make_string("(no symbols)");

    /* Build formatted output */
    size_t cap = 4096;
    char* out = (char*)malloc(cap);
    if (!out) { free(symbols); return make_string("(out of memory)"); }
    size_t pos = 0;
    int frame_num = 0;

    for (int i = skip; i < nframes && pos < cap - 256; i++) {
        int written = snprintf(out + pos, cap - pos, "  #%d  %s\n", frame_num, symbols[i]);
        if (written > 0) pos += (size_t)written;
        frame_num++;
    }
    free(symbols);

    Value v = make_string(out);
    free(out);
    return v;
}

/* ── traceback.depth() → int (number of frames) ── */
Value native_traceback_depth(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    void* buf[1];
    /* backtrace with 1 frame just to count — actually need a larger buffer */
    void* big[128];
    int n = backtrace(big, 128);
    return make_int(n - 1); /* subtract this function */
}

/* ── traceback.timestamp() → ISO 8601 string ── */
Value native_traceback_timestamp(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm_info);
    return make_string(buf);
}

/* ── traceback.context(message) → formatted error context string ── */
Value native_traceback_context(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    const char* message = "unknown error";
    if (argc >= 1 && args[0].type == VAL_STRING) {
        message = args[0].string_val;
    }

    /* Get timestamp */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", tm_info);

    /* Get minimal stack trace */
    void* buf[8];
    int n = backtrace(buf, 8);

    size_t cap = 2048;
    char* out = (char*)malloc(cap);
    if (!out) return make_string(message);
    snprintf(out, cap, "Traceback (most recent call last):\n  Time: %s\n  Error: %s\n  Frames: %d\n",
             ts, message, n > 2 ? n - 2 : 0);

    Value v = make_string(out);
    free(out);
    return v;
}

/* ── traceback.clear() → null (no-op, for API compatibility) ── */
Value native_traceback_clear(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    /* No-op: in a real implementation this would clear stored trace state */
    return make_null();
}

void stdlib_traceback_register(void) {
    register_native("traceback.stack", native_traceback_stack);
    register_native("traceback.format", native_traceback_format);
    register_native("traceback.depth", native_traceback_depth);
    register_native("traceback.timestamp", native_traceback_timestamp);
    register_native("traceback.context", native_traceback_context);
    register_native("traceback.clear", native_traceback_clear);
}
