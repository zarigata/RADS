#define _POSIX_C_SOURCE 199309L
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#include "stdlib_time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* ── Time Module ──
 * Provides timing and sleep functions similar to Python's time module.
 * Cross-platform support for all devices including 32-bit systems.
 *
 * Functions:
 * - time.sleep(seconds)     - sleep for specified seconds (float supported)
 * - time.time()             - Unix timestamp in seconds since epoch
 * - time.perf_counter()     - high-resolution monotonic clock for benchmarks
 * - time.monotonic()        - monotonic clock (never goes backward)
 * - time.clock()            - CPU time used by the process
 * - time.process_time()     - high-resolution process time
 * - time.thread_time()      - thread-specific CPU time (may be same as process_time)
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Value make_error(const char* msg);

/* ── Helper: Get high-resolution timestamp in seconds ── */
static double get_perf_counter(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
#endif
}

/* ── Helper: Get process CPU time in seconds ── */
static double get_process_time(void) {
#if defined(CLOCK_PROCESS_CPUTIME_ID)
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

/* ── time.sleep(seconds) ── */
Value native_time_sleep(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        return make_error("sleep() requires 1 argument: seconds");
    }

    double seconds = 0.0;
    if (args[0].type == VAL_INT) {
        seconds = (double)args[0].int_val;
    } else if (args[0].type == VAL_FLOAT) {
        seconds = args[0].float_val;
    } else {
        return make_error("sleep() argument must be a number");
    }

    if (seconds < 0) {
        return make_error("sleep() argument must be non-negative");
    }

    /* Convert to seconds and nanoseconds */
    time_t secs = (time_t)seconds;
    long nsecs = (long)((seconds - (double)secs) * 1e9);

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309L
    /* Use nanosleep for high-precision sleep */
    struct timespec req, rem;
    req.tv_sec = secs;
    req.tv_nsec = nsecs;
    while (nanosleep(&req, &rem) == -1) {
        /* Interrupted by signal, continue with remaining time */
        req = rem;
    }
#else
    /* Fallback to usleep */
    usleep((useconds_t)(seconds * 1e6));
#endif

    return make_null();
}

/* ── time.time() ── Unix timestamp in seconds since epoch ── */
Value native_time_time(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
#if defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return make_float((double)ts.tv_sec + (double)ts.tv_nsec * 1e-9);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return make_float((double)tv.tv_sec + (double)tv.tv_usec * 1e-6);
#endif
}

/* ── time.perf_counter() ── High-resolution monotonic clock for benchmarks ── */
Value native_time_perf_counter(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
    return make_float(get_perf_counter());
}

/* ── time.monotonic() ── Monotonic clock (never goes backward) ── */
Value native_time_monotonic(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
    return make_float(get_perf_counter());
}

/* ── time.clock() ── CPU time used by the process ── */
Value native_time_clock(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
    return make_float(get_process_time());
}

/* ── time.process_time() ── High-resolution process time ── */
Value native_time_process_time(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
    return make_float(get_process_time());
}

/* ── time.thread_time() ── Thread-specific CPU time ── */
Value native_time_thread_time(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
#if defined(CLOCK_THREAD_CPUTIME_ID)
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return make_float((double)ts.tv_sec + (double)ts.tv_nsec * 1e-9);
#else
    /* Fallback to process time if thread-specific timing unavailable */
    return make_float(get_process_time());
#endif
}

/* ── time.unix() ── Alias for time.time() for convenience ── */
Value native_time_unix(struct Interpreter* interp, int argc, Value* args) {
    return native_time_time(interp, argc, args);
}

/* ── time.millis() ── Unix timestamp in milliseconds ── */
Value native_time_millis(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
#if defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return make_float((double)(ts.tv_sec * 1000LL) + (double)ts.tv_nsec / 1e6);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return make_float((double)(tv.tv_sec * 1000LL) + (double)tv.tv_usec / 1e3);
#endif
}

/* ── time.micros() ── Unix timestamp in microseconds ── */
Value native_time_micros(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc;
#if defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return make_float((double)(ts.tv_sec * 1000000LL) + (double)ts.tv_nsec / 1e3);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return make_float((double)(tv.tv_sec * 1000000LL) + (double)tv.tv_usec);
#endif
}

/* ── Register all time functions ── */
void stdlib_time_register(void) {
    /* Basic timing functions */
    register_native("time.sleep", native_time_sleep);
    register_native("time.time", native_time_time);
    register_native("time.perf_counter", native_time_perf_counter);
    register_native("time.monotonic", native_time_monotonic);
    register_native("time.clock", native_time_clock);
    register_native("time.process_time", native_time_process_time);
    register_native("time.thread_time", native_time_thread_time);

    /* Convenience aliases */
    register_native("time.unix", native_time_unix);
    register_native("time.millis", native_time_millis);
    register_native("time.micros", native_time_micros);
}
