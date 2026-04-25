#define _POSIX_C_SOURCE 200809L
#include "stdlib_itertools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

static Value make_array(Array* arr) {
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

/* itertools.count(n, [start, [step]]) - Generate count sequence */
Value native_itertools_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    long long n = 100, start = 0, step = 1;
    if (argc >= 1 && args[0].type == VAL_INT) n = args[0].int_val;
    if (argc >= 2 && args[1].type == VAL_INT) start = args[1].int_val;
    if (argc >= 3 && args[2].type == VAL_INT) step = args[2].int_val;
    if (step == 0) step = 1;
    if (n < 0) n = 0;
    Array* arr = array_create((size_t)n);
    for (long long i = 0; i < n; i++)
        array_push(arr, make_int(start + i * step));
    return make_array(arr);
}

/* itertools.cycle(arr, n) - Cycle through array n times */
Value native_itertools_cycle(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    long long reps = 1;
    if (argc >= 2 && args[1].type == VAL_INT) reps = args[1].int_val;
    if (reps < 0 || src->count == 0) return make_array(array_create(0));
    Array* result = array_create((size_t)(src->count * reps));
    for (long long r = 0; r < reps; r++)
        for (size_t i = 0; i < src->count; i++)
            array_push(result, src->items[i]);
    return make_array(result);
}

/* itertools.repeat(val, n) - Repeat value n times */
Value native_itertools_repeat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[1].type != VAL_INT) return make_null();
    long long n = args[1].int_val;
    if (n < 0) n = 0;
    Array* arr = array_create((size_t)n);
    for (long long i = 0; i < n; i++) array_push(arr, args[0]);
    return make_array(arr);
}

/* itertools.chain(a, b) - Concatenate two arrays */
Value native_itertools_chain(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val, *b = args[1].array_val;
    Array* result = array_create(a->count + b->count);
    for (size_t i = 0; i < a->count; i++) array_push(result, a->items[i]);
    for (size_t i = 0; i < b->count; i++) array_push(result, b->items[i]);
    return make_array(result);
}

/* itertools.zip(a, b) - Zip into pairs */
Value native_itertools_zip(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val, *b = args[1].array_val;
    size_t len = a->count < b->count ? a->count : b->count;
    Array* result = array_create(len);
    for (size_t i = 0; i < len; i++) {
        Array* pair = array_create(2);
        array_push(pair, a->items[i]);
        array_push(pair, b->items[i]);
        array_push(result, make_array(pair));
    }
    return make_array(result);
}

/* itertools.enumerate(arr, [start]) - [index, value] pairs */
Value native_itertools_enumerate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    long long start = 0;
    if (argc >= 2 && args[1].type == VAL_INT) start = args[1].int_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Array* pair = array_create(2);
        array_push(pair, make_int(start + (long long)i));
        array_push(pair, src->items[i]);
        array_push(result, make_array(pair));
    }
    return make_array(result);
}

/* itertools.islice(arr, start, end, [step]) - Slice array */
Value native_itertools_islice(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_ARRAY || args[1].type != VAL_INT || args[2].type != VAL_INT)
        return make_null();
    Array* src = args[0].array_val;
    long long start = args[1].int_val, end = args[2].int_val, step = 1;
    if (argc >= 4 && args[3].type == VAL_INT) step = args[3].int_val;
    if (step == 0) step = 1;
    if (start < 0) start = (long long)src->count + start;
    if (end < 0) end = (long long)src->count + end;
    if (start < 0) start = 0;
    if (end > (long long)src->count) end = (long long)src->count;
    Array* result = array_create(16);
    if (step > 0) {
        for (long long i = start; i < end; i += step)
            if (i >= 0 && (size_t)i < src->count) array_push(result, src->items[(size_t)i]);
    } else {
        for (long long i = start; i > end; i += step)
            if (i >= 0 && (size_t)i < src->count) array_push(result, src->items[(size_t)i]);
    }
    return make_array(result);
}

