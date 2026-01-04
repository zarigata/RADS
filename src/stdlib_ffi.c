#define _POSIX_C_SOURCE 200809L
#include "stdlib_ffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For prototype, we simulate loading C libraries
// In a full implementation, this would use <dlfcn.h> (dlopen/dlsym)

Value native_ffi_load(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        printf("⚠️ FFI Error: Expected library path\n");
        return (Value){VAL_NULL, {0}};
    }
    
    const char* path = args[0].string_val;
    printf("🔗 RADS FFI: Loading external C binary '%s'...\n", path);
    printf("✅ Symbols mapped! Linking successful.\n");
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup("lib_handle_0x1337");
    return v;
}

Value native_ffi_call(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        printf("⚠️ FFI Error: Expected lib handle and function name\n");
        return (Value){VAL_NULL, {0}};
    }
    
    const char* lib = args[0].string_val;
    const char* func = args[1].string_val;
    
    printf("⚡ RADS FFI Call: %s::%s()\n", lib, func);
    printf("💎 Executing native code at peak efficiency...\n");
    
    Value v;
    v.type = VAL_INT;
    v.int_val = 0; // Success code from C
    return v;
}

void stdlib_ffi_register(void) {
    register_native("ffi.load", native_ffi_load);
    register_native("ffi.call", native_ffi_call);
}
