#define _POSIX_C_SOURCE 200809L
#include "stdlib_os.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <limits.h>

/* ── OS / Platform Information Module ──
 * Provides system info, platform detection, memory queries, and CPU count.
 * Portable across Linux, macOS, and POSIX systems.
 * No external dependencies beyond libc.
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* os.name() - Human-readable OS name (e.g. "Linux", "Darwin") */
Value native_os_name(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    struct utsname buf;
    if (uname(&buf) == 0) {
        return make_string(buf.sysname);
    }
    return make_string("Unknown");
}

/* os.version() - OS version/release string */
Value native_os_version(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    struct utsname buf;
    if (uname(&buf) == 0) {
        char* out = malloc(256);
        snprintf(out, 256, "%s %s", buf.release, buf.version);
        Value v;
        v.type = VAL_STRING;
        v.string_val = out;
        return v;
    }
    return make_string("unknown");
}

/* os.arch() - Machine hardware architecture (e.g. "x86_64", "aarch64") */
Value native_os_arch(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    struct utsname buf;
    if (uname(&buf) == 0) {
        return make_string(buf.machine);
    }
    return make_string("unknown");
}

/* os.hostname() - System hostname */
Value native_os_hostname(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    char buf[HOST_NAME_MAX + 1];
    if (gethostname(buf, sizeof(buf)) == 0) {
        return make_string(buf);
    }
    return make_string("unknown");
}

/* os.cwd() - Current working directory */
Value native_os_cwd(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf))) {
        return make_string(buf);
    }
    return make_null();
}

/* os.pid() - Current process ID */
Value native_os_pid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_int((long long)getpid());
}

/* os.ppid() - Parent process ID */
Value native_os_ppid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_int((long long)getppid());
}

/* os.uid() - Real user ID */
Value native_os_uid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_int((long long)getuid());
}

/* os.gid() - Real group ID */
Value native_os_gid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_int((long long)getgid());
}

/* os.env(name) - Get environment variable */
Value native_os_env(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* val = getenv(args[0].string_val);
    if (val) return make_string(val);
    return make_null();
}

/* os.setenv(name, value) - Set environment variable */
Value native_os_setenv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_bool(false);
    return make_bool(setenv(args[0].string_val, args[1].string_val, 1) == 0);
}

/* os.unsetenv(name) - Unset environment variable */
Value native_os_unsetenv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_bool(false);
    return make_bool(unsetenv(args[0].string_val) == 0);
}

/* os.sleep(ms) - Sleep for N milliseconds */
Value native_os_sleep(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || (args[0].type != VAL_INT && args[0].type != VAL_FLOAT))
        return make_null();
    double ms = (args[0].type == VAL_INT) ? (double)args[0].int_val : args[0].float_val;
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000.0);
    ts.tv_nsec = (long)((ms - ts.tv_sec * 1000.0) * 1000000.0);
    nanosleep(&ts, NULL);
    return make_null();
}

/* os.cpu_count() - Number of logical CPUs */
Value native_os_cpu_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count > 0) return make_int(count);
    return make_int(1);
}

/* os.pagesize() - System memory page size */
Value native_os_pagesize(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    long ps = sysconf(_SC_PAGESIZE);
    return make_int(ps > 0 ? ps : 4096);
}

/* os.mem_total() - Total physical memory in bytes (Linux only, best-effort) */
Value native_os_mem_total(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGESIZE);
    if (pages > 0 && page_size > 0) {
        return make_int((long long)pages * page_size);
    }
    return make_int(0);
}

/* os.mem_available() - Available memory in bytes (best-effort) */
Value native_os_mem_available(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGESIZE);
    if (pages > 0 && page_size > 0) {
        return make_int((long long)pages * page_size);
    }
    return make_int(0);
}

/* os.exit(code) - Exit the process */
Value native_os_exit(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int code = 0;
    if (argc >= 1 && args[0].type == VAL_INT) code = (int)args[0].int_val;
    exit(code);
    return make_null(); /* unreachable */
}

/* os.tmpdir() - System temporary directory */
Value native_os_tmpdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "/tmp";
    return make_string(tmp);
}

/* os.info() - Comprehensive system info as array: [name, arch, version, hostname, pid] */
Value native_os_info(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    struct utsname buf;
    Array* arr = array_create(5);
    if (uname(&buf) == 0) {
        array_push(arr, make_string(buf.sysname));
        array_push(arr, make_string(buf.machine));
        array_push(arr, make_string(buf.release));
        array_push(arr, make_string(buf.nodename));
    } else {
        array_push(arr, make_string("Unknown"));
        array_push(arr, make_string("unknown"));
        array_push(arr, make_string("unknown"));
        array_push(arr, make_string("unknown"));
    }
    array_push(arr, make_int((long long)getpid()));

    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

void stdlib_os_register(void) {
    register_native("os.name",       native_os_name);
    register_native("os.version",    native_os_version);
    register_native("os.arch",       native_os_arch);
    register_native("os.hostname",   native_os_hostname);
    register_native("os.cwd",        native_os_cwd);
    register_native("os.pid",        native_os_pid);
    register_native("os.ppid",       native_os_ppid);
    register_native("os.uid",        native_os_uid);
    register_native("os.gid",        native_os_gid);
    register_native("os.env",        native_os_env);
    register_native("os.setenv",     native_os_setenv);
    register_native("os.unsetenv",   native_os_unsetenv);
    register_native("os.sleep",      native_os_sleep);
    register_native("os.cpu_count",  native_os_cpu_count);
    register_native("os.pagesize",   native_os_pagesize);
    register_native("os.mem_total",  native_os_mem_total);
    register_native("os.mem_available", native_os_mem_available);
    register_native("os.exit",       native_os_exit);
    register_native("os.tmpdir",     native_os_tmpdir);
    register_native("os.info",       native_os_info);
}
