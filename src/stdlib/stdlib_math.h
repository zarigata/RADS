#ifndef RADS_STDLIB_MATH_H
#define RADS_STDLIB_MATH_H

#include "../core/interpreter.h"

// Math Module registration
void stdlib_math_register(void);

// Native functions
Value native_math_abs(struct Interpreter* interp, int argc, Value* args);
Value native_math_sqrt(struct Interpreter* interp, int argc, Value* args);
Value native_math_pow(struct Interpreter* interp, int argc, Value* args);
Value native_math_sin(struct Interpreter* interp, int argc, Value* args);
Value native_math_cos(struct Interpreter* interp, int argc, Value* args);
Value native_math_tan(struct Interpreter* interp, int argc, Value* args);
Value native_math_log(struct Interpreter* interp, int argc, Value* args);
Value native_math_exp(struct Interpreter* interp, int argc, Value* args);
Value native_math_round(struct Interpreter* interp, int argc, Value* args);
Value native_math_floor(struct Interpreter* interp, int argc, Value* args);
Value native_math_ceil(struct Interpreter* interp, int argc, Value* args);
Value native_math_trunc(struct Interpreter* interp, int argc, Value* args);
Value native_math_sign(struct Interpreter* interp, int argc, Value* args);
Value native_math_min(struct Interpreter* interp, int argc, Value* args);
Value native_math_max(struct Interpreter* interp, int argc, Value* args);
Value native_math_clamp(struct Interpreter* interp, int argc, Value* args);
Value native_math_random(struct Interpreter* interp, int argc, Value* args);
Value native_math_random_int(struct Interpreter* interp, int argc, Value* args);
Value native_math_pi(struct Interpreter* interp, int argc, Value* args);
Value native_math_e(struct Interpreter* interp, int argc, Value* args);

#endif