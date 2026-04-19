#define _POSIX_C_SOURCE 200809L
#include "stdlib_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>

/* ── Path Module ──
 * Path manipulation utilities (Python os.path equivalent).
 * Lightweight, no allocations beyond result strings.
 */

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

#define PATH_MAX_BUF 4096

/* ── path.join(parts...) ── */
Value native_path_join(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc == 0) return make_string("");

    /* Calculate total length */
    size_t total = 0;
    for (int i = 0; i < argc; i++) {
        if (args[i].type != VAL_STRING) continue;
        total += strlen(args[i].string_val) + 1; /* +1 for separator */
    }
    if (total == 0) return make_string("");

    char* buf = malloc(total + 1);
    if (!buf) return make_null();
    buf[0] = '\0';

    for (int i = 0; i < argc; i++) {
        if (args[i].type != VAL_STRING) continue;
        const char* part = args[i].string_val;
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] != '/' && part[0] != '/') {
            buf[len] = '/';
            buf[len + 1] = '\0';
        } else if (len > 0 && buf[len - 1] == '/' && part[0] == '/') {
            part++; /* skip duplicate separator */
        }
        strcat(buf, part);
    }

    Value v; v.type = VAL_STRING; v.string_val = buf; return v;
}

/* ── path.basename(p) ── */
Value native_path_basename(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_string("");
    char* tmp = strdup(args[0].string_val);
    if (!tmp) return make_string("");
    char* base = basename(tmp);
    Value v = make_string(base);
    free(tmp);
    return v;
}

/* ── path.dirname(p) ── */
Value native_path_dirname(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_string("");
    char* tmp = strdup(args[0].string_val);
    if (!tmp) return make_string("");
    char* dir = dirname(tmp);
    Value v = make_string(dir);
    free(tmp);
    return v;
}

/* ── path.extname(p) → extension including dot, or "" ── */
Value native_path_extname(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_string("");
    const char* p = args[0].string_val;
    const char* dot = strrchr(p, '.');
    if (!dot || dot == p || strchr(dot, '/')) return make_string("");
    return make_string(dot);
}

/* ── path.stem(p) → filename without extension ── */
Value native_path_stem(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_string("");
    char* tmp = strdup(args[0].string_val);
    if (!tmp) return make_string("");
    char* base = basename(tmp);
    char* dot = strrchr(base, '.');
    if (dot && dot != base) *dot = '\0';
    Value v = make_string(base);
    free(tmp);
    return v;
}

/* ── path.is_absolute(p) ── */
Value native_path_is_absolute(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_bool(false);
    return make_bool(args[0].string_val[0] == '/');
}

/* ── path.normalize(p) ── */
Value native_path_normalize(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_string("");
    char buf[PATH_MAX_BUF];
    if (!realpath(args[0].string_val, buf)) {
        /* realpath fails for non-existent paths; do basic cleanup instead */
        strncpy(buf, args[0].string_val, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        /* Remove trailing slashes (except root) */
        size_t len = strlen(buf);
        while (len > 1 && buf[len - 1] == '/') buf[--len] = '\0';
    }
    return make_string(buf);
}

/* ── path.split(p) → [dirname, basename] ── */
Value native_path_split(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    char* tmp = strdup(args[0].string_val);
    if (!tmp) return make_null();
    char* dir = dirname(tmp);
    char* dir_copy = strdup(dir);
    free(tmp);

    tmp = strdup(args[0].string_val);
    if (!tmp) { free(dir_copy); return make_null(); }
    char* base = basename(tmp);

    Array* arr = array_create(2);
    array_push(arr, make_string(dir_copy));
    array_push(arr, make_string(base));

    free(dir_copy);
    free(tmp);

    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── path.splitext(p) → [root, ext] ── */
Value native_path_splitext(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* p = args[0].string_val;
    const char* dot = strrchr(p, '.');
    if (!dot || dot == p || strchr(dot, '/')) {
        Array* arr = array_create(2);
        array_push(arr, make_string(p));
        array_push(arr, make_string(""));
        Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
    }
    size_t root_len = (size_t)(dot - p);
    char* root = malloc(root_len + 1);
    if (!root) return make_null();
    memcpy(root, p, root_len);
    root[root_len] = '\0';
    Array* arr = array_create(2);
    array_push(arr, make_string(root));
    array_push(arr, make_string(dot));
    free(root);
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── path.sep ── (constant: "/") */
Value native_path_sep(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("/");
}

/* ── path.cwd() ── */
Value native_path_cwd(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    char buf[PATH_MAX_BUF];
    if (!getcwd(buf, sizeof(buf))) return make_null();
    return make_string(buf);
}

/* ── path.home() ── */
Value native_path_home(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    const char* home = getenv("HOME");
    if (!home) home = "/";
    return make_string(home);
}

/* ── path.glob(pattern) ── basic glob using opendir + simple pattern matching */
#include <dirent.h>

static int simple_match(const char* pattern, const char* str) {
    while (*pattern && *str) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) return 1;
            while (*str) {
                if (simple_match(pattern, str)) return 1;
                str++;
            }
            return simple_match(pattern, str);
        } else if (*pattern == '?' || *pattern == *str) {
            pattern++; str++;
        } else {
            return 0;
        }
    }
    while (*pattern == '*') pattern++;
    return *pattern == '\0' && *str == '\0';
}

Value native_path_glob(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    const char* pattern = args[0].string_val;

    /* Find directory part */
    char dir_buf[PATH_MAX_BUF];
    const char* fname_pattern;
    const char* last_slash = strrchr(pattern, '/');
    if (last_slash) {
        size_t dlen = (size_t)(last_slash - pattern);
        if (dlen == 0) { dir_buf[0] = '/'; dir_buf[1] = '\0'; }
        else { memcpy(dir_buf, pattern, dlen); dir_buf[dlen] = '\0'; }
        fname_pattern = last_slash + 1;
    } else {
        strcpy(dir_buf, ".");
        fname_pattern = pattern;
    }

    DIR* d = opendir(dir_buf);
    if (!d) return make_null();

    Array* arr = array_create(32);
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (simple_match(fname_pattern, entry->d_name)) {
            /* Build full path */
            char full[PATH_MAX_BUF];
            if (strcmp(dir_buf, ".") == 0) {
                strncpy(full, entry->d_name, sizeof(full) - 1);
            } else {
                snprintf(full, sizeof(full), "%s/%s", dir_buf, entry->d_name);
            }
            full[sizeof(full) - 1] = '\0';
            array_push(arr, make_string(full));
        }
    }
    closedir(d);

    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

void stdlib_path_register(void) {
    register_native("path.join", native_path_join);
    register_native("path.basename", native_path_basename);
    register_native("path.dirname", native_path_dirname);
    register_native("path.extname", native_path_extname);
    register_native("path.stem", native_path_stem);
    register_native("path.is_absolute", native_path_is_absolute);
    register_native("path.normalize", native_path_normalize);
    register_native("path.split", native_path_split);
    register_native("path.splitext", native_path_splitext);
    register_native("path.sep", native_path_sep);
    register_native("path.cwd", native_path_cwd);
    register_native("path.home", native_path_home);
    register_native("path.glob", native_path_glob);
}
