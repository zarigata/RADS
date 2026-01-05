#ifndef RADS_STDLIB_JSON_H
#define RADS_STDLIB_JSON_H

#include "interpreter.h"

void stdlib_json_register(void);

// Helpers
Value native_json_get_string(struct Interpreter* interp, int argc, Value* args);
Value native_json_get_number(struct Interpreter* interp, int argc, Value* args);
Value native_json_get_bool(struct Interpreter* interp, int argc, Value* args);
Value native_json_stringify_kv(struct Interpreter* interp, int argc, Value* args);
Value native_json_escape(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_JSON_H
