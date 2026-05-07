#define _POSIX_C_SOURCE 200809L
#include "stdlib_string.h"
#include "../core/interpreter.h"
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

/* str.split(str, delim) → array of substrings */
Value native_str_split(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_null();
    const char* str = args[0].string_val;
    const char* delim = args[1].string_val;
    
    int count = 1;
    const char* p = str;
    while (*p) {
        if (strncmp(p, delim, strlen(delim)) == 0) count++;
        p++;
    }
    
    Value v; v.type = VAL_ARRAY; v.array_val = array_create(count);
    p = str;
    char buffer[1024]; int buf_idx = 0;
    
    while (*p) {
        if (strncmp(p, delim, strlen(delim)) == 0) {
            if (buf_idx > 0) {
                buffer[buf_idx] = '\0';
                Value elem; elem.type = VAL_STRING; elem.string_val = strdup(buffer);
                array_push(v.array_val, elem);
                buf_idx = 0;
            }
            p += strlen(delim);
        } else {
            buffer[buf_idx++] = *p++;
        }
    }
    
    if (buf_idx > 0) {
        buffer[buf_idx] = '\0';
        Value elem; elem.type = VAL_STRING; elem.string_val = strdup(buffer);
        array_push(v.array_val, elem);
    }
    return v;
}

/* str.join(arr) → concatenate strings in array */
Value native_str_join(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    if (arr->count == 0) {
        Value v; v.type = VAL_STRING; v.string_val = strdup(""); return v;
    }
    
    size_t total_len = 0;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_STRING) total_len += strlen(arr->items[i].string_val);
    }
    
    char* result = malloc(total_len + 1); result[0] = '\0';
    size_t result_idx = 0;
    
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_STRING) {
            strcpy(result + result_idx, arr->items[i].string_val);
            result_idx += strlen(arr->items[i].string_val);
        }
    }
    result[result_idx] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = result; return v;
}

/* str.find(str, sub) → index of first occurrence or -1 */
Value native_str_find(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_int(-1);
    const char* str = args[0].string_val;
    const char* sub = args[1].string_val;
    char* result = strstr(str, sub);
    return result ? make_int((int)(result - str)) : make_int(-1);
}

/* str.rfind(str, sub) → index of last occurrence or -1 */
Value native_str_rfind(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_int(-1);
    const char* str = args[0].string_val;
    const char* sub = args[1].string_val;
    char* result = NULL; const char* p = str;
    while (*p) {
        if (strncmp(p, sub, strlen(sub)) == 0) result = (char*)p;
        p++;
    }
    return result ? make_int((int)(result - str)) : make_int(-1);
}

/* str.isdigit(str) → 1 if all digits, 0 otherwise */
Value native_str_isdigit(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(0);
    const char* str = args[0].string_val;
    for (size_t i = 0; str[i]; i++) {
        if (!isdigit((unsigned char)str[i])) return make_int(0);
    }
    return make_int(1);
}

/* str.isalpha(str) → 1 if all letters, 0 otherwise */
Value native_str_isalpha(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(0);
    const char* str = args[0].string_val;
    for (size_t i = 0; str[i]; i++) {
        if (!isalpha((unsigned char)str[i])) return make_int(0);
    }
    return make_int(1);
}

/* str.isalnum(str) → 1 if all alphanumeric, 0 otherwise */
Value native_str_isalnum(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(0);
    const char* str = args[0].string_val;
    for (size_t i = 0; str[i]; i++) {
        if (!isalnum((unsigned char)str[i])) return make_int(0);
    }
    return make_int(1);
}

/* str.ljust(str, width) → left-justify in width */
Value native_str_ljust(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) return make_null();
    const char* str = args[0].string_val;
    int width = args[1].int_val;
    int len = strlen(str); int pad_len = width - len;
    if (pad_len < 0) pad_len = 0;
    char* result = malloc(width + 1); strcpy(result, str);
    for (int i = 0; i < pad_len; i++) result[len + i] = ' ';
    result[width] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = result; return v;
}

/* str.rjust(str, width) → right-justify in width */
Value native_str_rjust(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) return make_null();
    const char* str = args[0].string_val;
    int width = args[1].int_val;
    int len = strlen(str); int pad_len = width - len;
    if (pad_len < 0) pad_len = 0;
    char* result = malloc(width + 1); int result_idx = 0;
    for (int i = 0; i < pad_len; i++) result[result_idx++] = ' ';
    strcpy(result + result_idx, str); result[width] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = result; return v;
}

/* str.center(str, width) → center in width */
Value native_str_center(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) return make_null();
    const char* str = args[0].string_val;
    int width = args[1].int_val;
    int len = strlen(str); int total_pad = width - len;
    if (total_pad < 0) total_pad = 0;
    int left_pad = total_pad / 2; int right_pad = total_pad - left_pad;
    char* result = malloc(width + 1); int result_idx = 0;
    for (int i = 0; i < left_pad; i++) result[result_idx++] = ' ';
    strcpy(result + result_idx, str); result_idx += len;
    for (int i = 0; i < right_pad; i++) result[result_idx++] = ' ';
    result[width] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = result; return v;
}

