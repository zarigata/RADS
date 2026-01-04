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
}
