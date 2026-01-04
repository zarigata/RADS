#ifndef RADS_STDLIB_FFI_H
#define RADS_STDLIB_FFI_H

#include "interpreter.h"

// FFI Module registration
void stdlib_ffi_register(void);

// Native functions
Value native_ffi_load(struct Interpreter* interp, int argc, Value* args);
Value native_ffi_call(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_FFI_H
