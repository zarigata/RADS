#include "stdlib_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* ── Logging Module ──
 * Python-inspired logging for RADS applications.
 * Multiple log levels, a format template, and optional file output.
 * Designed lightweight for embedded/32-bit systems (VW Fusca rule).
 *
 * Log levels (matching Python's logging):
 *   CRITICAL = 50, ERROR = 40, WARNING = 30, INFO = 20, DEBUG = 10, NOTSET = 0
 *
 * Functions (registered as "logging.<name>"):
 * - logging.debug(msg)          - log at DEBUG level
 * - logging.info(msg)           - log at INFO level
 * - logging.warning(msg)        - log at WARNING level
 * - logging.error(msg)          - log at ERROR level
 * - logging.critical(msg)       - log at CRITICAL level
 * - logging.log(level, msg)     - log at a custom numeric level
 * - logging.setLevel(level)     - set minimum level that gets emitted
 * - logging.getLevel()          - current minimum level
 * - logging.getLevelName(level) - level number -> name string
 * - logging.basicConfig(...)    - optional (filename, level, format, timestamp)
 * Constants logging.DEBUG/INFO/... are zero-arg natives (same pattern as math.pi).
 */

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern void register_native(const char* name, NativeFn fn);

/* ── Constants ── */
#define LOG_NOTSET    0
#define LOG_DEBUG    10
#define LOG_INFO     20
#define LOG_WARNING  30
#define LOG_ERROR    40
#define LOG_CRITICAL 50

/* ── Module state (single root logger, like Python's default) ── */
static int g_log_level = LOG_INFO;
static FILE* g_log_file = NULL;
static char g_log_format[256] = "{level}: {message}";
static int g_include_timestamp = 0;

/* Level number -> canonical name; unknown levels get "LEVEL_n" */
static const char* level_name(int level) {
    switch (level) {
        case LOG_DEBUG:    return "DEBUG";
        case LOG_INFO:     return "INFO";
        case LOG_WARNING:  return "WARNING";
        case LOG_ERROR:    return "ERROR";
        case LOG_CRITICAL: return "CRITICAL";
        case LOG_NOTSET:   return "NOTSET";
        default: {
            static char buf[16];
            snprintf(buf, sizeof(buf), "LEVEL_%d", level);
            return buf;
        }
    }
}

/* Format {level}/{message}/{time}/{name} template and emit to stderr
 * (and file if configured). Messages longer than the buffer are truncated. */
static void emit_log(int level, const char* msg) {
    if (level < g_log_level) return;

    char ts[32] = "";
    if (g_include_timestamp) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    char output[1024];
    const char* fmt = g_log_format;
    char* out = output;
    const char* end = output + sizeof(output) - 1;

    while (*fmt && out < end) {
        if (strncmp(fmt, "{level}", 7) == 0) {
            out += snprintf(out, (size_t)(end - out) + 1, "%s", level_name(level));
            fmt += 7;
        } else if (strncmp(fmt, "{message}", 9) == 0) {
            out += snprintf(out, (size_t)(end - out) + 1, "%s", msg);
            fmt += 9;
        } else if (strncmp(fmt, "{time}", 6) == 0) {
            out += snprintf(out, (size_t)(end - out) + 1, "%s", ts);
            fmt += 6;
        } else if (strncmp(fmt, "{name}", 6) == 0) {
            out += snprintf(out, (size_t)(end - out) + 1, "%s", "root");
            fmt += 6;
        } else {
            *out++ = *fmt++;
        }
    }
    *out = '\0';

    fprintf(stderr, "%s\n", output);
    if (g_log_file) {
        fprintf(g_log_file, "%s\n", output);
        fflush(g_log_file);
    }
}

/* String view of a Value for message interpolation; non-strings log as "" */
static const char* arg_string(Value* v) {
    return v->type == VAL_STRING ? v->string_val : "";
}

/* ── Native functions ── */

Value stdlib_logging_debug(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: logging.debug(msg) needs a message string.\n"); return make_null(); }
    emit_log(LOG_DEBUG, arg_string(&args[0]));
    return make_null();
}

Value stdlib_logging_info(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: logging.info(msg) needs a message string.\n"); return make_null(); }
    emit_log(LOG_INFO, arg_string(&args[0]));
    return make_null();
}

Value stdlib_logging_warning(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: logging.warning(msg) needs a message string.\n"); return make_null(); }
    emit_log(LOG_WARNING, arg_string(&args[0]));
    return make_null();
}

Value stdlib_logging_error(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: logging.error(msg) needs a message string.\n"); return make_null(); }
    emit_log(LOG_ERROR, arg_string(&args[0]));
    return make_null();
}

Value stdlib_logging_critical(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: logging.critical(msg) needs a message string.\n"); return make_null(); }
    emit_log(LOG_CRITICAL, arg_string(&args[0]));
    return make_null();
}

Value stdlib_logging_log(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) { fprintf(stderr, "Error: logging.log(level, msg) needs a level and a message.\n"); return make_null(); }
    int level = args[0].type == VAL_INT ? (int)args[0].int_val : LOG_INFO;
    emit_log(level, arg_string(&args[1]));
    return make_null();
}

