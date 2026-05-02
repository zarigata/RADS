/*
 * stdlib_fnmatch.c — RADS fnmatch module
 *
 * Unix-style filename pattern matching.
 * Equivalent to Python's fnmatch module.
 *
 * Functions:
 *   fnmatch.fnmatch(name, pattern)     — match filename against pattern (case-sensitive)
 *   fnmatch.fnmatchcase(name, pattern) — case-sensitive match (alias behavior)
 *   fnmatch.filter(names, pattern)     — return matching names from list
 *   fnmatch.translate(pattern)         — convert glob pattern to regex string
 *
 * Pattern syntax:
 *   *      matches everything
 *   ?      matches any single character
 *   [seq]  matches any character in seq
 *   [!seq] matches any character not in seq
 */

#include "stdlib_fnmatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* ── Internal: match name against pattern ── */
static int fnmatch_impl(const char* name, const char* pattern) {
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            /* Skip consecutive stars */
            while (*pattern == '*') pattern++;
            if (*pattern == '\0') return 1;
            /* Try matching rest of pattern at each position */
            for (const char* n = name; ; n++) {
                if (fnmatch_impl(n, pattern)) return 1;
                if (*n == '\0') break;
            }
            return 0;
        }
        if (*name == '\0') return 0;

        if (*pattern == '?') {
            name++;
            pattern++;
            continue;
        }

        if (*pattern == '[') {
            pattern++;
            int negate = 0;
            if (*pattern == '!') {
                negate = 1;
                pattern++;
            }
            int matched = 0;
            char prev = '\0';
            while (*pattern && *pattern != ']') {
                if (*pattern == '-' && prev && *(pattern + 1) && *(pattern + 1) != ']') {
                    pattern++; /* skip - */
                    if (*name >= prev && *name <= *pattern) {
                        matched = 1;
                    }
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

        /* Literal character */
        if (*pattern != *name) return 0;
        pattern++;
        name++;
    }
    return *name == '\0';
}

/* ── Translate glob pattern to regex string ──
 * Caller must free the returned string. */
static char* translate_impl(const char* pattern) {
    size_t cap = strlen(pattern) * 4 + 16;
    char* regex = malloc(cap);
    if (!regex) return strdup(".*");
    size_t len = 0;

    /* Anchor start */
    regex[len++] = '^';

    for (const char* p = pattern; *p; p++) {
        if (len + 8 >= cap) {
            cap *= 2;
            regex = realloc(regex, cap);
        }

        switch (*p) {
            case '*':
                regex[len++] = '.';
                regex[len++] = '*';
                break;
            case '?':
                regex[len++] = '.';
                break;
            case '[': {
                regex[len++] = '[';
                p++;
                if (*p == '!') {
                    regex[len++] = '^';
                    p++;
                }
                while (*p && *p != ']') {
                    /* Escape regex special chars inside brackets */
                    if (*p == '\\') {
                        regex[len++] = '\\';
                        regex[len++] = '\\';
                    } else {
                        regex[len++] = *p;
                    }
                    p++;
                }
                if (*p == ']') regex[len++] = ']';
                break;
            }
            case '.':
            case '^':
            case '$':
            case '+':
            case '(':
            case ')':
            case '{':
            case '}':
            case '|':
            case '\\':
                regex[len++] = '\\';
                regex[len++] = *p;
                break;
            default:
                regex[len++] = *p;
                break;
        }
    }

    /* Anchor end */
    regex[len++] = '$';
    regex[len] = '\0';
    return regex;
}

/* ── Public API ── */

Value stdlib_fnmatch_fnmatch(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: fnmatch.fnmatch(name, pattern) requires two strings\n");
        return make_bool(false);
    }
    return make_bool(fnmatch_impl(args[0].string_val, args[1].string_val));
}

Value stdlib_fnmatch_fnmatchcase(struct Interpreter* interp, int argc, Value* args) {
    /* Same as fnmatch — always case-sensitive */
    return stdlib_fnmatch_fnmatch(interp, argc, args);
}

Value stdlib_fnmatch_filter(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: fnmatch.filter(names, pattern) requires an array and a string\n");
        return make_null();
    }
    Array* names = args[0].array_val;
    const char* pattern = args[1].string_val;
    Array* result = array_create(names->count);

    for (size_t i = 0; i < names->count; i++) {
        if (names->items[i].type == VAL_STRING &&
            fnmatch_impl(names->items[i].string_val, pattern)) {
            array_push(result, make_string(names->items[i].string_val));
        }
    }

    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

Value stdlib_fnmatch_translate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: fnmatch.translate(pattern) requires a string\n");
        return make_string("");
    }
    char* regex = translate_impl(args[0].string_val);
    Value v = make_string(regex);
    free(regex);
    return v;
}

void stdlib_fnmatch_register(void) {
    register_native("fnmatch.fnmatch", (NativeFn)stdlib_fnmatch_fnmatch);
    register_native("fnmatch.fnmatchcase", (NativeFn)stdlib_fnmatch_fnmatchcase);
    register_native("fnmatch.filter", (NativeFn)stdlib_fnmatch_filter);
    register_native("fnmatch.translate", (NativeFn)stdlib_fnmatch_translate);
}
