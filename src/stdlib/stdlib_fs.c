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
#include <unistd.h>

static bool check_argc(int argc, int expected) {
    return argc == expected;
}

static Value make_bool_val(bool b) {
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = b;
    return v;
}

static Value make_int_val(long long i) {
    Value v;
    v.type = VAL_INT;
    v.int_val = i;
    return v;
}

static Value make_null_val(void) {
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value native_fs_list_dir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_null_val();
    }

    DIR* d = opendir(args[0].string_val);
    if (!d) return make_null_val();

    size_t cap = 256;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) { closedir(d); return make_null_val(); }
    buf[0] = '\0';

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        const char* name = entry->d_name;
        // skip . and ..
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        size_t nlen = strlen(name);
        if (len + nlen + 2 > cap) {
            cap *= 2;
            char* nb = realloc(buf, cap);
            if (!nb) { free(buf); closedir(d); return make_null_val(); }
            buf = nb;
        }
        memcpy(buf + len, name, nlen);
        len += nlen;
        buf[len++] = '\n';
        buf[len] = '\0';
    }
    closedir(d);

    Value v;
    v.type = VAL_STRING;
    v.string_val = buf;
    return v;
}

Value native_fs_mkdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_bool_val(false);
    }
    int res = mkdir(args[0].string_val, 0775);
    if (res != 0 && errno == EEXIST) res = 0;
    return make_bool_val(res == 0);
}

Value native_fs_rmdir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_bool_val(false);
    }
    int res = rmdir(args[0].string_val);
    return make_bool_val(res == 0);
}

Value native_fs_stat_size(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_int_val(-1);
    }
    struct stat st;
    if (stat(args[0].string_val, &st) != 0) {
        return make_int_val(-1);
    }
    return make_int_val((long long)st.st_size);
}

Value native_fs_is_dir(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_bool_val(false);
    }
    struct stat st;
    if (stat(args[0].string_val, &st) != 0) {
        return make_bool_val(false);
    }
    return make_bool_val(S_ISDIR(st.st_mode));
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
    fclose(in);
    fclose(out);
    return ok;
}

Value native_fs_copy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_bool_val(false);
    }
    bool ok = copy_file_impl(args[0].string_val, args[1].string_val);
    return make_bool_val(ok);
}

Value native_fs_move(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return make_bool_val(false);
    }
    if (rename(args[0].string_val, args[1].string_val) == 0) {
        return make_bool_val(true);
    }
    // fallback copy+remove
    bool ok = copy_file_impl(args[0].string_val, args[1].string_val);
    if (ok) remove(args[0].string_val);
    return make_bool_val(ok);
}

Value native_fs_exists(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_bool_val(false);
    }
    return make_bool_val(access(args[0].string_val, F_OK) == 0);
}

Value native_fs_is_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_bool_val(false);
    }
    struct stat st;
    if (stat(args[0].string_val, &st) != 0) {
        return make_bool_val(false);
    }
    return make_bool_val(S_ISREG(st.st_mode));
}

Value native_fs_remove(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        return make_bool_val(false);
    }
    return make_bool_val(remove(args[0].string_val) == 0);
}

Value native_fs_getcwd(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;

    char buf[4096];
    if (getcwd(buf, sizeof(buf)) == NULL) {
        return make_null_val();
    }

    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup(buf);
    return v;
}

void stdlib_fs_register(void) {
    register_native("fs.list_dir", native_fs_list_dir);
    register_native("fs.mkdir", native_fs_mkdir);
    register_native("fs.rmdir", native_fs_rmdir);
    register_native("fs.stat_size", native_fs_stat_size);
    register_native("fs.is_dir", native_fs_is_dir);
    register_native("fs.copy", native_fs_copy);
    register_native("fs.move", native_fs_move);
    register_native("fs.exists", native_fs_exists);
    register_native("fs.is_file", native_fs_is_file);
    register_native("fs.remove", native_fs_remove);
    register_native("fs.getcwd", native_fs_getcwd);
}
