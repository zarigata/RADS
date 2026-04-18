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

Value native_math_atan2(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    double y = value_to_double(&args[0]);
    double x = value_to_double(&args[1]);
    Value v; v.type = VAL_FLOAT; v.float_val = atan2(y, x); return v;
}

Value native_math_hypot(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    double a = value_to_double(&args[0]);
    double b = value_to_double(&args[1]);
    Value v; v.type = VAL_FLOAT; v.float_val = hypot(a, b); return v;
}

Value native_math_sign(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    double val = value_to_double(&args[0]);
    Value v; v.type = VAL_INT;
    v.int_val = (val > 0) ? 1 : (val < 0) ? -1 : 0;
    return v;
}

Value native_math_pi(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    Value v; v.type = VAL_FLOAT; v.float_val = 3.14159265358979323846; return v;
}

Value native_math_e(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    Value v; v.type = VAL_FLOAT; v.float_val = 2.71828182845904523536; return v;
}

Value native_math_sum(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_int(0);
    Array* arr = args[0].array_val;
    double total = 0;
    for (size_t i = 0; i < arr->count; i++) {
        total += value_to_double(&arr->items[i]);
    }
    /* Return int if all were ints */
    int all_int = 1;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type != VAL_INT) { all_int = 0; break; }
    }
    if (all_int) { Value v; v.type = VAL_INT; v.int_val = (long long)total; return v; }
    Value v; v.type = VAL_FLOAT; v.float_val = total; return v;
}

Value native_math_avg(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) {
        Value v; v.type = VAL_FLOAT; v.float_val = 0.0; return v;
    }
    Array* arr = args[0].array_val;
    double total = 0;
    for (size_t i = 0; i < arr->count; i++) {
        total += value_to_double(&arr->items[i]);
    }
    Value v; v.type = VAL_FLOAT; v.float_val = total / (double)arr->count; return v;
}

Value native_math_median(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || args[0].array_val->count == 0) {
        Value v; v.type = VAL_FLOAT; v.float_val = 0.0; return v;
    }
    Array* arr = args[0].array_val;
    size_t n = arr->count;
    /* Copy values to temp array and sort */
    double* vals = malloc(n * sizeof(double));
    for (size_t i = 0; i < n; i++) vals[i] = value_to_double(&arr->items[i]);
    /* Simple insertion sort (n is typically small) */
    for (size_t i = 1; i < n; i++) {
        double key = vals[i];
        long long j = (long long)i - 1;
        while (j >= 0 && vals[j] > key) { vals[j+1] = vals[j]; j--; }
        vals[j+1] = key;
    }
    double med;
    if (n % 2 == 0) med = (vals[n/2 - 1] + vals[n/2]) / 2.0;
    else            med = vals[n/2];
    free(vals);
    Value v; v.type = VAL_FLOAT; v.float_val = med; return v;
}

Value native_math_deg_to_rad(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    double deg = value_to_double(&args[0]);
    Value v; v.type = VAL_FLOAT; v.float_val = deg * 3.14159265358979323846 / 180.0; return v;
}

Value native_math_rad_to_deg(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    double rad = value_to_double(&args[0]);
    Value v; v.type = VAL_FLOAT; v.float_val = rad * 180.0 / 3.14159265358979323846; return v;
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
    register_native("math.atan2", native_math_atan2);
    register_native("math.hypot", native_math_hypot);
    register_native("math.sign", native_math_sign);
    register_native("math.PI", native_math_pi);
    register_native("math.E", native_math_e);
    register_native("math.sum", native_math_sum);
    register_native("math.avg", native_math_avg);
    register_native("math.median", native_math_median);
    register_native("math.deg_to_rad", native_math_deg_to_rad);
    register_native("math.rad_to_deg", native_math_rad_to_deg);
}
