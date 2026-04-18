#include "stdlib_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void value_free(Value* value);
extern Value interpreter_execute_callback(Value func_val, int argc, Value* args);

Value stdlib_array_map(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.map() requires 2 arguments (array, function)\n");
        return make_null();
    }

    Value arr_val = args[0];
    Value func_val = args[1];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.map() first argument must be an array\n");
        return make_null();
    }

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: array.map() second argument must be a function\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;
    Array* result_arr = array_create(arr->count);

    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[1] = { arr->items[i] };
        Value result = interpreter_execute_callback(func_val, 1, call_args);
        array_push(result_arr, result);
        value_free(&result);
    }

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

Value stdlib_array_filter(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.filter() requires 2 arguments (array, predicate)\n");
        return make_null();
    }

    Value arr_val = args[0];
    Value func_val = args[1];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.filter() first argument must be an array\n");
        return make_null();
    }

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: array.filter() second argument must be a function\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;
    Array* result_arr = array_create(arr->count);

    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[1] = { arr->items[i] };
        Value result = interpreter_execute_callback(func_val, 1, call_args);
        
        bool keep = false;
        if (result.type == VAL_BOOL && result.bool_val) {
            keep = true;
        } else if (result.type == VAL_INT && result.int_val != 0) {
            keep = true;
        }
        
        if (keep) {
            array_push(result_arr, arr->items[i]);
        }
        value_free(&result);
    }

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

Value stdlib_array_reduce(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3) {
        fprintf(stderr, "Error: array.reduce() requires 3 arguments (array, reducer, initial)\n");
        return make_null();
    }

    Value arr_val = args[0];
    Value func_val = args[1];
    Value accumulator = args[2];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.reduce() first argument must be an array\n");
        return make_null();
    }

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: array.reduce() second argument must be a function\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[2] = { accumulator, arr->items[i] };
        Value result = interpreter_execute_callback(func_val, 2, call_args);
        value_free(&accumulator);
        accumulator = result;
    }

    return accumulator;
}

Value stdlib_array_find(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.find() requires 2 arguments (array, predicate)\n");
        return make_null();
    }

    Value arr_val = args[0];
    Value func_val = args[1];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.find() first argument must be an array\n");
        return make_null();
    }

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: array.find() second argument must be a function\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[1] = { arr->items[i] };
        Value result = interpreter_execute_callback(func_val, 1, call_args);
        
        bool found = false;
        if (result.type == VAL_BOOL && result.bool_val) {
            found = true;
        } else if (result.type == VAL_INT && result.int_val != 0) {
            found = true;
        }
        
        value_free(&result);
        
        if (found) {
            return arr->items[i];
        }
    }

    return make_null();
}

Value stdlib_array_some(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.some() requires 2 arguments (array, predicate)\n");
        return make_bool(false);
    }

    Value arr_val = args[0];
    Value func_val = args[1];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.some() first argument must be an array\n");
        return make_bool(false);
    }

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: array.some() second argument must be a function\n");
        return make_bool(false);
    }

    Array* arr = arr_val.array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[1] = { arr->items[i] };
        Value result = interpreter_execute_callback(func_val, 1, call_args);
        
        bool found = false;
        if (result.type == VAL_BOOL && result.bool_val) {
            found = true;
        } else if (result.type == VAL_INT && result.int_val != 0) {
            found = true;
        }
        
        value_free(&result);
        
        if (found) {
            return make_bool(true);
        }
    }

    return make_bool(false);
}

Value stdlib_array_every(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2) {
        fprintf(stderr, "Error: array.every() requires 2 arguments (array, predicate)\n");
        return make_bool(false);
    }

    Value arr_val = args[0];
    Value func_val = args[1];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.every() first argument must be an array\n");
        return make_bool(false);
    }

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Error: array.every() second argument must be a function\n");
        return make_bool(false);
    }

    Array* arr = arr_val.array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value call_args[1] = { arr->items[i] };
        Value result = interpreter_execute_callback(func_val, 1, call_args);
        
        bool passes = false;
        if (result.type == VAL_BOOL && result.bool_val) {
            passes = true;
        } else if (result.type == VAL_INT && result.int_val != 0) {
            passes = true;
        }
        
        value_free(&result);
        
        if (!passes) {
            return make_bool(false);
        }
    }

    return make_bool(true);
}