/* itertools.filter(arr, predicate) - Filter by named predicate */
Value native_itertools_filter(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* pred = args[1].string_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value v = src->items[i];
        bool keep = false;
        if (strcmp(pred, "nonzero") == 0)
            keep = (v.type == VAL_INT && v.int_val != 0) || (v.type == VAL_FLOAT && v.float_val != 0.0);
        else if (strcmp(pred, "positive") == 0)
            keep = (v.type == VAL_INT && v.int_val > 0) || (v.type == VAL_FLOAT && v.float_val > 0.0);
        else if (strcmp(pred, "negative") == 0)
            keep = (v.type == VAL_INT && v.int_val < 0) || (v.type == VAL_FLOAT && v.float_val < 0.0);
        else if (strcmp(pred, "even") == 0)
            keep = (v.type == VAL_INT && v.int_val % 2 == 0);
        else if (strcmp(pred, "odd") == 0)
            keep = (v.type == VAL_INT && v.int_val % 2 != 0);
        else if (strcmp(pred, "nonnull") == 0)
            keep = (v.type != VAL_NULL);
        if (keep) array_push(result, v);
    }
    return make_array(result);
}

/* itertools.map(arr, operation, [operand]) - Map named operation over array */
Value native_itertools_map(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* op = args[1].string_val;
    double operand = 0.0;
    if (argc >= 3) {
        if (args[2].type == VAL_INT) operand = (double)args[2].int_val;
        else if (args[2].type == VAL_FLOAT) operand = args[2].float_val;
    }
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value v = src->items[i];
        if (strcmp(op, "negate") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(-v.int_val) :
                              v.type == VAL_FLOAT ? make_float(-v.float_val) : v);
        } else if (strcmp(op, "double") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val * 2) :
                              v.type == VAL_FLOAT ? make_float(v.float_val * 2) : v);
        } else if (strcmp(op, "square") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val * v.int_val) :
                              v.type == VAL_FLOAT ? make_float(v.float_val * v.float_val) : v);
        } else if (strcmp(op, "abs") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val < 0 ? -v.int_val : v.int_val) :
                              v.type == VAL_FLOAT ? make_float(v.float_val < 0 ? -v.float_val : v.float_val) : v);
        } else if (strcmp(op, "increment") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val + 1) :
                              v.type == VAL_FLOAT ? make_float(v.float_val + 1) : v);
        } else if (strcmp(op, "decrement") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val - 1) :
                              v.type == VAL_FLOAT ? make_float(v.float_val - 1) : v);
        } else if (strcmp(op, "toupper") == 0) {
            if (v.type == VAL_STRING) {
                char* s = strdup(v.string_val);
                for (char* p = s; *p; p++) *p = toupper((unsigned char)*p);
                array_push(result, make_string(s)); free(s);
            } else array_push(result, v);
        } else if (strcmp(op, "tolower") == 0) {
            if (v.type == VAL_STRING) {
                char* s = strdup(v.string_val);
                for (char* p = s; *p; p++) *p = tolower((unsigned char)*p);
                array_push(result, make_string(s)); free(s);
            } else array_push(result, v);
        } else if (strcmp(op, "length") == 0) {
            if (v.type == VAL_STRING) array_push(result, make_int((long long)strlen(v.string_val)));
            else if (v.type == VAL_ARRAY) array_push(result, make_int((long long)v.array_val->count));
            else array_push(result, make_int(0));
        } else if (strcmp(op, "type") == 0) {
            const char* t = "null";
            if (v.type == VAL_BOOL) t = "bool";
            else if (v.type == VAL_INT) t = "int";
            else if (v.type == VAL_FLOAT) t = "float";
            else if (v.type == VAL_STRING) t = "string";
            else if (v.type == VAL_ARRAY) t = "array";
            else if (v.type == VAL_FUNCTION) t = "function";
            else if (v.type == VAL_STRUCT_INSTANCE) t = "struct";
            array_push(result, make_string(t));
        } else if (strcmp(op, "add") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val + (long long)operand) :
                              v.type == VAL_FLOAT ? make_float(v.float_val + operand) : v);
        } else if (strcmp(op, "multiply") == 0) {
            array_push(result, v.type == VAL_INT ? make_int(v.int_val * (long long)operand) :
                              v.type == VAL_FLOAT ? make_float(v.float_val * operand) : v);
        } else {
            array_push(result, v);
        }
    }
    return make_array(result);
}

