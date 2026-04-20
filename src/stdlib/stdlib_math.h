#ifndef RADS_STDLIB_MATH_H
#define RADS_STDLIB_MATH_H

#include "../core/interpreter.h"

// Math Module registration
void stdlib_math_register(void);

// Native functions
Value native_math_abs(struct Interpreter* interp, int argc, Value* args);
Value native_math_sqrt(struct Interpreter* interp, int argc, Value* args);
Value native_math_cbrt(struct Interpreter* interp, int argc, Value* args);
Value native_math_pow(struct Interpreter* interp, int argc, Value* args);
Value native_math_sin(struct Interpreter* interp, int argc, Value* args);
Value native_math_cos(struct Interpreter* interp, int argc, Value* args);
Value native_math_tan(struct Interpreter* interp, int argc, Value* args);
Value native_math_asin(struct Interpreter* interp, int argc, Value* args);
Value native_math_acos(struct Interpreter* interp, int argc, Value* args);
Value native_math_atan(struct Interpreter* interp, int argc, Value* args);
Value native_math_sinh(struct Interpreter* interp, int argc, Value* args);
Value native_math_cosh(struct Interpreter* interp, int argc, Value* args);
Value native_math_tanh(struct Interpreter* interp, int argc, Value* args);
Value native_math_log(struct Interpreter* interp, int argc, Value* args);
Value native_math_log2(struct Interpreter* interp, int argc, Value* args);
Value native_math_log10(struct Interpreter* interp, int argc, Value* args);
Value native_math_exp(struct Interpreter* interp, int argc, Value* args);
Value native_math_round(struct Interpreter* interp, int argc, Value* args);
Value native_math_floor(struct Interpreter* interp, int argc, Value* args);
Value native_math_ceil(struct Interpreter* interp, int argc, Value* args);
Value native_math_trunc(struct Interpreter* interp, int argc, Value* args);
Value native_math_sign(struct Interpreter* interp, int argc, Value* args);
Value native_math_min(struct Interpreter* interp, int argc, Value* args);
Value native_math_max(struct Interpreter* interp, int argc, Value* args);
Value native_math_clamp(struct Interpreter* interp, int argc, Value* args);
Value native_math_lerp(struct Interpreter* interp, int argc, Value* args);
Value native_math_atan2(struct Interpreter* interp, int argc, Value* args);
Value native_math_hypot(struct Interpreter* interp, int argc, Value* args);
Value native_math_fmod(struct Interpreter* interp, int argc, Value* args);
Value native_math_random(struct Interpreter* interp, int argc, Value* args);
Value native_math_random_int(struct Interpreter* interp, int argc, Value* args);
Value native_math_pi(struct Interpreter* interp, int argc, Value* args);
Value native_math_e(struct Interpreter* interp, int argc, Value* args);
Value native_math_nan(struct Interpreter* interp, int argc, Value* args);
Value native_math_inf(struct Interpreter* interp, int argc, Value* args);
Value native_math_isnan(struct Interpreter* interp, int argc, Value* args);
Value native_math_isinf(struct Interpreter* interp, int argc, Value* args);
Value native_math_isfinite(struct Interpreter* interp, int argc, Value* args);
Value native_math_sum(struct Interpreter* interp, int argc, Value* args);
Value native_math_avg(struct Interpreter* interp, int argc, Value* args);
Value native_math_median(struct Interpreter* interp, int argc, Value* args);
Value native_math_deg_to_rad(struct Interpreter* interp, int argc, Value* args);
Value native_math_rad_to_deg(struct Interpreter* interp, int argc, Value* args);

#endif