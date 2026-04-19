#define _POSIX_C_SOURCE 200809L
#include "stdlib_fs.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Unified Filesystem Module ──
 * Consolidates former stdlib_fs and stdlib_filesystem into one module.
 * Provides complete file/directory operations, path queries, and metadata.
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

static bool check_args(int argc, int expected, ValueType type0, const Value* args) {
    if (argc < expected) return false;
    if (type0 != VAL_NULL && args[0].type != type0) return false;
    return true;
}

/* ── Directory listing (returns array of strings) ── */
Value native_fs_readdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_null();
    DIR* d = opendir(args[0].string_val);
    if (!d) {
        fprintf(stderr, "Error: fs.readdir() cannot open '%s': %s\n",
                args[0].string_val, strerror(errno));
        return make_null();
    }
    Array* arr = array_create(32);
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        array_push(arr, make_string(entry->d_name));
    }
    closedir(d);
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── Stat: returns [size, is_dir, is_file, mode, mtime] ── */
Value native_fs_stat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_null();
    struct stat st;
    if (stat(args[0].string_val, &st) != 0) {
        fprintf(stderr, "Error: fs.stat() cannot stat '%s': %s\n",
                args[0].string_val, strerror(errno));
        return make_null();
    }
    Array* arr = array_create(5);
    array_push(arr, make_int(st.st_size));
    array_push(arr, make_bool(S_ISDIR(st.st_mode) != 0));
    array_push(arr, make_bool(S_ISREG(st.st_mode) != 0));
    array_push(arr, make_int(st.st_mode & 0777));
    array_push(arr, make_int(st.st_mtime));
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── File operations ── */
Value native_fs_exists(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    return make_bool(access(args[0].string_val, F_OK) == 0);
}

Value native_fs_is_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    struct stat st;
    return make_bool(stat(args[0].string_val, &st) == 0 && S_ISREG(st.st_mode));
}

Value native_fs_is_dir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    struct stat st;
    return make_bool(stat(args[0].string_val, &st) == 0 && S_ISDIR(st.st_mode));
}

Value native_fs_stat_size(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_int(-1);
    struct stat st;
    if (stat(args[0].string_val, &st) != 0) return make_int(-1);
    return make_int((long long)st.st_size);
}

Value native_fs_file_modified(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_int(-1);
    struct stat st;
    if (stat(args[0].string_val, &st) != 0) return make_int(-1);
    return make_int((long long)st.st_mtime);
}

Value native_fs_remove(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    return make_bool(remove(args[0].string_val) == 0);
}

Value native_fs_unlink(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    return make_bool(unlink(args[0].string_val) == 0);
}

/* ── Directory operations ── */
Value native_fs_mkdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    int res = mkdir(args[0].string_val, 0775);
    if (res != 0 && errno == EEXIST) res = 0;
    return make_bool(res == 0);
}

Value native_fs_rmdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    return make_bool(rmdir(args[0].string_val) == 0);
}

Value native_fs_getcwd(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    char buf[4096];
    if (!getcwd(buf, sizeof(buf))) return make_null();
    return make_string(buf);
}

Value native_fs_chdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_bool(false);
    return make_bool(chdir(args[0].string_val) == 0);
}

Value native_fs_chmod(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) return make_bool(false);
    return make_bool(chmod(args[0].string_val, (mode_t)args[1].int_val) == 0);
}

/* ── Rename / copy / move ── */
Value native_fs_rename(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_bool(false);
    return make_bool(rename(args[0].string_val, args[1].string_val) == 0);
}

static bool copy_file_impl(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    if (!in) return false;
    FILE* out = fopen(dst, "wb");
    if (!out) { fclose(in); return false; }
    char buf[8192];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { ok = false; break; }
    }
    fclose(in); fclose(out);
    return ok;
}

Value native_fs_copy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_bool(false);
    return make_bool(copy_file_impl(args[0].string_val, args[1].string_val));
}

Value native_fs_move(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_bool(false);
    if (rename(args[0].string_val, args[1].string_val) == 0) return make_bool(true);
    bool ok = copy_file_impl(args[0].string_val, args[1].string_val);
    if (ok) remove(args[0].string_val);
    return make_bool(ok);
}

/* ── Temp file ── */
Value native_fs_temp_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    char tmpl[] = "/tmp/rads_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return make_null();
    close(fd);
    return make_string(tmpl);
}

/* ── Temp directory ── */
Value native_fs_temp_dir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    char tmpl[] = "/tmp/rads_dir_XXXXXX";
    if (!mkdtemp(tmpl)) return make_null();
    return make_string(tmpl);
}

/* ── Read/write convenience ── */
Value native_fs_read_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_args(argc, 1, VAL_STRING, args)) return make_null();
    FILE* f = fopen(args[0].string_val, "rb");
    if (!f) {
        fprintf(stderr, "Error: fs.read_file() cannot open '%s': %s\n",
                args[0].string_val, strerror(errno));
        return make_null();
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return make_null(); }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);
    Value v; v.type = VAL_STRING; v.string_val = buf; return v;
}

Value native_fs_write_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_bool(false);
    FILE* f = fopen(args[0].string_val, "wb");
    if (!f) return make_bool(false);
    size_t len = strlen(args[1].string_val);
    bool ok = fwrite(args[1].string_val, 1, len, f) == len;
    fclose(f);
    return make_bool(ok);
}

Value native_fs_append_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_bool(false);
    FILE* f = fopen(args[0].string_val, "ab");
    if (!f) return make_bool(false);
    size_t len = strlen(args[1].string_val);
    bool ok = fwrite(args[1].string_val, 1, len, f) == len;
    fclose(f);
    return make_bool(ok);
}

void stdlib_fs_register(void) {
    /* Directory */
    register_native("fs.readdir", native_fs_readdir);
    register_native("fs.mkdir", native_fs_mkdir);
    register_native("fs.rmdir", native_fs_rmdir);
    register_native("fs.getcwd", native_fs_getcwd);
    register_native("fs.chdir", native_fs_chdir);

    /* File queries */
    register_native("fs.exists", native_fs_exists);
    register_native("fs.is_file", native_fs_is_file);
    register_native("fs.is_dir", native_fs_is_dir);
    register_native("fs.stat", native_fs_stat);
    register_native("fs.stat_size", native_fs_stat_size);
    register_native("fs.file_modified", native_fs_file_modified);

    /* File operations */
    register_native("fs.remove", native_fs_remove);
    register_native("fs.unlink", native_fs_unlink);
    register_native("fs.rename", native_fs_rename);
    register_native("fs.copy", native_fs_copy);
    register_native("fs.move", native_fs_move);
    register_native("fs.chmod", native_fs_chmod);

    /* Read/write */
    register_native("fs.read_file", native_fs_read_file);
    register_native("fs.write_file", native_fs_write_file);
    register_native("fs.append_file", native_fs_append_file);

    /* Temp */
    register_native("fs.temp_file", native_fs_temp_file);
    register_native("fs.temp_dir", native_fs_temp_dir);
}