/* itertools.reduce(arr, operation, [initial]) - Reduce array */
Value native_itertools_reduce(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* op = args[1].string_val;
    if (src->count == 0) return make_null();
    Value acc = src->items[0];
    if (argc >= 3) acc = args[2];
    for (size_t i = (argc >= 3 ? 0 : 1); i < src->count; i++) {
        Value v = src->items[i];
        if (strcmp(op, "sum") == 0 || strcmp(op, "add") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT) acc.int_val += v.int_val;
            else if (acc.type == VAL_FLOAT || v.type == VAL_FLOAT) {
                double a = acc.type == VAL_INT ? (double)acc.int_val : acc.float_val;
                double b = v.type == VAL_INT ? (double)v.int_val : v.float_val;
                acc = make_float(a + b);
            }
        } else if (strcmp(op, "product") == 0 || strcmp(op, "mul") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT) acc.int_val *= v.int_val;
            else {
                double a = acc.type == VAL_INT ? (double)acc.int_val : acc.float_val;
                double b = v.type == VAL_INT ? (double)v.int_val : v.float_val;
                acc = make_float(a * b);
            }
        } else if (strcmp(op, "max") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT && v.int_val > acc.int_val) acc = v;
            else if (acc.type == VAL_FLOAT && v.type == VAL_FLOAT && v.float_val > acc.float_val) acc = v;
        } else if (strcmp(op, "min") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT && v.int_val < acc.int_val) acc = v;
            else if (acc.type == VAL_FLOAT && v.type == VAL_FLOAT && v.float_val < acc.float_val) acc = v;
        } else if (strcmp(op, "concat") == 0) {
            if (acc.type == VAL_STRING && v.type == VAL_STRING) {
                char* buf = malloc(strlen(acc.string_val) + strlen(v.string_val) + 1);
                strcpy(buf, acc.string_val);
                strcat(buf, v.string_val);
                acc = make_string(buf);
                free(buf);
            }
        }
    }
    return acc;
}

/* itertools.takewhile(arr, predicate) - Take while predicate holds */
Value native_itertools_takewhile(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* pred = args[1].string_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value v = src->items[i];
        bool holds = false;
        if (strcmp(pred, "positive") == 0)
            holds = (v.type == VAL_INT && v.int_val > 0) || (v.type == VAL_FLOAT && v.float_val > 0.0);
        else if (strcmp(pred, "negative") == 0)
            holds = (v.type == VAL_INT && v.int_val < 0) || (v.type == VAL_FLOAT && v.float_val < 0.0);
        else if (strcmp(pred, "nonzero") == 0)
            holds = (v.type == VAL_INT && v.int_val != 0) || (v.type == VAL_FLOAT && v.float_val != 0.0);
        else if (strcmp(pred, "nonnull") == 0)
            holds = (v.type != VAL_NULL);
        if (!holds) break;
        array_push(result, v);
    }
    return make_array(result);
}

/* itertools.dropwhile(arr, predicate) - Drop while predicate holds */
Value native_itertools_dropwhile(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* pred = args[1].string_val;
    Array* result = array_create(src->count);
    bool dropping = true;
    for (size_t i = 0; i < src->count; i++) {
        Value v = src->items[i];
        if (dropping) {
            bool holds = false;
            if (strcmp(pred, "positive") == 0)
                holds = (v.type == VAL_INT && v.int_val > 0) || (v.type == VAL_FLOAT && v.float_val > 0.0);
            else if (strcmp(pred, "negative") == 0)
                holds = (v.type == VAL_INT && v.int_val < 0) || (v.type == VAL_FLOAT && v.float_val < 0.0);
            else if (strcmp(pred, "nonzero") == 0)
                holds = (v.type == VAL_INT && v.int_val != 0) || (v.type == VAL_FLOAT && v.float_val != 0.0);
            else if (strcmp(pred, "nonnull") == 0)
                holds = (v.type != VAL_NULL);
            if (!holds) { dropping = false; array_push(result, v); }
        } else {
            array_push(result, v);
        }
    }
    return make_array(result);
}

