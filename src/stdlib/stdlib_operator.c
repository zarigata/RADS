/*
 * stdlib_operator.c — Standard operators as functions for RADS
 *
 * Python-inspired operator module providing comparison, arithmetic, and
 * utility operators as callable functions. Essential for functional programming
 * with functools.map/filter/reduce.
 *
 * Comparison:  eq, ne, lt, le, gt, ge
 * Arithmetic:  add, sub, mul, truediv, floordiv, mod, neg, pos, abs
 * Bitwise:     band, bor, bxor, bnot, shl, shr
 * Logical:     and, or, not
 * Sequence:    concat, contains, count_of, index_of
 * Type checks: is_number, is_string, is_array, is_bool, is_null
 * Utility:     item, set_item, length
 */

#include "stdlib_operator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Helpers ──────────────────────────────────────────────── */

static double to_double(Value* v) {
    if (v->type == VAL_INT) return (double)v->int_val;
    if (v->type == VAL_FLOAT) return v->float_val;
    return 0.0;
}

static int compare_values(Value* a, Value* b) {
    /* Same type comparison */
    if (a->type == VAL_INT && b->type == VAL_INT) {
        if (a->int_val < b->int_val) return -1;
        if (a->int_val > b->int_val) return 1;
        return 0;
    }
    if (a->type == VAL_FLOAT && b->type == VAL_FLOAT) {
        if (a->float_val < b->float_val) return -1;
        if (a->float_val > b->float_val) return 1;
        return 0;
    }
    if ((a->type == VAL_INT || a->type == VAL_FLOAT) &&
        (b->type == VAL_INT || b->type == VAL_FLOAT)) {
        double da = to_double(a), db = to_double(b);
        if (da < db) return -1;
        if (da > db) return 1;
        return 0;
    }
    if (a->type == VAL_STRING && b->type == VAL_STRING) {
        return strcmp(a->string_val, b->string_val);
    }
    if (a->type == VAL_BOOL && b->type == VAL_BOOL) {
        if (a->bool_val == b->bool_val) return 0;
        return a->bool_val ? 1 : -1;
    }
    /* Different types: compare type ordinal */
    if (a->type < b->type) return -1;
    if (a->type > b->type) return 1;
    return 0;
}

/* ── Comparison operators ─────────────────────────────────── */

Value stdlib_op_eq(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(compare_values(&args[0], &args[1]) == 0);
}

Value stdlib_op_ne(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(compare_values(&args[0], &args[1]) != 0);
}

Value stdlib_op_lt(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(compare_values(&args[0], &args[1]) < 0);
}

Value stdlib_op_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(compare_values(&args[0], &args[1]) <= 0);
}

Value stdlib_op_gt(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(compare_values(&args[0], &args[1]) > 0);
}

Value stdlib_op_ge(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(compare_values(&args[0], &args[1]) >= 0);
}

/* ── Arithmetic operators ─────────────────────────────────── */

Value stdlib_op_add(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    /* String concatenation */
    if (args[0].type == VAL_STRING && args[1].type == VAL_STRING) {
        size_t la = strlen(args[0].string_val);
        size_t lb = strlen(args[1].string_val);
        char* buf = malloc(la + lb + 1);
        memcpy(buf, args[0].string_val, la);
        memcpy(buf + la, args[1].string_val, lb + 1);
        Value v = make_string(buf);
        free(buf);
        return v;
    }
    /* Numeric addition */
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        return make_int(args[0].int_val + args[1].int_val);
    }
    return make_float(to_double(&args[0]) + to_double(&args[1]));
}

Value stdlib_op_sub(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        return make_int(args[0].int_val - args[1].int_val);
    }
    return make_float(to_double(&args[0]) - to_double(&args[1]));
}

Value stdlib_op_mul(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    /* String repetition */
    if (args[0].type == VAL_STRING && args[1].type == VAL_INT) {
        size_t len = strlen(args[0].string_val);
        long long n = args[1].int_val;
        if (n <= 0) return make_string("");
        char* buf = malloc(len * (size_t)n + 1);
        buf[0] = '\0';
        for (long long i = 0; i < n; i++) {
            memcpy(buf + len * (size_t)i, args[0].string_val, len);
        }
        buf[len * (size_t)n] = '\0';
        Value v = make_string(buf);
        free(buf);
        return v;
    }
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        return make_int(args[0].int_val * args[1].int_val);
    }
    return make_float(to_double(&args[0]) * to_double(&args[1]));
}

Value stdlib_op_truediv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    double b = to_double(&args[1]);
    if (b == 0.0) return make_null();
    return make_float(to_double(&args[0]) / b);
}

