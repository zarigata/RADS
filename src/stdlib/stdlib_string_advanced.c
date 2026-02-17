#include "stdlib_string_advanced.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

extern Value make_string(const char* val);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

Value stdlib_string_split(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) {
        fprintf(stderr, "Error: string.split() requires 2 arguments (string, separator)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: string.split() arguments must be strings\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char* str = args[0].string_val;
    char* sep = args[1].string_val;
    
    Array* result_arr = array_create(8);
    
    char* str_copy = strdup(str);
    char* token = strtok(str_copy, sep);
    
    while (token != NULL) {
        Value item;
        item.type = VAL_STRING;
        item.string_val = strdup(token);
        array_push(result_arr, item);
        token = strtok(NULL, sep);
    }
    
    free(str_copy);
    
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

Value stdlib_string_join(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) {
        fprintf(stderr, "Error: string.join() requires 2 arguments (array, separator)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: string.join() requires array and string separator\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    Array* arr = args[0].array_val;
    char* sep = args[1].string_val;
    
    size_t total_len = 1;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_STRING) {
            total_len += strlen(arr->items[i].string_val);
        }
        if (i > 0) total_len += strlen(sep);
    }
    
    char* result = malloc(total_len);
    result[0] = '\0';
    
    for (size_t i = 0; i < arr->count; i++) {
        if (i > 0) strcat(result, sep);
        if (arr->items[i].type == VAL_STRING) {
            strcat(result, arr->items[i].string_val);
        }
    }
    
    Value v = make_string(result);
    free(result);
    return v;
}

Value stdlib_string_trim(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: string.trim() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: string.trim() argument must be a string\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char* str = args[0].string_val;
    
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == '\0') {
        return make_string("");
    }
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    size_t len = end - str + 1;
    char* trimmed = malloc(len + 1);
    strncpy(trimmed, str, len);
    trimmed[len] = '\0';
    
    Value v = make_string(trimmed);
    free(trimmed);
    return v;
}

Value stdlib_string_upper(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: string.upper() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: string.upper() argument must be a string\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char* str = args[0].string_val;
    char* upper = strdup(str);
    
    for (char* p = upper; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
    
    Value v = make_string(upper);
    free(upper);
    return v;
}

Value stdlib_string_lower(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: string.lower() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: string.lower() argument must be a string\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char* str = args[0].string_val;
    char* lower = strdup(str);
    
    for (char* p = lower; *p; p++) {
        *p = tolower((unsigned char)*p);
    }
    
    Value v = make_string(lower);
    free(lower);
    return v;
}

Value stdlib_string_replace(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3) {
        fprintf(stderr, "Error: string.replace() requires 3 arguments (string, old, new)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING || args[2].type != VAL_STRING) {
        fprintf(stderr, "Error: string.replace() arguments must be strings\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char* str = args[0].string_val;
    char* old_sub = args[1].string_val;
    char* new_sub = args[2].string_val;
    
    size_t old_len = strlen(old_sub);
    size_t new_len = strlen(new_sub);
    
    size_t count = 0;
    char* pos = str;
    while ((pos = strstr(pos, old_sub)) != NULL) {
        count++;
        pos += old_len;
    }
    
    size_t result_len = strlen(str) + count * (new_len - old_len) + 1;
    char* result = malloc(result_len);
    
    char* src = str;
    char* dst = result;
    
    while ((pos = strstr(src, old_sub)) != NULL) {
        size_t copy_len = pos - src;
        memcpy(dst, src, copy_len);
        dst += copy_len;
        memcpy(dst, new_sub, new_len);
        dst += new_len;
        src = pos + old_len;
    }
    strcpy(dst, src);
    
    Value v = make_string(result);
    free(result);
    return v;
}

Value stdlib_string_substring(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3) {
        fprintf(stderr, "Error: string.substring() requires 3 arguments (string, start, end)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_INT || args[2].type != VAL_INT) {
        fprintf(stderr, "Error: string.substring() requires (string, int, int)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char* str = args[0].string_val;
    long long start = args[1].int_val;
    long long end = args[2].int_val;
    
    size_t len = strlen(str);
    if (start < 0) start = 0;
    if (end > (long long)len) end = len;
    if (start >= end) {
        return make_string("");
    }
    
    size_t sub_len = end - start;
    char* substring = malloc(sub_len + 1);
    strncpy(substring, str + start, sub_len);
    substring[sub_len] = '\0';
    
    Value v = make_string(substring);
    free(substring);
    return v;
}

void stdlib_string_advanced_register(void) {
    register_native("string.split", stdlib_string_split);
    register_native("string.join", stdlib_string_join);
    register_native("string.trim", stdlib_string_trim);
    register_native("string.upper", stdlib_string_upper);
    register_native("string.lower", stdlib_string_lower);
    register_native("string.replace", stdlib_string_replace);
    register_native("string.substring", stdlib_string_substring);
}