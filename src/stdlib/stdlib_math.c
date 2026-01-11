#define _POSIX_C_SOURCE 200809L
#include "stdlib_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static bool check_argc(int argc, int expected) {
    return argc == expected;
}

static double value_to_double(Value* v) {
    if (v->type == VAL_INT) return (double)v->int_val;
    if (v->type == VAL_FLOAT) return v->float_val;
    return 0.0;
}

Value native_math_abs(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    if (args[0].type == VAL_INT) {
        Value v;
        v.type = VAL_INT;
        v.int_val = llabs(args[0].int_val);
        return v;
    } else if (args[0].type == VAL_FLOAT) {
        Value v;
        v.type = VAL_FLOAT;
        v.float_val = fabs(args[0].float_val);
        return v;
    }
    
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value native_math_sqrt(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = sqrt(val);
    return v;
}

Value native_math_pow(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double base = value_to_double(&args[0]);
    double exp = value_to_double(&args[1]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = pow(base, exp);
    return v;
}

Value native_math_floor(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_INT;
    v.int_val = (long long)floor(val);
    return v;
}

Value native_math_ceil(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_INT;
    v.int_val = (long long)ceil(val);
    return v;
}

Value native_math_round(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_INT;
    v.int_val = (long long)round(val);
    return v;
}

Value native_math_sin(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = sin(val);
    return v;
}

Value native_math_cos(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = cos(val);
    return v;
}

Value native_math_tan(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    double val = value_to_double(&args[0]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = tan(val);
    return v;
}

Value native_math_min(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        Value v;
        v.type = VAL_INT;
        v.int_val = (args[0].int_val < args[1].int_val) ? args[0].int_val : args[1].int_val;
        return v;
    }
    
    double a = value_to_double(&args[0]);
    double b = value_to_double(&args[1]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = (a < b) ? a : b;
    return v;
}

Value native_math_max(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        Value v;
        v.type = VAL_INT;
        v.int_val = (args[0].int_val > args[1].int_val) ? args[0].int_val : args[1].int_val;
        return v;
    }
    
    double a = value_to_double(&args[0]);
    double b = value_to_double(&args[1]);
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = (a > b) ? a : b;
    return v;
}

static bool random_initialized = false;

Value native_math_random(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    if (!random_initialized) {
        srand(time(NULL));
        random_initialized = true;
    }
    
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = (double)rand() / (double)RAND_MAX;
    return v;
}

Value native_math_random_int(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (!random_initialized) {
        srand(time(NULL));
        random_initialized = true;
    }

    long long min = 0;
    long long max = 100;

    if (argc >= 1 && args[0].type == VAL_INT) {
        max = args[0].int_val;
    }
    if (argc >= 2 && args[1].type == VAL_INT) {
        min = args[0].int_val;
        max = args[1].int_val;
    }

    if (min > max) {
        long long temp = min;
        min = max;
        max = temp;
    }

    Value v;
    v.type = VAL_INT;
    v.int_val = min + (rand() % (max - min + 1));
    return v;
}

Value native_math_log(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = value_to_double(&args[0]);

    Value v;
    v.type = VAL_FLOAT;
    v.float_val = log(val);
    return v;
}

Value native_math_log10(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = value_to_double(&args[0]);

    Value v;
    v.type = VAL_FLOAT;
    v.float_val = log10(val);
    return v;
}

Value native_math_exp(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (!check_argc(argc, 1)) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = value_to_double(&args[0]);

    Value v;
    v.type = VAL_FLOAT;
    v.float_val = exp(val);
    return v;
}

Value native_math_clamp(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc != 3) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double val = value_to_double(&args[0]);
    double min_val = value_to_double(&args[1]);
    double max_val = value_to_double(&args[2]);

    if (val < min_val) val = min_val;
    if (val > max_val) val = max_val;

    Value v;
    v.type = VAL_FLOAT;
    v.float_val = val;
    return v;
}

Value native_math_lerp(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc != 3) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    double a = value_to_double(&args[0]);
    double b = value_to_double(&args[1]);
    double t = value_to_double(&args[2]);

    Value v;
    v.type = VAL_FLOAT;
    v.float_val = a + (b - a) * t;
    return v;
}

void stdlib_math_register(void) {
    register_native("math.abs", native_math_abs);
    register_native("math.sqrt", native_math_sqrt);
    register_native("math.pow", native_math_pow);
    register_native("math.floor", native_math_floor);
    register_native("math.ceil", native_math_ceil);
    register_native("math.round", native_math_round);
    register_native("math.sin", native_math_sin);
    register_native("math.cos", native_math_cos);
    register_native("math.tan", native_math_tan);
    register_native("math.min", native_math_min);
    register_native("math.max", native_math_max);
    register_native("math.random", native_math_random);
    register_native("math.random_int", native_math_random_int);
    register_native("math.log", native_math_log);
    register_native("math.log10", native_math_log10);
    register_native("math.exp", native_math_exp);
    register_native("math.clamp", native_math_clamp);
    register_native("math.lerp", native_math_lerp);
}