Value stdlib_logging_set_level(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) {
        fprintf(stderr, "Error: logging.setLevel(level) needs an int level (e.g. logging.DEBUG).\n");
        return make_null();
    }
    g_log_level = (int)args[0].int_val;
    return make_null();
}

Value stdlib_logging_get_level(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_int(g_log_level);
}

Value stdlib_logging_get_level_name(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) {
        fprintf(stderr, "Error: logging.getLevelName(level) needs an int level.\n");
        return make_null();
    }
    return make_string(level_name((int)args[0].int_val));
}

/* basicConfig(filename?, level?, format?, timestamp?) — all optional.
 * positional form mirrors the original module; a plain string opens a log file. */
Value stdlib_logging_basic_config(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc >= 1 && args[0].type == VAL_STRING) {
        if (g_log_file) fclose(g_log_file);
        g_log_file = fopen(args[0].string_val, "a");
        if (!g_log_file) {
            fprintf(stderr, "Error: logging.basicConfig: could not open '%s' for writing.\n", args[0].string_val);
            return make_null();
        }
    }
    if (argc >= 2 && args[1].type == VAL_INT) g_log_level = (int)args[1].int_val;
    if (argc >= 3 && args[2].type == VAL_STRING) {
        strncpy(g_log_format, args[2].string_val, sizeof(g_log_format) - 1);
        g_log_format[sizeof(g_log_format) - 1] = '\0';
    }
    if (argc >= 4 && args[3].type == VAL_INT) g_include_timestamp = args[3].int_val != 0;
    return make_null();
}

/* ── Constants as zero-arg natives (pattern borrowed from math.pi) ── */
Value stdlib_logging_debug_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args; return make_int(LOG_DEBUG);
}
Value stdlib_logging_info_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args; return make_int(LOG_INFO);
}
Value stdlib_logging_warning_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args; return make_int(LOG_WARNING);
}
Value stdlib_logging_error_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args; return make_int(LOG_ERROR);
}
Value stdlib_logging_critical_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args; return make_int(LOG_CRITICAL);
}
Value stdlib_logging_notset_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args; return make_int(LOG_NOTSET);
}

/* ── Registration ── */
void stdlib_logging_register(void) {
    register_native("logging.debug", (NativeFn)stdlib_logging_debug);
    register_native("logging.info", (NativeFn)stdlib_logging_info);
    register_native("logging.warning", (NativeFn)stdlib_logging_warning);
    register_native("logging.error", (NativeFn)stdlib_logging_error);
    register_native("logging.critical", (NativeFn)stdlib_logging_critical);
    register_native("logging.log", (NativeFn)stdlib_logging_log);
    register_native("logging.setLevel", (NativeFn)stdlib_logging_set_level);
    register_native("logging.getLevel", (NativeFn)stdlib_logging_get_level);
    register_native("logging.getLevelName", (NativeFn)stdlib_logging_get_level_name);
    register_native("logging.basicConfig", (NativeFn)stdlib_logging_basic_config);

    register_native("logging.DEBUG", (NativeFn)stdlib_logging_debug_val);
    register_native("logging.INFO", (NativeFn)stdlib_logging_info_val);
    register_native("logging.WARNING", (NativeFn)stdlib_logging_warning_val);
    register_native("logging.ERROR", (NativeFn)stdlib_logging_error_val);
    register_native("logging.CRITICAL", (NativeFn)stdlib_logging_critical_val);
    register_native("logging.NOTSET", (NativeFn)stdlib_logging_notset_val);
}
