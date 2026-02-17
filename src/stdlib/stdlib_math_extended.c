#include "../core/interpreter.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

extern Value make_int(long long val);
extern Value make_float(double val);

static bool random_initialized = false;

static void init_random(void) {
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = true;
    }
}

Value stdlib_math_min(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) {
        fprintf(stderr, "Error: math.min() requires at least 2 arguments\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double min_val;
    bool is_int = true;
    
    if (args[0].type == VAL_INT) {
        min_val = (double)args[0].int_val;
    } else if (args[0].type == VAL_FLOAT) {
        min_val = args[0].float_val;
        is_int = false;
    } else {
        fprintf(stderr, "Error: math.min() arguments must be numbers\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    for (int i = 1; i < argc; i++) {
        double val;
        if (args[i].type == VAL_INT) {
            val = (double)args[i].int_val;
        } else if (args[i].type == VAL_FLOAT) {
            val = args[i].float_val;
            is_int = false;
        } else {
            continue;
        }
        if (val < min_val) min_val = val;
    }

    return is_int ? make_int((long long)min_val) : make_float(min_val);
}

Value stdlib_math_max(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) {
        fprintf(stderr, "Error: math.max() requires at least 2 arguments\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double max_val;
    bool is_int = true;
    
    if (args[0].type == VAL_INT) {
        max_val = (double)args[0].int_val;
    } else if (args[0].type == VAL_FLOAT) {
        max_val = args[0].float_val;
        is_int = false;
    } else {
        fprintf(stderr, "Error: math.max() arguments must be numbers\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    for (int i = 1; i < argc; i++) {
        double val;
        if (args[i].type == VAL_INT) {
            val = (double)args[i].int_val;
        } else if (args[i].type == VAL_FLOAT) {
            val = args[i].float_val;
            is_int = false;
        } else {
            continue;
        }
        if (val > max_val) max_val = val;
    }

    return is_int ? make_int((long long)max_val) : make_float(max_val);
}

Value stdlib_math_clamp(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3) {
        fprintf(stderr, "Error: math.clamp() requires 3 arguments (value, min, max)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val, min_val, max_val;
    bool is_int = (args[0].type == VAL_INT && args[1].type == VAL_INT && args[2].type == VAL_INT);
    
    val = (args[0].type == VAL_INT) ? (double)args[0].int_val : args[0].float_val;
    min_val = (args[1].type == VAL_INT) ? (double)args[1].int_val : args[1].float_val;
    max_val = (args[2].type == VAL_INT) ? (double)args[2].int_val : args[2].float_val;
    
    if (val < min_val) val = min_val;
    if (val > max_val) val = max_val;
    
    return is_int ? make_int((long long)val) : make_float(val);
}

Value stdlib_math_floor(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: math.floor() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = (args[0].type == VAL_INT) ? (double)args[0].int_val : args[0].float_val;
    return make_int((long long)floor(val));
}

Value stdlib_math_ceil(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: math.ceil() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = (args[0].type == VAL_INT) ? (double)args[0].int_val : args[0].float_val;
    return make_int((long long)ceil(val));
}

Value stdlib_math_round(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: math.round() requires 1 argument\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = (args[0].type == VAL_INT) ? (double)args[0].int_val : args[0].float_val;
    return make_int((long long)round(val));
}

Value stdlib_math_random(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    init_random();
    
    double random_val = (double)rand() / (double)RAND_MAX;
    return make_float(random_val);
}

Value stdlib_math_srand(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) {
        fprintf(stderr, "Error: math.srand() requires 1 integer argument (seed)\n");
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    srand((unsigned int)args[0].int_val);
    random_initialized = true;
    
    Value v;
    v.type = VAL_NULL;
    return v;
}

void stdlib_math_extended_register(void) {
    register_native("math.min", (NativeFn)stdlib_math_min);
    register_native("math.max", (NativeFn)stdlib_math_max);
    register_native("math.clamp", (NativeFn)stdlib_math_clamp);
    register_native("math.floor", (NativeFn)stdlib_math_floor);
    register_native("math.ceil", (NativeFn)stdlib_math_ceil);
    register_native("math.round", (NativeFn)stdlib_math_round);
    register_native("math.random", (NativeFn)stdlib_math_random);
    register_native("math.srand", (NativeFn)stdlib_math_srand);
}