/* itertools.combinations(arr, r) - Generate combinations of length r */
Value native_itertools_combinations(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    long long r = (long long)src->count;
    if (argc >= 2 && args[1].type == VAL_INT && args[1].int_val > 0) r = args[1].int_val;
    if (r <= 0 || r > (long long)src->count) return make_array(array_create(0));
    size_t n = src->count;
    /* Iterative combinations using index array */
    size_t* idx = malloc(sizeof(size_t) * (size_t)r);
    for (long long i = 0; i < r; i++) idx[i] = (size_t)i;
    Array* result = array_create(64);
    /* Generate first combination */
    while (1) {
        Array* combo = array_create((size_t)r);
        for (long long j = 0; j < r; j++) array_push(combo, src->items[idx[j]]);
        array_push(result, make_array(combo));
        /* Find rightmost index that can be incremented */
        long long k;
        for (k = r - 1; k >= 0; k--) {
            if (idx[k] != n - (size_t)(r - k)) break;
        }
        if (k < 0) break;
        idx[k]++;
        for (long long j = k + 1; j < r; j++) idx[j] = idx[j-1] + 1;
    }
    free(idx);
    return make_array(result);
}

/* itertools.permutations(arr, [r]) - Generate permutations (limited to small arrays) */
Value native_itertools_permutations(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    long long r = (long long)src->count;
    if (argc >= 2 && args[1].type == VAL_INT && args[1].int_val > 0) r = args[1].int_val;
    if (r <= 0 || r > (long long)src->count || src->count == 0)
        return make_array(array_create(0));
    size_t n = src->count;
    if (n > 8) r = r > 8 ? 8 : r; /* cap for safety */
    /* Use Heap's algorithm on full array, collect first-r of each perm */
    size_t* idx = malloc(sizeof(size_t) * n);
    for (size_t i = 0; i < n; i++) idx[i] = i;
    size_t* c = calloc(n, sizeof(size_t));
    Array* result = array_create(128);
    /* First permutation */
    {
        Array* perm = array_create((size_t)r);
        for (long long j = 0; j < r; j++) array_push(perm, src->items[idx[j]]);
        array_push(result, make_array(perm));
    }
    size_t i = 0;
    while (i < n) {
        if (c[i] < i) {
            if (i % 2 == 0) { size_t tmp = idx[0]; idx[0] = idx[i]; idx[i] = tmp; }
            else { size_t tmp = idx[c[i]]; idx[c[i]] = idx[i]; idx[i] = tmp; }
            Array* perm = array_create((size_t)r);
            for (long long j = 0; j < r; j++) array_push(perm, src->items[idx[j]]);
            array_push(result, make_array(perm));
            c[i]++;
            i = 0;
        } else {
            c[i] = 0;
            i++;
        }
    }
    free(idx);
    free(c);
    return make_array(result);
}

/* itertools.product(a, b) - Cartesian product of two arrays */
Value native_itertools_product(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val, *b = args[1].array_val;
    Array* result = array_create(a->count * b->count);
    for (size_t i = 0; i < a->count; i++) {
        for (size_t j = 0; j < b->count; j++) {
            Array* pair = array_create(2);
            array_push(pair, a->items[i]);
            array_push(pair, b->items[j]);
            array_push(result, make_array(pair));
        }
    }
    return make_array(result);
}

