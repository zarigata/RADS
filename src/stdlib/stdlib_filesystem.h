#ifndef RADS_STDLIB_FILESYSTEM_H
#define RADS_STDLIB_FILESYSTEM_H

#include "../core/interpreter.h"

// Filesystem Module registration
void stdlib_filesystem_register(void);

// Native functions
Value native_fs_read_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_write_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_append_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_file_exists(struct Interpreter* interp, int argc, Value* args);
Value native_fs_file_size(struct Interpreter* interp, int argc, Value* args);
Value native_fs_file_modified(struct Interpreter* interp, int argc, Value* args);
Value native_fs_directory_exists(struct Interpreter* interp, int argc, Value* args);
Value native_fs_create_directory(struct Interpreter* interp, int argc, Value* args);
Value native_fs_list_directory(struct Interpreter* interp, int argc, Value* args);
Value native_fs_remove_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_rename_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_current_directory(struct Interpreter* interp, int argc, Value* args);
Value native_fs_change_directory(struct Interpreter* interp, int argc, Value* args);
Value native_fs_environment_variable(struct Interpreter* interp, int argc, Value* args);
Value native_fs_copy_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_move_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_temp_file(struct Interpreter* interp, int argc, Value* args);

#endif