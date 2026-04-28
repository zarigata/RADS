#include "stdlib_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* ── Logging Module ──
 * Python-inspired logging for RADS applications.
 * Supports multiple log levels, formatters, and output targets.
 * Designed to be lightweight for embedded/32-bit systems.
 *
 * Log Levels (matching Python):
 *   CRITICAL = 50, ERROR = 40, WARNING = 30, INFO = 20, DEBUG = 10, NOTSET = 0
 *
 * Functions:
 * - logging.debug(msg)          - Log at DEBUG level
 * - logging.info(msg)           - Log at INFO level
 * - logging.warning(msg)        - Log at WARNING level
 * - logging.error(msg)          - Log at ERROR level
 * - logging.critical(msg)       - Log at CRITICAL level
 * - logging.log(level, msg)     - Log at custom level
 * - logging.setLevel(level)     - Set minimum log level
 * - logging.getLevel()          - Get current log level
 * - logging.basicConfig(opts)   - Configure logging (file, level, format)
 * - logging.getLevelName(level) - Get level name from number
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Value make_error(const char* msg);

/* ── Constants ── */
#define LOG_NOTSET    0
#define LOG_DEBUG    10
#define LOG_INFO     20
#define LOG_WARNING  30
#define LOG_ERROR    40
#define LOG_CRITICAL 50

/* ── Module state ── */
static int g_log_level = LOG_INFO;
static FILE* g_log_file = NULL;
static char g_log_format[256] = "{level}: {message}";
static int g_include_timestamp = 0;

/* ── Helper: Get level name ── */
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

/* ── Helper: Format and emit log message ── */
static void emit_log(int level, const char* msg) {
    if (level < g_log_level) return;

    /* Get timestamp */
    char ts[32] = "";
    if (g_include_timestamp) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    /* Build formatted message */
    char output[1024];
    const char* fmt = g_log_format;
    char* out = output;
    const char* end = output + sizeof(output) - 1;

    while (*fmt && out < end) {
        if (strncmp(fmt, "{level}", 7) == 0) {
            out += snprintf(out, end - out, "%s", level_name(level));
            fmt += 7;
        } else if (strncmp(fmt, "{message}", 9) == 0) {
            out += snprintf(out, end - out, "%s", msg);
            fmt += 9;
        } else if (strncmp(fmt, "{time}", 6) == 0) {
            out += snprintf(out, end - out, "%s", ts);
            fmt += 6;
        } else if (strncmp(fmt, "{name}", 6) == 0) {
            out += snprintf(out, end - out, "%s", "root");
            fmt += 6;
        } else {
            *out++ = *fmt++;
        }
    }
    *out = '\0';

    /* Output to stderr */
    fprintf(stderr, "%s\n", output);

    /* Output to file if configured */
    if (g_log_file) {
        fprintf(g_log_file, "%s\n", output);
        fflush(g_log_file);
    }
}

/* ── Native function wrappers ── */

static Value native_log_debug(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.debug(msg) requires 1 argument");
    emit_log(LOG_DEBUG, argv[0].type == VAL_STRING ? argv[0].as.string : "");
    return make_null();
}

static Value native_log_info(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.info(msg) requires 1 argument");
    emit_log(LOG_INFO, argv[0].type == VAL_STRING ? argv[0].as.string : "");
    return make_null();
}

static Value native_log_warning(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.warning(msg) requires 1 argument");
    emit_log(LOG_WARNING, argv[0].type == VAL_STRING ? argv[0].as.string : "");
    return make_null();
}

static Value native_log_error(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.error(msg) requires 1 argument");
    emit_log(LOG_ERROR, argv[0].type == VAL_STRING ? argv[0].as.string : "");
    return make_null();
}

static Value native_log_critical(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.critical(msg) requires 1 argument");
    emit_log(LOG_CRITICAL, argv[0].type == VAL_STRING ? argv[0].as.string : "");
    return make_null();
}

static Value native_log_log(int argc, Value* argv) {
    if (argc < 2) return make_error("logging.log(level, msg) requires 2 arguments");
    int level = (argv[0].type == VAL_INT) ? (int)argv[0].as.integer : LOG_INFO;
    const char* msg = argv[1].type == VAL_STRING ? argv[1].as.string : "";
    emit_log(level, msg);
    return make_null();
}

static Value native_set_level(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.setLevel(level) requires 1 argument");
    g_log_level = (argv[0].type == VAL_INT) ? (int)argv[0].as.integer : LOG_INFO;
    return make_null();
}

static Value native_get_level(int argc, Value* argv) {
    (void)argc; (void)argv;
    return make_int(g_log_level);
}

static Value native_get_level_name(int argc, Value* argv) {
    if (argc < 1) return make_error("logging.getLevelName(level) requires 1 argument");
    int level = (argv[0].type == VAL_INT) ? (int)argv[0].as.integer : 0;
    return make_string(level_name(level));
}

static Value native_basic_config(int argc, Value* argv) {
    /* logging.basicConfig() accepts optional named options via struct/map */
    if (argc >= 1 && argv[0].type == VAL_STRING) {
        /* Simple string arg treated as filename */
        if (g_log_file) fclose(g_log_file);
        g_log_file = fopen(argv[0].as.string, "a");
        if (!g_log_file) return make_error("logging.basicConfig: could not open file");
    }
    if (argc >= 2 && argv[1].type == VAL_INT) {
        g_log_level = (int)argv[1].as.integer;
    }
    if (argc >= 3 && argv[2].type == VAL_STRING) {
        strncpy(g_log_format, argv[2].as.string, sizeof(g_log_format) - 1);
    }
    if (argc >= 4 && argv[3].type == VAL_INT) {
        g_include_timestamp = (argv[3].as.integer != 0);
    }
    return make_null();
}

/* ── Constants as functions ── */
static Value native_logging_debug_val(int argc, Value* argv) { (void)argc; (void)argv; return make_int(LOG_DEBUG); }
static Value native_logging_info_val(int argc, Value* argv) { (void)argc; (void)argv; return make_int(LOG_INFO); }
static Value native_logging_warning_val(int argc, Value* argv) { (void)argc; (void)argv; return make_int(LOG_WARNING); }
static Value native_logging_error_val(int argc, Value* argv) { (void)argc; (void)argv; return make_int(LOG_ERROR); }
static Value native_logging_critical_val(int argc, Value* argv) { (void)argc; (void)argv; return make_int(LOG_CRITICAL); }
static Value native_logging_notset_val(int argc, Value* argv) { (void)argc; (void)argv; return make_int(LOG_NOTSET); }

/* ── Registration ── */
void stdlib_logging_register(void) {
    /* Log functions */
    register_native("logging_debug", native_log_debug);
    register_native("logging_info", native_log_info);
    register_native("logging_warning", native_log_warning);
    register_native("logging_error", native_log_error);
    register_native("logging_critical", native_log_critical);
    register_native("logging_log", native_log_log);
    register_native("logging_setLevel", native_set_level);
    register_native("logging_getLevel", native_get_level);
    register_native("logging_getLevelName", native_get_level_name);
    register_native("logging_basicConfig", native_basic_config);

    /* Constants */
    register_native("logging_DEBUG", native_logging_debug_val);
    register_native("logging_INFO", native_logging_info_val);
    register_native("logging_WARNING", native_logging_warning_val);
    register_native("logging_ERROR", native_logging_error_val);
    register_native("logging_CRITICAL", native_logging_critical_val);
    register_native("logging_NOTSET", native_logging_notset_val);
}