Value stdlib_array_sort(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 1) {
        fprintf(stderr, "Error: array.sort() requires 1 argument (array)\n");
        return make_null();
    }

    Value arr_val = args[0];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.sort() argument must be an array\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;
    
    for (size_t i = 0; i < arr->count; i++) {
        for (size_t j = i + 1; j < arr->count; j++) {
            bool should_swap = false;
            
            if (arr->items[i].type == VAL_INT && arr->items[j].type == VAL_INT) {
                should_swap = arr->items[i].int_val > arr->items[j].int_val;
            } else if (arr->items[i].type == VAL_FLOAT && arr->items[j].type == VAL_FLOAT) {
                should_swap = arr->items[i].float_val > arr->items[j].float_val;
            }
            
            if (should_swap) {
                Value temp = arr->items[i];
                arr->items[i] = arr->items[j];
                arr->items[j] = temp;
            }
        }
    }
    
    return make_null();
}

Value stdlib_array_reverse(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 1) {
        fprintf(stderr, "Error: array.reverse() requires 1 argument (array)\n");
        return make_null();
    }

    Value arr_val = args[0];

    if (arr_val.type != VAL_ARRAY) {
        fprintf(stderr, "Error: array.reverse() argument must be an array\n");
        return make_null();
    }

    Array* arr = arr_val.array_val;
    
    for (size_t i = 0; i < arr->count / 2; i++) {
        Value temp = arr->items[i];
        arr->items[i] = arr->items[arr->count - 1 - i];
        arr->items[arr->count - 1 - i] = temp;
    }
    
    return make_null();
}

/* ── value equality helper ── */
static bool value_equals(Value* a, Value* b) {
    if (a->type != b->type) return false;
    switch (a->type) {
        case VAL_INT:    return a->int_val == b->int_val;
        case VAL_FLOAT:  return a->float_val == b->float_val;
        case VAL_BOOL:   return a->bool_val == b->bool_val;
        case VAL_STRING: return strcmp(a->string_val, b->string_val) == 0;
        case VAL_NULL:   return true;
        default:         return false;
    }
}

/* array.includes(arr, value) → bool */
Value stdlib_array_includes(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_bool(false);
    Array* arr = args[0].array_val;
    for (size_t i = 0; i < arr->count; i++) {
        if (value_equals(&arr->items[i], &args[1])) return make_bool(true);
    }
    return make_bool(false);
}

/* array.index_of(arr, value) → int (-1 if not found) */
Value stdlib_array_index_of(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_int(-1);
    Array* arr = args[0].array_val;
    for (size_t i = 0; i < arr->count; i++) {
        if (value_equals(&arr->items[i], &args[1])) return make_int((long long)i);
    }
    return make_int(-1);
}

/* array.last_index_of(arr, value) → int */
Value stdlib_array_last_index_of(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_int(-1);
    Array* arr = args[0].array_val;
    for (size_t i = arr->count; i > 0; i--) {
        if (value_equals(&arr->items[i-1], &args[1])) return make_int((long long)(i-1));
    }
    return make_int(-1);
}

/* array.flat(arr, depth) → flattened array (depth defaults to 1) */
static void flatten_recursive(Array* src, Array* dst, int depth) {
    for (size_t i = 0; i < src->count; i++) {
        if (depth > 0 && src->items[i].type == VAL_ARRAY) {
            flatten_recursive(src->items[i].array_val, dst, depth - 1);
        } else {
            array_push(dst, src->items[i]);
        }
    }
}

Value stdlib_array_flat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    int depth = 1;
    if (argc >= 2 && args[1].type == VAL_INT) depth = (int)args[1].int_val;
    if (depth < 0) depth = 999; /* effectively infinite */
    Array* result = array_create(args[0].array_val->count);
    flatten_recursive(args[0].array_val, result, depth);
    Value v; v.type = VAL_ARRAY; v.array_val = result;
    return v;
}

