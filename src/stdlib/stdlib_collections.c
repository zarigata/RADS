#define _POSIX_C_SOURCE 200809L
#include "stdlib_collections.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* === Stack operations (LIFO) === */

/* Push element onto stack (alias for array.push semantics) */
Value native_collections_push(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_null();
    array_push(args[0].array_val, args[1]);
    return make_int((long long)args[0].array_val->count);
}

/* Pop last element from stack */
Value native_collections_pop(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    Array* arr = args[0].array_val;
    Value v = arr->items[arr->count - 1];
    arr->count--;
    return v;
}

/* Peek at last element without removing */
Value native_collections_peek(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    return args[0].array_val->items[args[0].array_val->count - 1];
}

/* === Queue operations (FIFO) === */

/* Dequeue first element */
Value native_collections_dequeue(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    Array* arr = args[0].array_val;
    Value v = arr->items[0];
    if (arr->count > 1) {
        memmove(arr->items, arr->items + 1, sizeof(Value) * (arr->count - 1));
    }
    arr->count--;
    return v;
}

/* Enqueue element (push to end) */
Value native_collections_enqueue(struct Interpreter* interp, int argc, Value* args) {
    return native_collections_push(interp, argc, args);
}

/* === Set operations === */

/* Helper: check if value equals */
static int value_equals(Value* a, Value* b) {
    if (a->type != b->type) return 0;
    switch (a->type) {
        case VAL_INT:   return a->int_val == b->int_val;
        case VAL_FLOAT: return a->float_val == b->float_val;
        case VAL_BOOL:  return a->bool_val == b->bool_val;
        case VAL_STRING: return a->string_val && b->string_val && strcmp(a->string_val, b->string_val) == 0;
        case VAL_NULL:  return 1;
        default:        return 0;
    }
}

/* Check if array contains a value */
Value native_collections_contains(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_bool(false);
    Array* arr = args[0].array_val;
    for (size_t i = 0; i < arr->count; i++) {
        if (value_equals(&arr->items[i], &args[1])) return make_bool(true);
    }
    return make_bool(false);
}

