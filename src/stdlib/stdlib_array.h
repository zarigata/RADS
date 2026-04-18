#ifndef RADS_STDLIB_ARRAY_H
#define RADS_STDLIB_ARRAY_H

#include "../core/interpreter.h"

void stdlib_array_register(void);

Value stdlib_array_map(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_filter(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_reduce(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_find(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_some(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_every(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_sort(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_reverse(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_includes(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_index_of(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_flat(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_slice(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_unique(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_count(struct Interpreter* interp, int argc, Value* args);
Value stdlib_array_concat(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_ARRAY_H
