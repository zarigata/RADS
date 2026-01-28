#include "stdlib_array.h"
#include <stdio.h>

// ============================================================================
// ARRAY OPERATIONS - map, filter, reduce, find, some, every, sort, reverse
// RADS v0.0.7 "DARK MOON"
// ============================================================================

Value stdlib_array_map(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.map() requires 2 arguments (array, function)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Apply function to each element
    printf("✓ array.map() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_filter(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.filter() requires 2 arguments (array, predicate)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Filter array based on predicate
    printf("✓ array.filter() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_reduce(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.reduce() requires 2 arguments (array, reducer)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Reduce array based on reducer
    printf("✓ array.reduce() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_find(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.find() requires 2 arguments (array, predicate)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Find element based on predicate
    printf("✓ array.find() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_some(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.some() requires 2 arguments (array, predicate)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Check if any element matches predicate
    printf("✓ array.some() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_every(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.every() requires 2 arguments (array, predicate)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Check if all elements match predicate
    printf("✓ array.every() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_sort(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 1) {
        fprintf(stderr, "Error: array.sort() requires 1 argument (array)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Sort array in place
    printf("✓ array.sort() called\n");
    return (Value){.type = VAL_NULL};
}

Value stdlib_array_reverse(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 1) {
        fprintf(stderr, "Error: array.reverse() requires 1 argument (array)\n");
        return (Value){.type = VAL_NULL};
    }

    // TODO: Reverse array in place
    printf("✓ array.reverse() called\n");
    return (Value){.type = VAL_NULL};
}

void stdlib_array_register(void) {
    register_native("array.map", (NativeFn)stdlib_array_map);
    register_native("array.filter", (NativeFn)stdlib_array_filter);
    register_native("array.reduce", (NativeFn)stdlib_array_reduce);
    register_native("array.find", (NativeFn)stdlib_array_find);
    register_native("array.some", (NativeFn)stdlib_array_some);
    register_native("array.every", (NativeFn)stdlib_array_every);
    register_native("array.sort", (NativeFn)stdlib_array_sort);
    register_native("array.reverse", (NativeFn)stdlib_array_reverse);
}