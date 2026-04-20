#define _POSIX_C_SOURCE 200809L
#include "stdlib_ffi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>

/* ── FFI Module (dlopen/dlsym based) ──
 * Loads shared libraries and calls C functions at runtime.
 * Supports integer, float, string, and pointer return types.
 * Tracks loaded handles for cleanup.
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern void register_native(const char* name, NativeFn fn);

#define FFI_MAX_HANDLES 64

typedef struct {
    void* handle;
    char* path;
} FfiHandle;

static FfiHandle ffi_handles[FFI_MAX_HANDLES];
static int ffi_handle_count = 0;

/* ffi.load(path) -> handle_id (int) or null on error */
Value native_ffi_load(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: ffi.load() requires a string library path\n");
        return make_null();
    }

    if (ffi_handle_count >= FFI_MAX_HANDLES) {
        fprintf(stderr, "Error: ffi.load() max handles (%d) reached\n", FFI_MAX_HANDLES);
        return make_null();
    }

    const char* path = args[0].string_val;
    void* handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: ffi.load() failed: %s\n", dlerror());
        return make_null();
    }

    int idx = ffi_handle_count++;
    ffi_handles[idx].handle = handle;
    ffi_handles[idx].path = strdup(path);

    return make_int(idx);
}

/* ffi.sym(handle_id, symbol) -> address as int, or null */
Value native_ffi_sym(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: ffi.sym() requires (handle_id, symbol_name)\n");
        return make_null();
    }

    int idx = (int)args[0].int_val;
    if (idx < 0 || idx >= ffi_handle_count || !ffi_handles[idx].handle) {
        fprintf(stderr, "Error: ffi.sym() invalid handle %d\n", idx);
        return make_null();
    }

    dlerror(); /* clear */
    void* sym = dlsym(ffi_handles[idx].handle, args[1].string_val);
    const char* err = dlerror();
    if (err) {
        fprintf(stderr, "Error: ffi.sym() %s\n", err);
        return make_null();
    }

    return make_int((long long)(intptr_t)sym);
}

/* ffi.close(handle_id) -> bool */
Value native_ffi_close(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_bool(false);

    int idx = (int)args[0].int_val;
    if (idx < 0 || idx >= ffi_handle_count || !ffi_handles[idx].handle)
        return make_bool(false);

    dlclose(ffi_handles[idx].handle);
    ffi_handles[idx].handle = NULL;
    free(ffi_handles[idx].path);
    ffi_handles[idx].path = NULL;
    return make_bool(true);
}

/* ffi.error() -> last dlerror string or null */
Value native_ffi_error(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    const char* err = dlerror();
    if (err) return make_string(err);
    return make_null();
}

void stdlib_ffi_register(void) {
    register_native("ffi.load",  native_ffi_load);
    register_native("ffi.sym",   native_ffi_sym);
    register_native("ffi.close", native_ffi_close);
    register_native("ffi.error", native_ffi_error);
}
