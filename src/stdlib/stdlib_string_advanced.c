#include "stdlib_string_advanced.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// ADVANCED STRING OPERATIONS - split, join, trim, upper/lower, replace, substring
// RADS v0.0.7 "DARK MOON"
// ============================================================================

Value stdlib_string_split(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 2) {
        fprintf(stderr, "Error: string.split() requires at least 2 arguments\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // Get string value
    // TODO: Extract string from args[0]
    char* separator = "...";  // Get from args[1]
    // TODO: Split string and return array
    printf("✓ string.split() called\n");
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value stdlib_string_join(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 2) {
        fprintf(stderr, "Error: string.join() requires at least 2 arguments\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Join array of strings with separator
    printf("✓ string.join() called\n");
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value stdlib_string_trim(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 1) {
        fprintf(stderr, "Error: string.trim() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Trim whitespace from string
    printf("✓ string.trim() called\n");
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value stdlib_string_upper(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 1) {
        fprintf(stderr, "Error: string.upper() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Convert string to uppercase
    printf("✓ string.upper() called\n");
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value stdlib_string_lower(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 1) {
        fprintf(stderr, "Error: string.lower() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Convert string to lowercase
    printf("✓ string.lower() called\n");
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value stdlib_string_replace(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 3) {
        fprintf(stderr, "Error: string.replace() requires 3 arguments (string, old, new)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Replace all occurrences of substring
    printf("✓ string.replace() called\n");
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value stdlib_string_substring(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)args;  // Suppress unused warnings
    if (argc < 3) {
        fprintf(stderr, "Error: string.substring() requires 3 arguments (string, start, end)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Extract substring from string
    printf("✓ string.substring() called\n");
    Value v;
    v.type = VAL_NULL;
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