/* itertools.unique(arr) - Remove duplicates (preserves order) */
Value native_itertools_unique(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value v = src->items[i];
        bool dup = false;
        for (size_t j = 0; j < result->count; j++) {
            Value rv = result->items[j];
            if (v.type == rv.type) {
                if (v.type == VAL_INT && v.int_val == rv.int_val) { dup = true; break; }
                if (v.type == VAL_FLOAT && v.float_val == rv.float_val) { dup = true; break; }
                if (v.type == VAL_STRING && strcmp(v.string_val, rv.string_val) == 0) { dup = true; break; }
                if (v.type == VAL_BOOL && v.bool_val == rv.bool_val) { dup = true; break; }
                if (v.type == VAL_NULL) { dup = true; break; }
            }
        }
        if (!dup) array_push(result, v);
    }
    return make_array(result);
}

/* itertools.flatten(arr) - Flatten one level of nesting */
Value native_itertools_flatten(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        if (src->items[i].type == VAL_ARRAY) {
            Array* inner = src->items[i].array_val;
            for (size_t j = 0; j < inner->count; j++)
                array_push(result, inner->items[j]);
        } else {
            array_push(result, src->items[i]);
        }
    }
    return make_array(result);
}

/* itertools.groupby(arr, key) - Group by named key function */
Value native_itertools_groupby(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* key = args[1].string_val;
    /* Groups: array of arrays, each group has same key value */
    Array* groups = array_create(16);
    Array* keys_seen = array_create(16);
    for (size_t i = 0; i < src->count; i++) {
        Value v = src->items[i];
        Value kv = make_null();
        if (strcmp(key, "type") == 0) {
            const char* t = "null";
            if (v.type == VAL_INT) t = "int";
            else if (v.type == VAL_FLOAT) t = "float";
            else if (v.type == VAL_STRING) t = "string";
            else if (v.type == VAL_BOOL) t = "bool";
            else if (v.type == VAL_ARRAY) t = "array";
            kv = make_string(t);
        } else if (strcmp(key, "parity") == 0) {
            if (v.type == VAL_INT)
                kv = make_string(v.int_val % 2 == 0 ? "even" : "odd");
        } else if (strcmp(key, "sign") == 0) {
            if (v.type == VAL_INT)
                kv = make_string(v.int_val > 0 ? "positive" : v.int_val < 0 ? "negative" : "zero");
            else if (v.type == VAL_FLOAT)
                kv = make_string(v.float_val > 0 ? "positive" : v.float_val < 0 ? "negative" : "zero");
        }
        /* Find existing group */
        int found = -1;
        for (size_t g = 0; g < keys_seen->count; g++) {
            Value gk = keys_seen->items[g];
            if (gk.type == VAL_STRING && kv.type == VAL_STRING && strcmp(gk.string_val, kv.string_val) == 0) {
                found = (int)g; break;
            }
        }
        if (found >= 0) {
            array_push(groups->items[found].array_val, v);
        } else {
            Array* grp = array_create(8);
            array_push(grp, v);
            array_push(groups, make_array(grp));
            array_push(keys_seen, kv);
        }
    }
    return make_array(groups);
}