Value stdlib_op_floordiv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    double b = to_double(&args[1]);
    if (b == 0.0) return make_null();
    double result = floor(to_double(&args[0]) / b);
    return make_int((long long)result);
}

Value stdlib_op_mod(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        if (args[1].int_val == 0) return make_null();
        long long a = args[0].int_val;
        long long b = args[1].int_val;
        /* C99 behavior: truncate toward zero. Python uses floor mod. */
        long long r = a % b;
        if (r != 0 && ((a < 0) != (b < 0))) r += b;
        return make_int(r);
    }
    double b = to_double(&args[1]);
    if (b == 0.0) return make_null();
    return make_float(fmod(to_double(&args[0]), b));
}

Value stdlib_op_neg(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    if (args[0].type == VAL_INT) return make_int(-args[0].int_val);
    if (args[0].type == VAL_FLOAT) return make_float(-args[0].float_val);
    return make_null();
}

Value stdlib_op_pos(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    if (args[0].type == VAL_INT) return make_int(args[0].int_val);
    if (args[0].type == VAL_FLOAT) return make_float(args[0].float_val);
    return make_null();
}

Value stdlib_op_abs_op(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    if (args[0].type == VAL_INT) return make_int(llabs(args[0].int_val));
    if (args[0].type == VAL_FLOAT) return make_float(fabs(args[0].float_val));
    return make_null();
}

/* ── Bitwise operators ────────────────────────────────────── */

Value stdlib_op_band(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT) return make_null();
    return make_int(args[0].int_val & args[1].int_val);
}

Value stdlib_op_bor(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT) return make_null();
    return make_int(args[0].int_val | args[1].int_val);
}

Value stdlib_op_bxor(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT) return make_null();
    return make_int(args[0].int_val ^ args[1].int_val);
}

Value stdlib_op_bnot(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    return make_int(~args[0].int_val);
}

Value stdlib_op_shl(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT) return make_null();
    return make_int(args[0].int_val << args[1].int_val);
}

Value stdlib_op_shr(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT) return make_null();
    return make_int(args[0].int_val >> args[1].int_val);
}

/* ── Logical operators ────────────────────────────────────── */

static bool is_truthy(Value* v) {
    if (v->type == VAL_BOOL) return v->bool_val;
    if (v->type == VAL_INT) return v->int_val != 0;
    if (v->type == VAL_FLOAT) return v->float_val != 0.0;
    if (v->type == VAL_STRING) return v->string_val != NULL && v->string_val[0] != '\0';
    if (v->type == VAL_ARRAY) return v->array_val != NULL && v->array_val->count > 0;
    return false;
}

Value stdlib_op_and(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(is_truthy(&args[0]) && is_truthy(&args[1]));
}

Value stdlib_op_or(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    return make_bool(is_truthy(&args[0]) || is_truthy(&args[1]));
}

Value stdlib_op_not(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(true);
    return make_bool(!is_truthy(&args[0]));
}

/* ── Sequence operators ───────────────────────────────────── */

Value stdlib_op_concat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    /* String concat */
    if (args[0].type == VAL_STRING && args[1].type == VAL_STRING) {
        return stdlib_op_add(interp, argc, args);
    }
    /* Array concat */
    if (args[0].type == VAL_ARRAY && args[1].type == VAL_ARRAY) {
        Array* a = args[0].array_val;
        Array* b = args[1].array_val;
        Array* result = array_create(a->count + b->count);
        for (size_t i = 0; i < a->count; i++) array_push(result, a->items[i]);
        for (size_t i = 0; i < b->count; i++) array_push(result, b->items[i]);
        Value v; v.type = VAL_ARRAY; v.array_val = result; return v;
    }
    return make_null();
}

Value stdlib_op_contains(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_bool(false);
    /* String contains */
    if (args[0].type == VAL_STRING && args[1].type == VAL_STRING) {
        return make_bool(strstr(args[0].string_val, args[1].string_val) != NULL);
    }
    /* Array contains */
    if (args[0].type == VAL_ARRAY) {
        Array* arr = args[0].array_val;
        for (size_t i = 0; i < arr->count; i++) {
            if (compare_values(&arr->items[i], &args[1]) == 0) {
                return make_bool(true);
            }
        }
    }
    return make_bool(false);
}

Value stdlib_op_count_of(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_int(0);
    Array* arr = args[0].array_val;
    long long count = 0;
    for (size_t i = 0; i < arr->count; i++) {
        if (compare_values(&arr->items[i], &args[1]) == 0) count++;
    }
    return make_int(count);
}

