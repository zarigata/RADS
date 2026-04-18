#define _POSIX_C_SOURCE 200809L
#include "stdlib_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* Execute a command and return stdout output as string */
Value native_process_exec(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    FILE* pipe = popen(args[0].string_val, "r");
    if (!pipe) return make_null();

    size_t cap = 4096;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) { pclose(pipe); return make_null(); }

    char tmp[1024];
    while (fgets(tmp, sizeof(tmp), pipe)) {
        size_t tlen = strlen(tmp);
        while (len + tlen + 1 > cap) {
            cap *= 2;
            char* nb = realloc(buf, cap);
            if (!nb) { free(buf); pclose(pipe); return make_null(); }
            buf = nb;
        }
        memcpy(buf + len, tmp, tlen);
        len += tlen;
    }
    buf[len] = '\0';
    int status = pclose(pipe);

    /* Return string; caller can check exit code separately if needed */
    Value v;
    v.type = VAL_STRING;
    v.string_val = buf;
    (void)status;
    return v;
}

/* Execute a command and return {stdout, exit_code} struct */
Value native_process_exec_status(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    FILE* pipe = popen(args[0].string_val, "r");
    if (!pipe) return make_null();

    size_t cap = 4096, len = 0;
    char* buf = malloc(cap);
    if (!buf) { pclose(pipe); return make_null(); }

    char tmp[1024];
    while (fgets(tmp, sizeof(tmp), pipe)) {
        size_t tlen = strlen(tmp);
        while (len + tlen + 1 > cap) { cap *= 2; buf = realloc(buf, cap); }
        memcpy(buf + len, tmp, tlen);
        len += tlen;
    }
    buf[len] = '\0';
    int status = pclose(pipe);

    /* We return stdout as string. Exit code is available via WEXITSTATUS */
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    /* Build a simple array: [stdout_string, exit_code] */
    Array* result = array_create(2);
    Value sv; sv.type = VAL_STRING; sv.string_val = buf;
    array_push(result, sv);
    array_push(result, make_int(exit_code));

    Value v; v.type = VAL_ARRAY; v.array_val = result;
    return v;
}

/* Get current PID */
Value native_process_pid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    return make_int((long long)getpid());
}

/* Get parent PID */
Value native_process_ppid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    return make_int((long long)getppid());
}

/* Get UID */
Value native_process_uid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    return make_int((long long)getuid());
}

/* Sleep for N seconds */
Value native_process_sleep(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    unsigned int secs = (unsigned int)args[0].int_val;
    sleep(secs);
    return make_null();
}

/* Sleep for N milliseconds (uses usleep) */
Value native_process_sleep_ms(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    useconds_t ms = (useconds_t)args[0].int_val;
    usleep(ms * 1000);
    return make_null();
}

/* Get current working directory */
Value native_process_cwd(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) {
        return make_string(buf);
    }
    return make_null();
}

/* Change directory */
Value native_process_chdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_bool(false);
    return make_bool(chdir(args[0].string_val) == 0);
}

/* Get hostname */
Value native_process_hostname(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
        return make_string(buf);
    }
    return make_null();
}

/* Get number of CPU cores */
Value native_process_cpu_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return make_int(n > 0 ? (long long)n : 1);
}

/* System uptime in seconds */
Value native_process_uptime(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return make_int((long long)ts.tv_sec);
    }
    return make_null();
}

/* Exit the process */
Value native_process_exit(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int code = 0;
    if (argc >= 1 && args[0].type == VAL_INT) {
        code = (int)args[0].int_val;
    }
    exit(code);
    return make_null(); /* unreachable */
}

void stdlib_process_register(void) {
    register_native("process.exec",        native_process_exec);
    register_native("process.exec_status", native_process_exec_status);
    register_native("process.pid",         native_process_pid);
    register_native("process.ppid",        native_process_ppid);
    register_native("process.uid",         native_process_uid);
    register_native("process.sleep",       native_process_sleep);
    register_native("process.sleep_ms",    native_process_sleep_ms);
    register_native("process.cwd",         native_process_cwd);
    register_native("process.chdir",       native_process_chdir);
    register_native("process.hostname",    native_process_hostname);
    register_native("process.cpu_count",   native_process_cpu_count);
    register_native("process.uptime",      native_process_uptime);
    register_native("process.exit",        native_process_exit);
}
