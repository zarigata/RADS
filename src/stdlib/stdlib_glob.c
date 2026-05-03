/*
 * stdlib_glob.c — RADS glob module
 *
 * Unix-style pathname pattern expansion.
 * Equivalent to Python's glob module.
 *
 * Functions:
 *   glob.glob(pattern)          — expand pattern, return matching paths
 *   glob.glob_recursive(pattern) — like glob but recurse into subdirs (**)
 *   glob.iglob(pattern)         — alias for glob (no lazy iteration in C)
 *   glob.has_magic(pattern)     — check if pattern contains glob chars
 *   glob.escape(path)           — escape special characters in path
 *
 * Pattern syntax:
 *   *      matches any sequence of characters (except /)
 *   ?      matches any single character (except /)
 *   [seq]  matches any character in seq
 *   [!seq] matches any character not in seq
 *   **     matches any sequence including / (only with _recursive)
 *
 * Zero external dependencies.
 */

#include "stdlib_glob.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* ── Internal: check if pattern has glob magic chars ── */
static int has_magic(const char* pattern) {
    for (const char* p = pattern; *p; p++) {
        if (*p == '*' || *p == '?' || *p == '[') return 1;
    }
    return 0;
}

/* ── Internal: match name against simple glob pattern (no /) ── */
static int match_glob(const char* name, const char* pattern) {
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            while (*pattern == '*') pattern++;
            if (*pattern == '\0') return 1;
            for (const char* n = name; ; n++) {
                if (match_glob(n, pattern)) return 1;
                if (*n == '\0') break;
            }
            return 0;
        }
        if (*name == '\0') return 0;
        if (*pattern == '?') {
            name++; pattern++; continue;
        }
        if (*pattern == '[') {
            pattern++;
            int negate = 0;
            if (*pattern == '!') { negate = 1; pattern++; }
            int matched = 0;
            char prev = '\0';
            while (*pattern && *pattern != ']') {
                if (*pattern == '-' && prev && *(pattern+1) && *(pattern+1) != ']') {
                    pattern++;
                    if (*name >= prev && *name <= *pattern) matched = 1;
                } else {
                    if (*name == *pattern) matched = 1;
                }
                prev = *pattern;
                pattern++;
            }
            if (*pattern == ']') pattern++;
            if (negate) matched = !matched;
            if (!matched) return 0;
            name++;
            continue;
        }
        if (*name != *pattern) return 0;
        name++; pattern++;
    }
    return *name == '\0';
}