Value stdlib_op_index_of(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) return make_int(-1);
    Array* arr = args[0].array_val;
    for (size_t i = 0; i < arr->count; i++) {
        if (compare_values(&arr->items[i], &args[1]) == 0) {
            return make_int((long long)i);
        }
    }
    return make_int(-1);
}

/* ── Type checks ──────────────────────────────────────────── */

Value stdlib_op_is_number(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    return make_bool(args[0].type == VAL_INT || args[0].type == VAL_FLOAT);
}

Value stdlib_op_is_string(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    return make_bool(args[0].type == VAL_STRING);
}

Value stdlib_op_is_array(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    return make_bool(args[0].type == VAL_ARRAY);
}

Value stdlib_op_is_bool(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    return make_bool(args[0].type == VAL_BOOL);
}

Value stdlib_op_is_null(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    return make_bool(args[0].type == VAL_NULL);
}

Value stdlib_op_is_function(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    return make_bool(args[0].type == VAL_FUNCTION);
}

/* ── Utility operators ────────────────────────────────────── */

Value stdlib_op_item(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    /* Array indexing */
    if (args[0].type == VAL_ARRAY && args[1].type == VAL_INT) {
        Array* arr = args[0].array_val;
        long long idx = args[1].int_val;
        if (idx < 0) idx += (long long)arr->count;
        if (idx >= 0 && idx < (long long)arr->count) {
            return arr->items[(size_t)idx];
        }
    }
    /* String indexing */
    if (args[0].type == VAL_STRING && args[1].type == VAL_INT) {
        size_t len = strlen(args[0].string_val);
        long long idx = args[1].int_val;
        if (idx < 0) idx += (long long)len;
        if (idx >= 0 && idx < (long long)len) {
            char buf[2] = { args[0].string_val[(size_t)idx], '\0' };
            return make_string(buf);
        }
    }
    return make_null();
}

Value stdlib_op_length(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_int(0);
    if (args[0].type == VAL_STRING) return make_int((long long)strlen(args[0].string_val));
    if (args[0].type == VAL_ARRAY) return make_int((long long)args[0].array_val->count);
    return make_int(0);
}

Value stdlib_op_min(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    return compare_values(&args[0], &args[1]) <= 0 ? args[0] : args[1];
}

Value stdlib_op_max(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    return compare_values(&args[0], &args[1]) >= 0 ? args[0] : args[1];
}

/* ── Registration ─────────────────────────────────────────── */

void stdlib_operator_register(void) {
    /* Comparison */
    register_native("operator.eq", stdlib_op_eq);
    register_native("operator.ne", stdlib_op_ne);
    register_native("operator.lt", stdlib_op_lt);
    register_native("operator.le", stdlib_op_le);
    register_native("operator.gt", stdlib_op_gt);
    register_native("operator.ge", stdlib_op_ge);
    /* Arithmetic */
    register_native("operator.add", stdlib_op_add);
    register_native("operator.sub", stdlib_op_sub);
    register_native("operator.mul", stdlib_op_mul);
    register_native("operator.truediv", stdlib_op_truediv);
    register_native("operator.floordiv", stdlib_op_floordiv);
    register_native("operator.mod", stdlib_op_mod);
    register_native("operator.neg", stdlib_op_neg);
    register_native("operator.pos", stdlib_op_pos);
    register_native("operator.abs", stdlib_op_abs_op);
    /* Bitwise */
    register_native("operator.band", stdlib_op_band);
    register_native("operator.bor", stdlib_op_bor);
    register_native("operator.bxor", stdlib_op_bxor);
    register_native("operator.bnot", stdlib_op_bnot);
    register_native("operator.shl", stdlib_op_shl);
    register_native("operator.shr", stdlib_op_shr);
    /* Logical */
    register_native("operator.and", stdlib_op_and);
    register_native("operator.or", stdlib_op_or);
    register_native("operator.not", stdlib_op_not);
    /* Sequence */
    register_native("operator.concat", stdlib_op_concat);
    register_native("operator.contains", stdlib_op_contains);
    register_native("operator.count_of", stdlib_op_count_of);
    register_native("operator.index_of", stdlib_op_index_of);
    /* Type checks */
    register_native("operator.is_number", stdlib_op_is_number);
    register_native("operator.is_string", stdlib_op_is_string);
    register_native("operator.is_array", stdlib_op_is_array);
    register_native("operator.is_bool", stdlib_op_is_bool);
    register_native("operator.is_null", stdlib_op_is_null);
    register_native("operator.is_function", stdlib_op_is_function);
    /* Utility */
    register_native("operator.item", stdlib_op_item);
    register_native("operator.length", stdlib_op_length);
    register_native("operator.min", stdlib_op_min);
    register_native("operator.max", stdlib_op_max);
}
