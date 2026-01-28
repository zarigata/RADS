#ifndef RADS_STDLIB_STRING_ADVANCED_H
#define RADS_STDLIB_STRING_ADVANCED_H

#include "../core/interpreter.h"

void stdlib_string_advanced_register(void);

Value stdlib_string_split(struct Interpreter* interp, int argc, Value* args);
Value stdlib_string_join(struct Interpreter* interp, int argc, Value* args);
Value stdlib_string_trim(struct Interpreter* interp, int argc, Value* args);
Value stdlib_string_upper(struct Interpreter* interp, int argc, Value* args);
Value stdlib_string_lower(struct Interpreter* interp, int argc, Value* args);
Value stdlib_string_replace(struct Interpreter* interp, int argc, Value* args);
Value stdlib_string_substring(struct Interpreter* interp, int argc, Value* args);

#endif
