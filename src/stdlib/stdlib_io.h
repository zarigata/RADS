#ifndef RADS_STDLIB_IO_H
#define RADS_STDLIB_IO_H

#include "interpreter.h"

// IO Module registration
void stdlib_io_register(void);

// Native functions
Value native_io_read_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_write_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_file_exists(struct Interpreter* interp, int argc, Value* args);
Value native_io_delete_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_append_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_file_size(struct Interpreter* interp, int argc, Value* args);
Value native_io_read_line(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_IO_H
