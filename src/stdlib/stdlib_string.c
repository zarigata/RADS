#define _POSIX_C_SOURCE 200809L
#include "stdlib_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static bool check_argc(int argc, int expected) {
    return argc == expected;
}

Value native_str_length(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_INT;
        v.int_val = 0;
        return v;
    }
    
    Value v;
    v.type = VAL_INT;
    v.int_val = strlen(args[0].string_val);
    return v;
}

Value native_str_upper(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    char* str = strdup(args[0].string_val);
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = str;
    return v;
}

Value native_str_lower(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    char* str = strdup(args[0].string_val);
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = str;
    return v;
}

Value native_str_trim(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    const char* str = args[0].string_val;
    
    // Trim leading whitespace
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        Value v;
        v.type = VAL_STRING;
        v.string_val = strdup("");
        return v;
    }
    
    // Trim trailing whitespace
    const char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    size_t len = end - str + 1;
    char* result = malloc(len + 1);
    memcpy(result, str, len);
    result[len] = '\0';
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = result;
    return v;
}

Value native_str_substring(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    const char* str = args[0].string_val;
    long long start = args[1].int_val;
    long long len = strlen(str);
    long long end = len;
    
    if (argc >= 3 && args[2].type == VAL_INT) {
        end = args[2].int_val;
    }
    
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) {
        Value v;
        v.type = VAL_STRING;
        v.string_val = strdup("");
        return v;
    }
    
    long long substr_len = end - start;
    char* result = malloc(substr_len + 1);
    memcpy(result, str + start, substr_len);
    result[substr_len] = '\0';
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = result;
    return v;
}

Value native_str_contains(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = (strstr(args[0].string_val, args[1].string_val) != NULL);
    return v;
}

Value native_str_replace(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 3 || args[0].type != VAL_STRING || 
        args[1].type != VAL_STRING || args[2].type != VAL_STRING) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    const char* str = args[0].string_val;
    const char* find = args[1].string_val;
    const char* replace = args[2].string_val;
    
    if (strlen(find) == 0) {
        Value v;
        v.type = VAL_STRING;
        v.string_val = strdup(str);
        return v;
    }
    
    const char* pos = strstr(str, find);
    if (!pos) {
        Value v;
        v.type = VAL_STRING;
        v.string_val = strdup(str);
        return v;
    }
    
    size_t prefix_len = pos - str;
    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    size_t suffix_len = strlen(pos + find_len);
    
    char* result = malloc(prefix_len + replace_len + suffix_len + 1);
    memcpy(result, str, prefix_len);
    memcpy(result + prefix_len, replace, replace_len);
    memcpy(result + prefix_len + replace_len, pos + find_len, suffix_len);
    result[prefix_len + replace_len + suffix_len] = '\0';
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = result;
    return v;
}

Value native_str_starts_with(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    const char* str = args[0].string_val;
    const char* prefix = args[1].string_val;
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = (strncmp(str, prefix, strlen(prefix)) == 0);
    return v;
}

Value native_str_ends_with(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    const char* str = args[0].string_val;
    const char* suffix = args[1].string_val;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = (strcmp(str + str_len - suffix_len, suffix) == 0);
    return v;
}

/* str.repeat(str, count) → repeated string */
Value native_str_repeat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        Value v; v.type = VAL_STRING; v.string_val = strdup(""); return v;
    }
    const char* s = args[0].string_val;
    long long count = args[1].int_val;
    if (count <= 0) { Value v; v.type = VAL_STRING; v.string_val = strdup(""); return v; }
    size_t slen = strlen(s);
    char* out = malloc(slen * (size_t)count + 1);
    if (!out) { Value v; v.type = VAL_STRING; v.string_val = strdup(""); return v; }
    out[0] = '\0';
    for (long long i = 0; i < count; i++) {
        memcpy(out + slen * (size_t)i, s, slen);
    }
    out[slen * (size_t)count] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

/* str.pad_left(str, width, fill_char) */
Value native_str_pad_left(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        Value v; v.type = VAL_STRING; v.string_val = strdup(""); return v;
    }
    const char* s = args[0].string_val;
    long long width = args[1].int_val;
    char fill = ' ';
    if (argc >= 3 && args[2].type == VAL_STRING && strlen(args[2].string_val) > 0) {
        fill = args[2].string_val[0];
    }
    size_t slen = strlen(s);
    if ((long long)slen >= width) { Value v; v.type = VAL_STRING; v.string_val = strdup(s); return v; }
    size_t pad = (size_t)width - slen;
    char* out = malloc((size_t)width + 1);
    memset(out, fill, pad);
    memcpy(out + pad, s, slen);
    out[width] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

