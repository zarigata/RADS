#include "../core/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// ============================================================================
// FILESYSTEM OPERATIONS - readdir, stat, unlink, path manipulation
// RADS v0.0.7 "DARK MOON"
// ============================================================================

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/**
 * fs.readdir(path) - Read directory contents
 * Returns an array of filenames in the directory
 */
Value stdlib_fs_readdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: fs.readdir() requires 1 argument (directory path)\n");
        return make_null();
    }

    Value path_val = args[0];
    if (path_val.type != VAL_STRING) {
        fprintf(stderr, "Error: fs.readdir() argument must be a string path\n");
        return make_null();
    }

    const char* path = path_val.string_val;
    DIR* dir = opendir(path);
    
    if (dir == NULL) {
        fprintf(stderr, "Error: fs.readdir() cannot open directory '%s': %s\n", 
                path, strerror(errno));
        return make_null();
    }

    Array* result_arr = array_create(32);
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        Value filename = make_string(entry->d_name);
        array_push(result_arr, filename);
    }

    closedir(dir);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

/**
 * fs.stat(path) - Get file status
 * Returns [size, is_dir, is_file, mode, mtime]
 */
Value stdlib_fs_stat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: fs.stat() requires 1 argument (file path)\n");
        return make_null();
    }

    Value path_val = args[0];
    if (path_val.type != VAL_STRING) {
        fprintf(stderr, "Error: fs.stat() argument must be a string path\n");
        return make_null();
    }

    const char* path = path_val.string_val;
    struct stat file_stat;
    
    if (stat(path, &file_stat) != 0) {
        fprintf(stderr, "Error: fs.stat() cannot stat '%s': %s\n", 
                path, strerror(errno));
        return make_null();
    }

    Array* result_arr = array_create(5);
    array_push(result_arr, make_int(file_stat.st_size));
    array_push(result_arr, make_bool(S_ISDIR(file_stat.st_mode) != 0));
    array_push(result_arr, make_bool(S_ISREG(file_stat.st_mode) != 0));
    array_push(result_arr, make_int(file_stat.st_mode & 0777));
    array_push(result_arr, make_int(file_stat.st_mtime));

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

/**
 * fs.unlink(path) - Delete a file
 * Returns true on success, false on failure
 */
Value stdlib_fs_unlink(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: fs.unlink() requires 1 argument (file path)\n");
        return make_bool(false);
    }

    Value path_val = args[0];
    if (path_val.type != VAL_STRING) {
        fprintf(stderr, "Error: fs.unlink() argument must be a string path\n");
        return make_bool(false);
    }

    if (unlink(path_val.string_val) == 0) {
        return make_bool(true);
    }
    fprintf(stderr, "Error: fs.unlink() cannot delete '%s': %s\n", 
            path_val.string_val, strerror(errno));
    return make_bool(false);
}

/**
 * fs.exists(path) - Check if file/directory exists
 */
Value stdlib_fs_exists(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        return make_bool(false);
    }

    struct stat file_stat;
    return make_bool(stat(args[0].string_val, &file_stat) == 0);
}

/**
 * fs.mkdir(path) - Create a directory
 */
Value stdlib_fs_mkdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        return make_bool(false);
    }

    return make_bool(mkdir(args[0].string_val, 0755) == 0);
}

/**
 * fs.rmdir(path) - Remove an empty directory
 */
Value stdlib_fs_rmdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        return make_bool(false);
    }

    return make_bool(rmdir(args[0].string_val) == 0);
}

/**
 * fs.rename(old_path, new_path) - Rename/move a file or directory
 */
Value stdlib_fs_rename(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_bool(false);
    }

    return make_bool(rename(args[0].string_val, args[1].string_val) == 0);
}

/**
 * fs.getcwd() - Get current working directory
 */
Value stdlib_fs_getcwd(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        return make_string(buffer);
    }
    return make_null();
}

/**
 * fs.chmod(path, mode) - Change file permissions
 */
Value stdlib_fs_chmod(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        return make_bool(false);
    }

    return make_bool(chmod(args[0].string_val, (mode_t)args[1].int_val) == 0);
}

void stdlib_filesystem_register(void) {
    register_native("fs.readdir", (NativeFn)stdlib_fs_readdir);
    register_native("fs.stat", (NativeFn)stdlib_fs_stat);
    register_native("fs.unlink", (NativeFn)stdlib_fs_unlink);
    register_native("fs.exists", (NativeFn)stdlib_fs_exists);
    register_native("fs.mkdir", (NativeFn)stdlib_fs_mkdir);
    register_native("fs.rmdir", (NativeFn)stdlib_fs_rmdir);
    register_native("fs.rename", (NativeFn)stdlib_fs_rename);
    register_native("fs.getcwd", (NativeFn)stdlib_fs_getcwd);
    register_native("fs.chmod", (NativeFn)stdlib_fs_chmod);
}