/* itertools.starmap(arr_of_pairs, operation) - Like map but unpacks inner arrays */
Value native_itertools_starmap(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();
    Array* src = args[0].array_val;
    const char* op = args[1].string_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        if (src->items[i].type != VAL_ARRAY) { array_push(result, src->items[i]); continue; }
        Array* pair = src->items[i].array_val;
        if (pair->count < 2) { array_push(result, src->items[i]); continue; }
        Value a = pair->items[0], b = pair->items[1];
        if (strcmp(op, "add") == 0) {
            if (a.type == VAL_INT && b.type == VAL_INT) array_push(result, make_int(a.int_val + b.int_val));
            else if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
                array_push(result, make_float((a.type==VAL_INT?(double)a.int_val:a.float_val) + (b.type==VAL_INT?(double)b.int_val:b.float_val)));
        } else if (strcmp(op, "sub") == 0) {
            if (a.type == VAL_INT && b.type == VAL_INT) array_push(result, make_int(a.int_val - b.int_val));
        } else if (strcmp(op, "mul") == 0) {
            if (a.type == VAL_INT && b.type == VAL_INT) array_push(result, make_int(a.int_val * b.int_val));
        } else if (strcmp(op, "div") == 0) {
            if (a.type == VAL_INT && b.type == VAL_INT && b.int_val != 0)
                array_push(result, make_float((double)a.int_val / (double)b.int_val));
        } else if (strcmp(op, "max") == 0) {
            if (a.type == VAL_INT && b.type == VAL_INT)
                array_push(result, make_int(a.int_val > b.int_val ? a.int_val : b.int_val));
        } else if (strcmp(op, "min") == 0) {
            if (a.type == VAL_INT && b.type == VAL_INT)
                array_push(result, make_int(a.int_val < b.int_val ? a.int_val : b.int_val));
        } else {
            array_push(result, src->items[i]);
        }
    }
    return make_array(result);
}

/* itertools.accumulate(arr, [operation]) - Running accumulate */
Value native_itertools_accumulate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* src = args[0].array_val;
    const char* op = "sum";
    if (argc >= 2 && args[1].type == VAL_STRING) op = args[1].string_val;
    if (src->count == 0) return make_array(array_create(0));
    Array* result = array_create(src->count);
    Value acc = src->items[0];
    array_push(result, acc);
    for (size_t i = 1; i < src->count; i++) {
        Value v = src->items[i];
        if (strcmp(op, "sum") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT) acc = make_int(acc.int_val + v.int_val);
            else acc = make_float((acc.type==VAL_INT?(double)acc.int_val:acc.float_val) + (v.type==VAL_INT?(double)v.int_val:v.float_val));
        } else if (strcmp(op, "mul") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT) acc = make_int(acc.int_val * v.int_val);
            else acc = make_float((acc.type==VAL_INT?(double)acc.int_val:acc.float_val) * (v.type==VAL_INT?(double)v.int_val:v.float_val));
        } else if (strcmp(op, "max") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT) acc = make_int(acc.int_val > v.int_val ? acc.int_val : v.int_val);
        } else if (strcmp(op, "min") == 0) {
            if (acc.type == VAL_INT && v.type == VAL_INT) acc = make_int(acc.int_val < v.int_val ? acc.int_val : v.int_val);
        }
        array_push(result, acc);
    }
    return make_array(result);
}

/* Registration */
extern void register_native_fn(const char* module, const char* name, NativeFn fn);

void stdlib_itertools_register(void) {
    register_native_fn("itertools", "count", native_itertools_count);
    register_native_fn("itertools", "cycle", native_itertools_cycle);
    register_native_fn("itertools", "repeat", native_itertools_repeat);
    register_native_fn("itertools", "chain", native_itertools_chain);
    register_native_fn("itertools", "zip", native_itertools_zip);
    register_native_fn("itertools", "enumerate", native_itertools_enumerate);
    register_native_fn("itertools", "islice", native_itertools_islice);
    register_native_fn("itertools", "filter", native_itertools_filter);
    register_native_fn("itertools", "map", native_itertools_map);
    register_native_fn("itertools", "reduce", native_itertools_reduce);
    register_native_fn("itertools", "takewhile", native_itertools_takewhile);
    register_native_fn("itertools", "dropwhile", native_itertools_dropwhile);
    register_native_fn("itertools", "combinations", native_itertools_combinations);
    register_native_fn("itertools", "permutations", native_itertools_permutations);
    register_native_fn("itertools", "product", native_itertools_product);
    register_native_fn("itertools", "unique", native_itertools_unique);
    register_native_fn("itertools", "flatten", native_itertools_flatten);
    register_native_fn("itertools", "groupby", native_itertools_groupby);
    register_native_fn("itertools", "starmap", native_itertools_starmap);
    register_native_fn("itertools", "accumulate", native_itertools_accumulate);
}
