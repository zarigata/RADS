#define _POSIX_C_SOURCE 200809L
#include "stdlib_sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* --- Comparison helpers --- */

static int cmp_int_asc(const void* a, const void* b) {
    long long va = (*(const Value*)a).int_val;
    long long vb = (*(const Value*)b).int_val;
    return (va > vb) - (va < vb);
}

static int cmp_int_desc(const void* a, const void* b) {
    return -cmp_int_asc(a, b);
}

static int cmp_float_asc(const void* a, const void* b) {
    double va = (*(const Value*)a).float_val;
    double vb = (*(const Value*)b).float_val;
    return (va > vb) - (va < vb);
}

static int cmp_float_desc(const void* a, const void* b) {
    return -cmp_float_asc(a, b);
}

static int cmp_string_asc(const void* a, const void* b) {
    return strcmp((*(const Value*)a).string_val, (*(const Value*)b).string_val);
}

static int cmp_string_desc(const void* a, const void* b) {
    return -cmp_string_asc(a, b);
}

/* sort(arr) - Sort array in ascending order (in-place, returns sorted array) */
Value native_sort_asc(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    if (arr->count == 0) return args[0];

    ValueType t = arr->items[0].type;
    if (t != VAL_INT && t != VAL_FLOAT && t != VAL_STRING) return make_null();

    /* Verify homogeneous */
    for (size_t i = 1; i < arr->count; i++) {
        if (arr->items[i].type != t) return make_null();
    }

    if (t == VAL_INT)        qsort(arr->items, arr->count, sizeof(Value), cmp_int_asc);
    else if (t == VAL_FLOAT) qsort(arr->items, arr->count, sizeof(Value), cmp_float_asc);
    else                     qsort(arr->items, arr->count, sizeof(Value), cmp_string_asc);

    return args[0];
}

/* sort.desc(arr) - Sort descending */
Value native_sort_desc(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    if (arr->count == 0) return args[0];

    ValueType t = arr->items[0].type;
    if (t != VAL_INT && t != VAL_FLOAT && t != VAL_STRING) return make_null();

    for (size_t i = 1; i < arr->count; i++) {
        if (arr->items[i].type != t) return make_null();
    }

    if (t == VAL_INT)        qsort(arr->items, arr->count, sizeof(Value), cmp_int_desc);
    else if (t == VAL_FLOAT) qsort(arr->items, arr->count, sizeof(Value), cmp_float_desc);
    else                     qsort(arr->items, arr->count, sizeof(Value), cmp_string_desc);

    return args[0];
}

/* sort.reverse(arr) - Reverse array in-place */
Value native_sort_reverse(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    size_t lo = 0, hi = arr->count - 1;
    while (lo < hi) {
        Value tmp = arr->items[lo];
        arr->items[lo] = arr->items[hi];
        arr->items[hi] = tmp;
        lo++; hi--;
    }
    return args[0];
}

/* sort.copy(arr) - Shallow copy of array */
Value native_sort_copy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* dest = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        array_push(dest, src->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = dest;
    return v;
}

/* sort.sorted(arr) - Return new sorted copy (ascending) */
Value native_sort_sorted(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    /* Copy then sort */
    Value copy = native_sort_copy(interp, argc, args);
    if (copy.type == VAL_NULL) return make_null();
    Value copy_arg = copy;
    return native_sort_asc(interp, 1, &copy_arg);
}

/* sort.min(arr) - Return minimum element */
Value native_sort_min(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    Array* arr = args[0].array_val;
    Value min = arr->items[0];
    for (size_t i = 1; i < arr->count; i++) {
        if (arr->items[i].type == VAL_INT && min.type == VAL_INT) {
            if (arr->items[i].int_val < min.int_val) min = arr->items[i];
        } else if (arr->items[i].type == VAL_FLOAT && min.type == VAL_FLOAT) {
            if (arr->items[i].float_val < min.float_val) min = arr->items[i];
        } else if (arr->items[i].type == VAL_STRING && min.type == VAL_STRING) {
            if (strcmp(arr->items[i].string_val, min.string_val) < 0) min = arr->items[i];
        }
    }
    return min;
}

/* sort.max(arr) - Return maximum element */
Value native_sort_max(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    Array* arr = args[0].array_val;
    Value max = arr->items[0];
    for (size_t i = 1; i < arr->count; i++) {
        if (arr->items[i].type == VAL_INT && max.type == VAL_INT) {
            if (arr->items[i].int_val > max.int_val) max = arr->items[i];
        } else if (arr->items[i].type == VAL_FLOAT && max.type == VAL_FLOAT) {
            if (arr->items[i].float_val > max.float_val) max = arr->items[i];
        } else if (arr->items[i].type == VAL_STRING && max.type == VAL_STRING) {
            if (strcmp(arr->items[i].string_val, max.string_val) > 0) max = arr->items[i];
        }
    }
    return max;
}

/* sort.sum(arr) - Sum numeric array */
Value native_sort_sum(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    long long int_sum = 0;
    double float_sum = 0.0;
    bool has_float = false;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_INT) int_sum += arr->items[i].int_val;
        else if (arr->items[i].type == VAL_FLOAT) { float_sum += arr->items[i].float_val; has_float = true; }
    }
    if (has_float) return make_float(float_sum + (double)int_sum);
    return make_int(int_sum);
}

/* sort.shuffle(arr) - Fisher-Yates shuffle in-place */
Value native_sort_shuffle(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    for (size_t i = arr->count - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        Value tmp = arr->items[i];
        arr->items[i] = arr->items[j];
        arr->items[j] = tmp;
    }
    return args[0];
}

/* Registration */
#include <stdio.h>

extern void register_native_fn(const char* module, const char* name, NativeFn fn);

void stdlib_sort_register(void) {
    srand((unsigned)time(NULL));
    register_native_fn("sort", "asc", native_sort_asc);
    register_native_fn("sort", "desc", native_sort_desc);
    register_native_fn("sort", "reverse", native_sort_reverse);
    register_native_fn("sort", "copy", native_sort_copy);
    register_native_fn("sort", "sorted", native_sort_sorted);
    register_native_fn("sort", "min", native_sort_min);
    register_native_fn("sort", "max", native_sort_max);
    register_native_fn("sort", "sum", native_sort_sum);
    register_native_fn("sort", "shuffle", native_sort_shuffle);
}
