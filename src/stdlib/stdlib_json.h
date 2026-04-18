#ifndef RADS_STDLIB_JSON_H
#define RADS_STDLIB_JSON_H

#include "../core/interpreter.h"

void stdlib_json_register(void);

Value native_json_parse(struct Interpreter* interp, int argc, Value* args);
Value native_json_stringify(struct Interpreter* interp, int argc, Value* args);
Value native_json_stringify_pretty(struct Interpreter* interp, int argc, Value* args);
Value native_json_get(struct Interpreter* interp, int argc, Value* args);
Value native_json_keys(struct Interpreter* interp, int argc, Value* args);
Value native_json_values(struct Interpreter* interp, int argc, Value* args);
Value native_json_has_key(struct Interpreter* interp, int argc, Value* args);
Value native_json_type_of(struct Interpreter* interp, int argc, Value* args);

// Legacy helpers
Value native_json_get_string(struct Interpreter* interp, int argc, Value* args);
Value native_json_get_number(struct Interpreter* interp, int argc, Value* args);
Value native_json_get_bool(struct Interpreter* interp, int argc, Value* args);
Value native_json_stringify_kv(struct Interpreter* interp, int argc, Value* args);
Value native_json_escape(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_JSON_H