/* ── Internal: recursive glob expansion ── */
static void glob_expand(const char* base_dir, const char* pattern, int recursive, Array* results) {
    /* If no more magic, check if path exists */
    if (!has_magic(pattern)) {
        char fullpath[4096];
        if (base_dir[0]) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", base_dir, pattern);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s", pattern);
        }
        struct stat st;
        if (stat(fullpath, &st) == 0) {
            array_push(results, make_string(fullpath));
        }
        return;
    }

    /* Split pattern at first / */
    const char* slash = strchr(pattern, '/');
    char head[1024];
    const char* rest;

    if (slash) {
        size_t hlen = (size_t)(slash - pattern);
        if (hlen >= sizeof(head)) hlen = sizeof(head) - 1;
        memcpy(head, pattern, hlen);
        head[hlen] = '\0';
        rest = slash + 1;
    } else {
        snprintf(head, sizeof(head), "%s", pattern);
        rest = NULL;
    }

    /* Handle ** for recursive match */
    if (strcmp(head, "**") == 0 && recursive) {
        /* ** matches zero or more directories */
        /* First: try matching rest at current dir (zero dirs) */
        if (rest && *rest) {
            glob_expand(base_dir, rest, recursive, results);
        }
        /* Then: recurse into each subdirectory */
        char dirpath[4096];
        if (base_dir[0]) {
            snprintf(dirpath, sizeof(dirpath), "%s", base_dir);
        } else {
            snprintf(dirpath, sizeof(dirpath), ".");
        }
        DIR* d = opendir(dirpath);
        if (d) {
            struct dirent* entry;
            while ((entry = readdir(d)) != NULL) {
                if (entry->d_name[0] == '.' &&
                    (entry->d_name[1] == '\0' ||
                     (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
                    continue;
                char subdir[4096];
                if (base_dir[0]) {
                    snprintf(subdir, sizeof(subdir), "%s/%s", base_dir, entry->d_name);
                } else {
                    snprintf(subdir, sizeof(subdir), "%s", entry->d_name);
                }
                struct stat st;
                if (stat(subdir, &st) == 0 && S_ISDIR(st.st_mode)) {
                    /* Recurse with same ** pattern */
                    glob_expand(subdir, pattern, recursive, results);
                }
            }
            closedir(d);
        }
        return;
    }

    /* Normal glob: expand head against directory entries */
    if (!has_magic(head)) {
        /* No magic in head — just descend */
        char newbase[4096];
        if (base_dir[0]) {
            snprintf(newbase, sizeof(newbase), "%s/%s", base_dir, head);
        } else {
            snprintf(newbase, sizeof(newbase), "%s", head);
        }
        if (rest && *rest) {
            glob_expand(newbase, rest, recursive, results);
        } else {
            struct stat st;
            if (stat(newbase, &st) == 0) {
                array_push(results, make_string(newbase));
            }
        }
        return;
    }

    /* Magic in head — scan directory */
    char dirpath[4096];
    if (base_dir[0]) {
        snprintf(dirpath, sizeof(dirpath), "%s", base_dir);
    } else {
        snprintf(dirpath, sizeof(dirpath), ".");
    }

    DIR* d = opendir(dirpath);
    if (!d) return;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;  /* Skip dotfiles by default */
        if (!match_glob(entry->d_name, head)) continue;

        char fullpath[4096];
        if (base_dir[0]) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", base_dir, entry->d_name);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s", entry->d_name);
        }

        if (rest && *rest) {
            /* Check if it's a directory before descending */
            struct stat st;
            if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                glob_expand(fullpath, rest, recursive, results);
            }
        } else {
            array_push(results, make_string(fullpath));
        }
    }
    closedir(d);
}

/* ── Public API ── */

Value stdlib_glob_glob(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: glob.glob(pattern) requires a string argument\n");
        return make_null();
    }
    Array* arr = array_create(16);
    glob_expand("", args[0].string_val, 0, arr);
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

Value stdlib_glob_glob_recursive(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: glob.glob_recursive(pattern) requires a string argument\n");
        return make_null();
    }
    Array* arr = array_create(32);
    glob_expand("", args[0].string_val, 1, arr);
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

Value stdlib_glob_iglob(struct Interpreter* interp, int argc, Value* args) {
    /* Alias — no lazy iter in C, just returns array like glob */
    return stdlib_glob_glob(interp, argc, args);
}

Value stdlib_glob_has_magic_fn(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_bool(false);
    return make_bool(has_magic(args[0].string_val));
}

Value stdlib_glob_escape(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* path = args[0].string_val;
    size_t len = strlen(path);
    /* Worst case: every char needs escaping */
    char* buf = malloc(len * 2 + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = path[i];
        if (c == '*' || c == '?' || c == '[' || c == ']') {
            buf[j++] = '[';
            buf[j++] = c;
            buf[j++] = ']';
        } else {
            buf[j++] = c;
        }
    }
    buf[j] = '\0';
    Value v = make_string(buf);
    free(buf);
    return v;
}

void stdlib_glob_register(void) {
    register_native("glob.glob", (NativeFn)stdlib_glob_glob);
    register_native("glob.glob_recursive", (NativeFn)stdlib_glob_glob_recursive);
    register_native("glob.iglob", (NativeFn)stdlib_glob_iglob);
    register_native("glob.has_magic", (NativeFn)stdlib_glob_has_magic_fn);
    register_native("glob.escape", (NativeFn)stdlib_glob_escape);
}