/* Remove duplicates from array, return new array */
Value native_collections_unique(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);

    for (size_t i = 0; i < src->count; i++) {
        int found = 0;
        for (size_t j = 0; j < result->count; j++) {
            if (value_equals(&result->items[j], &src->items[i])) { found = 1; break; }
        }
        if (!found) array_push(result, src->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Union of two arrays (unique elements from both) */
Value native_collections_union(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val;
    Array* b = args[1].array_val;
    Array* result = array_create(a->count + b->count);

    for (size_t i = 0; i < a->count; i++) array_push(result, a->items[i]);
    for (size_t i = 0; i < b->count; i++) {
        int found = 0;
        for (size_t j = 0; j < result->count; j++) {
            if (value_equals(&result->items[j], &b->items[i])) { found = 1; break; }
        }
        if (!found) array_push(result, b->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Intersection of two arrays */
Value native_collections_intersect(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val;
    Array* b = args[1].array_val;
    Array* result = array_create(a->count > b->count ? a->count : b->count);

    for (size_t i = 0; i < a->count; i++) {
        for (size_t j = 0; j < b->count; j++) {
            if (value_equals(&a->items[i], &b->items[j])) {
                /* Check not already in result */
                int dup = 0;
                for (size_t k = 0; k < result->count; k++) {
                    if (value_equals(&result->items[k], &a->items[i])) { dup = 1; break; }
                }
                if (!dup) array_push(result, a->items[i]);
                break;
            }
        }
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Difference: elements in a but not in b */
Value native_collections_difference(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val;
    Array* b = args[1].array_val;
    Array* result = array_create(a->count);

    for (size_t i = 0; i < a->count; i++) {
        int found = 0;
        for (size_t j = 0; j < b->count; j++) {
            if (value_equals(&a->items[i], &b->items[j])) { found = 1; break; }
        }
        if (!found) array_push(result, a->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* === Utility operations === */

/* Count occurrences of a value in array */
Value native_collections_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_int(0);
    Array* arr = args[0].array_val;
    long long c = 0;
    for (size_t i = 0; i < arr->count; i++) {
        if (value_equals(&arr->items[i], &args[1])) c++;
    }
    return make_int(c);
}

/* Reverse array, return new array */
Value native_collections_reverse(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);
    for (size_t i = src->count; i > 0; i--) {
        array_push(result, src->items[i - 1]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Get first N elements */
Value native_collections_take(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_INT) return make_null();
    Array* src = args[0].array_val;
    long long n = args[1].int_val;
    if (n < 0) n = 0;
    if ((size_t)n > src->count) n = (long long)src->count;
    Array* result = array_create((size_t)n);
    for (long long i = 0; i < n; i++) {
        array_push(result, src->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Skip first N elements, return rest */
Value native_collections_skip(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_INT) return make_null();
    Array* src = args[0].array_val;
    long long n = args[1].int_val;
    if (n < 0) n = 0;
    if ((size_t)n > src->count) n = (long long)src->count;
    Array* result = array_create(src->count - (size_t)n);
    for (size_t i = (size_t)n; i < src->count; i++) {
        array_push(result, src->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Flatten nested arrays (one level) */
Value native_collections_flatten(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    /* Estimate capacity */
    size_t cap = src->count * 2;
    Array* result = array_create(cap);

    for (size_t i = 0; i < src->count; i++) {
        if (src->items[i].type == VAL_ARRAY) {
            Array* inner = src->items[i].array_val;
            for (size_t j = 0; j < inner->count; j++) {
                array_push(result, inner->items[j]);
            }
        } else {
            array_push(result, src->items[i]);
        }
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Zip two arrays into array of pairs (arrays of 2) */
Value native_collections_zip(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val;
    Array* b = args[1].array_val;
    size_t min_len = a->count < b->count ? a->count : b->count;
    Array* result = array_create(min_len);

    for (size_t i = 0; i < min_len; i++) {
        Array* pair = array_create(2);
        array_push(pair, a->items[i]);
        array_push(pair, b->items[i]);
        Value pv;
        pv.type = VAL_ARRAY;
        pv.array_val = pair;
        array_push(result, pv);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Chunk array into chunks of size N */
Value native_collections_chunk(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_INT) return make_null();
    Array* src = args[0].array_val;
    long long size = args[1].int_val;
    if (size <= 0) return make_null();

    size_t n_chunks = (src->count + (size_t)size - 1) / (size_t)size;
    Array* result = array_create(n_chunks);

    for (size_t i = 0; i < src->count; i += (size_t)size) {
        size_t end = i + (size_t)size;
        if (end > src->count) end = src->count;
        Array* chunk = array_create((size_t)size);
        for (size_t j = i; j < end; j++) {
            array_push(chunk, src->items[j]);
        }
        Value cv;
        cv.type = VAL_ARRAY;
        cv.array_val = chunk;
        array_push(result, cv);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Sort array of numbers (ascending) */
Value native_collections_sort(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) array_push(result, src->items[i]);

    /* Simple insertion sort (lightweight, good for small-medium arrays) */
    for (size_t i = 1; i < result->count; i++) {
        Value key = result->items[i];
        double key_val = (key.type == VAL_INT) ? (double)key.int_val :
                         (key.type == VAL_FLOAT) ? key.float_val : 0.0;
        long long j = (long long)i - 1;
        while (j >= 0) {
            double aj = (result->items[j].type == VAL_INT) ? (double)result->items[j].int_val :
                        (result->items[j].type == VAL_FLOAT) ? result->items[j].float_val : 0.0;
            if (aj > key_val) {
                result->items[j + 1] = result->items[j];
                j--;
            } else break;
        }
        result->items[j + 1] = key;
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Shuffle array (Fisher-Yates), return new array */
Value native_collections_shuffle(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) array_push(result, src->items[i]);

    /* Fisher-Yates shuffle using rand */
    srand((unsigned)(size_t)interp ^ (unsigned)time(NULL));
    for (size_t i = result->count; i > 1; i--) {
        size_t j = (size_t)rand() % i;
        Value tmp = result->items[i - 1];
        result->items[i - 1] = result->items[j];
        result->items[j] = tmp;
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* Find index of value in array, or -1 */
Value native_collections_index_of(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_int(-1);
    Array* arr = args[0].array_val;
    for (size_t i = 0; i < arr->count; i++) {
        if (value_equals(&arr->items[i], &args[1])) return make_int((long long)i);
    }
    return make_int(-1);
}

/* Remove element at index, return removed value */
Value native_collections_remove_at(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_INT) return make_null();
    Array* arr = args[0].array_val;
    long long idx = args[1].int_val;
    if (idx < 0 || (size_t)idx >= arr->count) return make_null();
    Value v = arr->items[idx];
    for (size_t i = (size_t)idx; i < arr->count - 1; i++) {
        arr->items[i] = arr->items[i + 1];
    }
    arr->count--;
    return v;
}

/* Sum array of numbers */
Value native_collections_sum(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_int(0);
    Array* arr = args[0].array_val;
    double s = 0.0;
    int has_float = 0;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_INT) s += (double)arr->items[i].int_val;
        else if (arr->items[i].type == VAL_FLOAT) { s += arr->items[i].float_val; has_float = 1; }
    }
    return has_float ? make_float(s) : make_int((long long)s);
}

/* Min of array */
Value native_collections_min(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    Array* arr = args[0].array_val;
    double m = (arr->items[0].type == VAL_INT) ? (double)arr->items[0].int_val : arr->items[0].float_val;
    int has_float = (arr->items[0].type == VAL_FLOAT);
    for (size_t i = 1; i < arr->count; i++) {
        double v = (arr->items[i].type == VAL_INT) ? (double)arr->items[i].int_val : arr->items[i].float_val;
        if (arr->items[i].type == VAL_FLOAT) has_float = 1;
        if (v < m) m = v;
    }
    return has_float ? make_float(m) : make_int((long long)m);
}

/* Max of array */
Value native_collections_max(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) return make_null();
    Array* arr = args[0].array_val;
    double m = (arr->items[0].type == VAL_INT) ? (double)arr->items[0].int_val : arr->items[0].float_val;
    int has_float = (arr->items[0].type == VAL_FLOAT);
    for (size_t i = 1; i < arr->count; i++) {
        double v = (arr->items[i].type == VAL_INT) ? (double)arr->items[i].int_val : arr->items[i].float_val;
        if (arr->items[i].type == VAL_FLOAT) has_float = 1;
        if (v > m) m = v;
    }
    return has_float ? make_float(m) : make_int((long long)m);
}

void stdlib_collections_register(void) {
    /* Stack */
    register_native("collections.push", native_collections_push);
    register_native("collections.pop", native_collections_pop);
    register_native("collections.peek", native_collections_peek);
    /* Queue */
    register_native("collections.enqueue", native_collections_enqueue);
    register_native("collections.dequeue", native_collections_dequeue);
    /* Set operations */
    register_native("collections.contains", native_collections_contains);
    register_native("collections.unique", native_collections_unique);
    register_native("collections.union", native_collections_union);
    register_native("collections.intersect", native_collections_intersect);
    register_native("collections.difference", native_collections_difference);
    /* Utility */
    register_native("collections.count", native_collections_count);
    register_native("collections.reverse", native_collections_reverse);
    register_native("collections.take", native_collections_take);
    register_native("collections.skip", native_collections_skip);
    register_native("collections.flatten", native_collections_flatten);
    register_native("collections.zip", native_collections_zip);
    register_native("collections.chunk", native_collections_chunk);
    register_native("collections.sort", native_collections_sort);
    register_native("collections.shuffle", native_collections_shuffle);
    register_native("collections.index_of", native_collections_index_of);
    register_native("collections.remove_at", native_collections_remove_at);
    /* Aggregation */
    register_native("collections.sum", native_collections_sum);
    register_native("collections.min", native_collections_min);
    register_native("collections.max", native_collections_max);
}