/* array.slice(arr, start, end) → sub-array */
Value stdlib_array_slice(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    long long start = 0;
    long long end = (long long)arr->count;
    if (argc >= 2 && args[1].type == VAL_INT) start = args[1].int_val;
    if (argc >= 3 && args[2].type == VAL_INT) end = args[2].int_val;

    /* Negative indices */
    if (start < 0) start = (long long)arr->count + start;
    if (end < 0)   end   = (long long)arr->count + end;
    if (start < 0) start = 0;
    if (end > (long long)arr->count) end = (long long)arr->count;
    if (start >= end) {
        Array* empty = array_create(0);
        Value v; v.type = VAL_ARRAY; v.array_val = empty;
        return v;
    }

    size_t len = (size_t)(end - start);
    Array* result = array_create(len);
    for (size_t i = 0; i < len; i++) {
        array_push(result, arr->items[(size_t)start + i]);
    }
    Value v; v.type = VAL_ARRAY; v.array_val = result;
    return v;
}

/* array.unique(arr) → array with duplicates removed */
Value stdlib_array_unique(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    Array* result = array_create(arr->count);
    for (size_t i = 0; i < arr->count; i++) {
        int dup = 0;
        for (size_t j = 0; j < result->count; j++) {
            if (value_equals(&arr->items[i], &result->items[j])) { dup = 1; break; }
        }
        if (!dup) array_push(result, arr->items[i]);
    }
    Value v; v.type = VAL_ARRAY; v.array_val = result;
    return v;
}

/* array.count(arr) → int */
Value stdlib_array_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_int(0);
    return make_int((long long)args[0].array_val->count);
}

/* array.is_empty(arr) → bool */
Value stdlib_array_is_empty(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_bool(true);
    return make_bool(args[0].array_val->count == 0);
}

/* array.concat(arr1, arr2) → new array */
Value stdlib_array_concat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return make_null();
    Array* a = args[0].array_val;
    Array* b = args[1].array_val;
    Array* result = array_create(a->count + b->count);
    for (size_t i = 0; i < a->count; i++) array_push(result, a->items[i]);
    for (size_t i = 0; i < b->count; i++) array_push(result, b->items[i]);
    Value v; v.type = VAL_ARRAY; v.array_val = result;
    return v;
}

/* array.push(arr, value) → arr (mutating) */
Value stdlib_array_push_val(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_null();
    array_push(args[0].array_val, args[1]);
    return args[0];
}

/* array.pop(arr) → removed value (mutating) */
Value stdlib_array_pop(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    if (arr->count == 0) return make_null();
    Value v = arr->items[arr->count - 1];
    arr->count--;
    return v;
}

/* array.shift(arr) → removed value (mutating) */
Value stdlib_array_shift(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    if (arr->count == 0) return make_null();
    Value v = arr->items[0];
    memmove(arr->items, arr->items + 1, (arr->count - 1) * sizeof(Value));
    arr->count--;
    return v;
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
    register_native("array.includes", (NativeFn)stdlib_array_includes);
    register_native("array.index_of", (NativeFn)stdlib_array_index_of);
    register_native("array.last_index_of", (NativeFn)stdlib_array_last_index_of);
    register_native("array.flat", (NativeFn)stdlib_array_flat);
    register_native("array.slice", (NativeFn)stdlib_array_slice);
    register_native("array.unique", (NativeFn)stdlib_array_unique);
    register_native("array.count", (NativeFn)stdlib_array_count);
    register_native("array.is_empty", (NativeFn)stdlib_array_is_empty);
    register_native("array.concat", (NativeFn)stdlib_array_concat);
    register_native("array.push", (NativeFn)stdlib_array_push_val);
    register_native("array.pop", (NativeFn)stdlib_array_pop);
    register_native("array.shift", (NativeFn)stdlib_array_shift);
}