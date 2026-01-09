#ifndef RADS_STDLIB_FS_H
#define RADS_STDLIB_FS_H

#include "interpreter.h"

void stdlib_fs_register(void);

// fs helpers
Value native_fs_list_dir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_mkdir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_rmdir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_stat_size(struct Interpreter* interp, int argc, Value* args);
Value native_fs_copy(struct Interpreter* interp, int argc, Value* args);
Value native_fs_move(struct Interpreter* interp, int argc, Value* args);
Value native_fs_is_dir(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_FS_H
