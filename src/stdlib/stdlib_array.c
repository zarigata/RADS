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