/* str.pad_right(str, width, fill_char) */
Value native_str_pad_right(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        Value v; v.type = VAL_STRING; v.string_val = strdup(""); return v;
    }
    const char* s = args[0].string_val;
    long long width = args[1].int_val;
    char fill = ' ';
    if (argc >= 3 && args[2].type == VAL_STRING && strlen(args[2].string_val) > 0) {
        fill = args[2].string_val[0];
    }
    size_t slen = strlen(s);
    if ((long long)slen >= width) { Value v; v.type = VAL_STRING; v.string_val = strdup(s); return v; }
    char* out = malloc((size_t)width + 1);
    memcpy(out, s, slen);
    memset(out + slen, fill, (size_t)width - slen);
    out[width] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

/* str.char_at(str, index) → single-char string */
Value native_str_char_at(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        Value v; v.type = VAL_NULL; return v;
    }
    const char* s = args[0].string_val;
    long long idx = args[1].int_val;
    long long len = (long long)strlen(s);
    if (idx < 0 || idx >= len) { Value v; v.type = VAL_NULL; return v; }
    char* out = malloc(2);
    out[0] = s[idx]; out[1] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

/* str.to_int(str) → int */
Value native_str_to_int(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char* end;
    long long val = strtoll(args[0].string_val, &end, 10);
    if (end == args[0].string_val) return make_null();
    Value v; v.type = VAL_INT; v.int_val = val; return v;
}

/* str.to_float(str) → float */
Value native_str_to_float(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char* end;
    double val = strtod(args[0].string_val, &end);
    if (end == args[0].string_val) return make_null();
    Value v; v.type = VAL_FLOAT; v.float_val = val; return v;
}

/* str.is_empty(str) → bool */
Value native_str_is_empty(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) { Value v; v.type = VAL_BOOL; v.bool_val = true; return v; }
    Value v; v.type = VAL_BOOL; v.bool_val = (args[0].string_val[0] == '\0'); return v;
}

/* str.reverse(str) → reversed string */
Value native_str_reverse(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    size_t len = strlen(args[0].string_val);
    char* out = malloc(len + 1);
    for (size_t i = 0; i < len; i++) out[i] = args[0].string_val[len - 1 - i];
    out[len] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

/* str.count(str, substr) → int count of non-overlapping occurrences */
Value native_str_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        Value v; v.type = VAL_INT; v.int_val = 0; return v;
    }
    const char* haystack = args[0].string_val;
    const char* needle = args[1].string_val;
    size_t nlen = strlen(needle);
    if (nlen == 0) { Value v; v.type = VAL_INT; v.int_val = 0; return v; }
    long long count = 0;
    const char* p = haystack;
    while ((p = strstr(p, needle)) != NULL) { count++; p += nlen; }
    Value v; v.type = VAL_INT; v.int_val = count; return v;
}

/* str.title(str) → title-cased string */
Value native_str_title(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char* out = strdup(args[0].string_val);
    int new_word = 1;
    for (int i = 0; out[i]; i++) {
        if (isspace((unsigned char)out[i])) { new_word = 1; }
        else {
            if (new_word) out[i] = toupper((unsigned char)out[i]);
            else out[i] = tolower((unsigned char)out[i]);
            new_word = 0;
        }
    }
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

/* str.capitalize(str) → first char upper, rest lower */
Value native_str_capitalize(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char* out = strdup(args[0].string_val);
    if (out[0]) out[0] = toupper((unsigned char)out[0]);
    for (int i = 1; out[i]; i++) out[i] = tolower((unsigned char)out[i]);
    Value v; v.type = VAL_STRING; v.string_val = out; return v;
}

extern Value make_null(void);
extern Value make_int(long long val);
extern Value make_float(double val);

void stdlib_string_register(void) {
    register_native("str.length", native_str_length);
    register_native("str.upper", native_str_upper);
    register_native("str.lower", native_str_lower);
    register_native("str.trim", native_str_trim);
    register_native("str.substring", native_str_substring);
    register_native("str.contains", native_str_contains);
    register_native("str.replace", native_str_replace);
    register_native("str.starts_with", native_str_starts_with);
    register_native("str.ends_with", native_str_ends_with);
    register_native("str.repeat", native_str_repeat);
    register_native("str.pad_left", native_str_pad_left);
    register_native("str.pad_right", native_str_pad_right);
    register_native("str.char_at", native_str_char_at);
    register_native("str.to_int", native_str_to_int);
    register_native("str.to_float", native_str_to_float);
    register_native("str.is_empty", native_str_is_empty);
    register_native("str.reverse", native_str_reverse);
    register_native("str.count", native_str_count);
    register_native("str.title", native_str_title);
    register_native("str.capitalize", native_str_capitalize);
}
