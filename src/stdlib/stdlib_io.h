#ifndef RADS_STDLIB_IO_H
#define RADS_STDLIB_IO_H

#include "../core/interpreter.h"

// IO Module registration
void stdlib_io_register(void);

// Native functions
Value native_io_read_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_write_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_file_exists(struct Interpreter* interp, int argc, Value* args);
Value native_io_file_size(struct Interpreter* interp, int argc, Value* args);
Value native_io_file_modified(struct Interpreter* interp, int argc, Value* args);
Value native_io_directory_exists(struct Interpreter* interp, int argc, Value* args);
Value native_io_create_directory(struct Interpreter* interp, int argc, Value* args);
Value native_io_list_directory(struct Interpreter* interp, int argc, Value* args);
Value native_io_remove_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_rename_file(struct Interpreter* interp, int argc, Value* args);
Value native_io_current_directory(struct Interpreter* interp, int argc, Value* args);
Value native_io_change_directory(struct Interpreter* interp, int argc, Value* args);
Value native_io_environment_variable(struct Interpreter* interp, int argc, Value* args);

#endif