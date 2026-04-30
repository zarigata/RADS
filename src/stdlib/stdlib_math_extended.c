/*
 * stdlib_math_extended.c — Extended math functions for RADS
 *
 * Supplements stdlib_math.c with additional math utilities.
 * NOTE: min, max, clamp, floor, ceil, round, random are already
 * registered in stdlib_math.c. This module only adds srand().
 */

#include "../core/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern Value make_int(long long val);

static bool random_initialized = false;

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
    register_native("math.srand", (NativeFn)stdlib_math_srand);
}
