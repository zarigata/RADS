#ifndef STDLIB_STRUCT_H
#define STDLIB_STRUCT_H

#include "../core/interpreter.h"

Value stdlib_struct_pack(Interpreter* interp, int argc, Value* args);
Value stdlib_struct_unpack(Interpreter* interp, int argc, Value* args);
Value stdlib_struct_calcsize(Interpreter* interp, int argc, Value* args);
Value stdlib_struct_pack_into(Interpreter* interp, int argc, Value* args);
Value stdlib_struct_unpack_from(Interpreter* interp, int argc, Value* args);
Value stdlib_struct_iter_unpack(Interpreter* interp, int argc, Value* args);
void stdlib_struct_register(void);

#endif
