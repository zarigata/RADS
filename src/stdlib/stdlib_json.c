#define _POSIX_C_SOURCE 200809L
#include "stdlib_json.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static bool check_argc(int argc, int expected) {
    return argc == expected;
}

__attribute__((unused))
static Value make_string(const char* s) {
    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup(s ? s : "");
    return v;
}

static Value make_null(void) {
    Value v;
    v.type = VAL_NULL;
    return v;
}

static void trim_spaces(const char** p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

// Very small helper: find a string value for a key in a flat JSON object {"k":"v",...}
Value native_json_get_string(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_null();
    }
    const char* json = args[0].string_val;
    const char* key = args[1].string_val;
    if (!json || !key) return make_null();

    // naive search for "key"
    size_t keylen = strlen(key);
    const char* p = json;
    while ((p = strstr(p, key)) != NULL) {
        // ensure we're at a quoted key
        if (p > json && p[-1] != '"' ) { p += keylen; continue; }
        const char* after = p + keylen;
        // next char should be "
        if (*after != '"') { p += keylen; continue; }
        // find colon
        const char* colon = strchr(after, ':');
        if (!colon) return make_null();
        colon++;
        trim_spaces(&colon);
        if (*colon == '"') {
            colon++;
            const char* end = strchr(colon, '"');
            if (!end) return make_null();
            size_t len = (size_t)(end - colon);
            char* out = malloc(len + 1);
            if (!out) return make_null();
            memcpy(out, colon, len);
            out[len] = '\0';
            Value v; v.type = VAL_STRING; v.string_val = out; return v;
        }
        // non-string value not supported in this helper
        return make_null();
    }
    return make_null();
}

// Very small helper: parse a numeric literal for a flat key (no nested objects)
Value native_json_get_number(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_null();
    }
    const char* json = args[0].string_val;
    const char* key = args[1].string_val;
    size_t keylen = strlen(key);
    const char* p = json;
    while ((p = strstr(p, key)) != NULL) {
        if (p > json && p[-1] != '"') { p += keylen; continue; }
        const char* after = p + keylen;
        if (*after != '"') { p += keylen; continue; }
        const char* colon = strchr(after, ':');
        if (!colon) return make_null();
        colon++;
        trim_spaces(&colon);
        // parse number
        char* endptr = NULL;
        double val = strtod(colon, &endptr);
        if (endptr == colon) return make_null();
        Value v;
        // choose int if integral
        if (val == (long long)val) {
            v.type = VAL_INT;
            v.int_val = (long long)val;
        } else {
            v.type = VAL_FLOAT;
            v.float_val = val;
        }
        return v;
    }
    return make_null();
}

// Very small helper: parse boolean literal true/false for a flat key
Value native_json_get_bool(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_null();
    }
    const char* json = args[0].string_val;
    const char* key = args[1].string_val;
    size_t keylen = strlen(key);
    const char* p = json;
    while ((p = strstr(p, key)) != NULL) {
        if (p > json && p[-1] != '"') { p += keylen; continue; }
        const char* after = p + keylen;
        if (*after != '"') { p += keylen; continue; }
        const char* colon = strchr(after, ':');
        if (!colon) return make_null();
        colon++;
        trim_spaces(&colon);
        if (strncmp(colon, "true", 4) == 0) {
            Value v; v.type = VAL_BOOL; v.bool_val = true; return v;
        }
        if (strncmp(colon, "false", 5) == 0) {
            Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
        }
        return make_null();
    }
    return make_null();
}

// Simple builder: json.stringify_kv("k","v") => {"k":"v"}
Value native_json_stringify_kv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_null();
    }
    const char* k = args[0].string_val;
    const char* v = args[1].string_val;
    size_t len = strlen(k) + strlen(v) + 7; // {"":""}
    char* out = malloc(len + 1);
    if (!out) return make_null();
    snprintf(out, len + 1, "{\"%s\":\"%s\"}", k, v);
    Value val; val.type = VAL_STRING; val.string_val = out; return val;
}

// Escape quotes and backslash
Value native_json_escape(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_null();
    }
    const char* s = args[0].string_val;
    size_t len = 0;
    for (const char* p = s; *p; p++) {
        if (*p == '"' || *p == '\\') len++;
        len++;
    }
    char* out = malloc(len + 1);
    if (!out) return make_null();
    char* w = out;
    for (const char* p = s; *p; p++) {
        if (*p == '"' || *p == '\\') *w++ = '\\';
        *w++ = *p;
    }
    *w = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

void stdlib_json_register(void) {
    register_native("json.get_string", native_json_get_string);
    register_native("json.get_number", native_json_get_number);
    register_native("json.get_bool", native_json_get_bool);
    register_native("json.stringify_kv", native_json_stringify_kv);
    register_native("json.escape", native_json_escape);
}
