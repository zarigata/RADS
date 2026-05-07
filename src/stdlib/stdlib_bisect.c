/*
 * stdlib_bisect.c — RADS bisect module
 *
 * Binary search utilities for sorted arrays.
 * Equivalent to Python's bisect module.
 *
 * Functions:
 *   bisect.bisect(arr, x)        — insertion point to the right (alias: bisect_right)
 *   bisect.bisect_left(arr, x)   — insertion point to the left
 *   bisect.bisect_right(arr, x)  — insertion point to the right
 *   bisect.insort(arr, x)        — insert maintaining sort (alias: insort_right)
 *   bisect.insort_left(arr, x)   — insert left of duplicates
 *   bisect.insort_right(arr, x)  — insert right of duplicates
 *   bisect.index(arr, x)         — exact-match index or -1
 *   bisect.range(arr, lo, hi)    — indices of values in [lo, hi)
 */

#include "stdlib_bisect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_int(long long val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* Get double from Value for comparison */
static double val_to_double(Value* v) {
    if (v->type == VAL_INT) return (double)v->int_val;
    if (v->type == VAL_FLOAT) return v->float_val;
    return 0.0;
}

/* Get string from Value for comparison (or NULL) */
static const char* val_to_string(Value* v) {
    if (v->type == VAL_STRING) return v->string_val;
    return NULL;
}

/* Compare two Values: returns -1, 0, or 1 */
static int val_compare(Value* a, Value* b) {
    /* String comparison */
    if (a->type == VAL_STRING && b->type == VAL_STRING) {
        int cmp = strcmp(a->string_val, b->string_val);
        return cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
    }
    /* Numeric comparison */
    double va = val_to_double(a);
    double vb = val_to_double(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

/* ── bisect_right: find insertion point to the right of equal elements ── */
static long long bisect_right_impl(Array* arr, Value* x) {
    long long lo = 0;
    long long hi = (long long)arr->count;
    while (lo < hi) {
        long long mid = lo + (hi - lo) / 2;
        if (val_compare(&arr->items[mid], x) <= 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/* ── bisect_left: find insertion point to the left of equal elements ── */
static long long bisect_left_impl(Array* arr, Value* x) {
    long long lo = 0;
    long long hi = (long long)arr->count;
    while (lo < hi) {
        long long mid = lo + (hi - lo) / 2;
        if (val_compare(&arr->items[mid], x) < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/* ── Public API ── */

Value stdlib_bisect_right(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: bisect.bisect_right(arr, x) requires an array and a value\n");
        return make_int(-1);
    }
    return make_int(bisect_right_impl(args[0].array_val, &args[1]));
}

Value stdlib_bisect_left(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: bisect.bisect_left(arr, x) requires an array and a value\n");
        return make_int(-1);
    }
    return make_int(bisect_left_impl(args[0].array_val, &args[1]));
}

Value stdlib_bisect_bisect(struct Interpreter* interp, int argc, Value* args) {
    /* Alias for bisect_right */
    return stdlib_bisect_right(interp, argc, args);
}

Value stdlib_insort_right(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: bisect.insort_right(arr, x) requires an array and a value\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    long long pos = bisect_right_impl(arr, &args[1]);
    /* Shift elements right to make room */
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity * 2;
        arr->items = realloc(arr->items, arr->capacity * sizeof(Value));
    }
    /* Clone the value to insert */
    Value insert_val;
    if (args[1].type == VAL_STRING) {
        insert_val.type = VAL_STRING;
        insert_val.string_val = strdup(args[1].string_val);
    } else {
        insert_val = args[1];
    }
    /* Shift right */
    memmove(&arr->items[pos + 1], &arr->items[pos],
            (arr->count - pos) * sizeof(Value));
    arr->items[pos] = insert_val;
    arr->count++;
    return make_null();
}

Value stdlib_insort_left(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: bisect.insort_left(arr, x) requires an array and a value\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    long long pos = bisect_left_impl(arr, &args[1]);
    /* Shift elements right to make room */
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity * 2;
        arr->items = realloc(arr->items, arr->capacity * sizeof(Value));
    }
    Value insert_val;
    if (args[1].type == VAL_STRING) {
        insert_val.type = VAL_STRING;
        insert_val.string_val = strdup(args[1].string_val);
    } else {
        insert_val = args[1];
    }
    memmove(&arr->items[pos + 1], &arr->items[pos],
            (arr->count - pos) * sizeof(Value));
    arr->items[pos] = insert_val;
    arr->count++;
    return make_null();
}

Value stdlib_insort(struct Interpreter* interp, int argc, Value* args) {
    /* Alias for insort_right */
    return stdlib_insort_right(interp, argc, args);
}

Value stdlib_bisect_index(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: bisect.index(arr, x) requires an array and a value\n");
        return make_int(-1);
    }
    Array* arr = args[0].array_val;
    long long pos = bisect_left_impl(arr, &args[1]);
    if (pos < (long long)arr->count && val_compare(&arr->items[pos], &args[1]) == 0) {
        return make_int(pos);
    }
    return make_int(-1);
}

Value stdlib_bisect_range(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: bisect.range(arr, lo, hi) requires an array, lo, and hi\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    long long lo_pos = bisect_left_impl(arr, &args[1]);
    long long hi_pos = bisect_right_impl(arr, &args[2]);
    Array* result = array_create(hi_pos - lo_pos);
    for (long long i = lo_pos; i < hi_pos; i++) {
        array_push(result, arr->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

void stdlib_bisect_register(void) {
    register_native("bisect.bisect", (NativeFn)stdlib_bisect_bisect);
    register_native("bisect.bisect_left", (NativeFn)stdlib_bisect_left);
    register_native("bisect.bisect_right", (NativeFn)stdlib_bisect_right);
    register_native("bisect.insort", (NativeFn)stdlib_insort);
    register_native("bisect.insort_left", (NativeFn)stdlib_insort_left);
    register_native("bisect.insort_right", (NativeFn)stdlib_insort_right);
    register_native("bisect.index", (NativeFn)stdlib_bisect_index);
    register_native("bisect.range", (NativeFn)stdlib_bisect_range);
}
