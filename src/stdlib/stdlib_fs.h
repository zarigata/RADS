#ifndef RADS_STDLIB_FS_H
#define RADS_STDLIB_FS_H

#include "../core/interpreter.h"

void stdlib_fs_register(void);

/* Unified filesystem functions */
Value native_fs_readdir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_stat(struct Interpreter* interp, int argc, Value* args);
Value native_fs_exists(struct Interpreter* interp, int argc, Value* args);
Value native_fs_is_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_is_dir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_stat_size(struct Interpreter* interp, int argc, Value* args);
Value native_fs_file_modified(struct Interpreter* interp, int argc, Value* args);
Value native_fs_remove(struct Interpreter* interp, int argc, Value* args);
Value native_fs_unlink(struct Interpreter* interp, int argc, Value* args);
Value native_fs_mkdir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_rmdir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_getcwd(struct Interpreter* interp, int argc, Value* args);
Value native_fs_chdir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_chmod(struct Interpreter* interp, int argc, Value* args);
Value native_fs_rename(struct Interpreter* interp, int argc, Value* args);
Value native_fs_copy(struct Interpreter* interp, int argc, Value* args);
Value native_fs_move(struct Interpreter* interp, int argc, Value* args);
Value native_fs_temp_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_temp_dir(struct Interpreter* interp, int argc, Value* args);
Value native_fs_read_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_write_file(struct Interpreter* interp, int argc, Value* args);
Value native_fs_append_file(struct Interpreter* interp, int argc, Value* args);

#endif // RADS_STDLIB_FS_H