extern Value make_null(void);
extern Value make_int(long long val);
extern Value make_float(double val);


/* str.format(template, args...) - Simple template with {} placeholders */
Value native_str_format(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* tmpl = args[0].string_val;
    size_t tmpl_len = strlen(tmpl);
    int arg_idx = 1;
    
    /* Worst case: each char becomes itself, plus arg expansions */
    size_t buf_cap = tmpl_len * 4 + 256;
    char* buf = (char*)malloc(buf_cap);
    if (!buf) return make_null();
    size_t bp = 0;
    
    for (size_t i = 0; i < tmpl_len; i++) {
        if (tmpl[i] == '{' && i + 1 < tmpl_len && tmpl[i + 1] == '}') {
            /* Expand argument */
            if (arg_idx < argc) {
                Value v = args[arg_idx++];
                if (v.type == VAL_INT) {
                    bp += (size_t)snprintf(buf + bp, buf_cap - bp, "%lld", v.int_val);
                } else if (v.type == VAL_FLOAT) {
                    bp += (size_t)snprintf(buf + bp, buf_cap - bp, "%g", v.float_val);
                } else if (v.type == VAL_STRING) {
                    size_t slen = strlen(v.string_val);
                    if (bp + slen >= buf_cap) {
                        buf_cap = bp + slen + 256;
                        buf = (char*)realloc(buf, buf_cap);
                    }
                    memcpy(buf + bp, v.string_val, slen);
                    bp += slen;
                } else if (v.type == VAL_BOOL) {
                    const char* bs = v.bool_val ? "true" : "false";
                    size_t slen = strlen(bs);
                    memcpy(buf + bp, bs, slen);
                    bp += slen;
                } else if (v.type == VAL_NULL) {
                    memcpy(buf + bp, "null", 4); bp += 4;
                }
            }
            i++; /* skip '}' */
        } else {
            if (bp + 1 >= buf_cap) { buf_cap *= 2; buf = (char*)realloc(buf, buf_cap); }
            buf[bp++] = tmpl[i];
        }
    }
    buf[bp] = '\0';
    Value result = make_string(buf);
    free(buf);
    return result;
}

/* str.interpolate(template, struct) - Replace {key} with struct field values */
Value native_str_interpolate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRUCT_INSTANCE) return make_null();
    const char* tmpl = args[0].string_val;
    StructInstance* inst = args[1].struct_instance;
    size_t tmpl_len = strlen(tmpl);
    
    size_t buf_cap = tmpl_len * 4 + 256;
    char* buf = (char*)malloc(buf_cap);
    if (!buf) return make_null();
    size_t bp = 0;
    
    for (size_t i = 0; i < tmpl_len; i++) {
        if (tmpl[i] == '{') {
            /* Find closing } */
            size_t j = i + 1;
            while (j < tmpl_len && tmpl[j] != '}') j++;
            if (j < tmpl_len) {
                /* Extract key name */
                size_t key_len = j - i - 1;
                char key[256];
                if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
                memcpy(key, tmpl + i + 1, key_len);
                key[key_len] = '\0';
                
                /* Look up field */
                FieldValue* fv = inst->fields;
                while (fv) {
                    if (strcmp(fv->name, key) == 0 && fv->value) {
                        Value* v = fv->value;
                        if (v->type == VAL_INT) bp += (size_t)snprintf(buf + bp, buf_cap - bp, "%lld", v->int_val);
                        else if (v->type == VAL_FLOAT) bp += (size_t)snprintf(buf + bp, buf_cap - bp, "%g", v->float_val);
                        else if (v->type == VAL_STRING) {
                            size_t slen = strlen(v->string_val);
                            if (bp + slen >= buf_cap) { buf_cap = bp + slen + 256; buf = (char*)realloc(buf, buf_cap); }
                            memcpy(buf + bp, v->string_val, slen); bp += slen;
                        } else if (v->type == VAL_BOOL) {
                            const char* bs = v->bool_val ? "true" : "false";
                            size_t slen = strlen(bs); memcpy(buf + bp, bs, slen); bp += slen;
                        }
                        break;
                    }
                    fv = fv->next;
                }
                i = j; /* skip past } */
                continue;
            }
        }
        if (bp + 1 >= buf_cap) { buf_cap *= 2; buf = (char*)realloc(buf, buf_cap); }
        buf[bp++] = tmpl[i];
    }
    buf[bp] = '\0';
    Value result = make_string(buf);
    free(buf);
    return result;
}

void stdlib_string_register(void) {
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
    register_native("str.split", native_str_split);
    register_native("str.join", native_str_join);
    register_native("str.find", native_str_find);
    register_native("str.rfind", native_str_rfind);
    register_native("str.isdigit", native_str_isdigit);
    register_native("str.isalpha", native_str_isalpha);
    register_native("str.isalnum", native_str_isalnum);
    register_native("str.ljust", native_str_ljust);
    register_native("str.rjust", native_str_rjust);
    register_native("str.center", native_str_center);
    register_native("str.format", native_str_format);
    register_native("str.interpolate", native_str_interpolate);
}
