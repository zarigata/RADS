#define _POSIX_C_SOURCE 200809L
#include "stdlib_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to check argument count
static bool check_argc(int argc, int expected) {
    return argc == expected;
}

Value native_io_read_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; // Unused
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        // Return null on error for now
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    FILE* file = fopen(args[0].string_val, "rb");
    if (!file) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    
    Value v;
    v.type = VAL_STRING;
    v.string_val = buffer; // Ownership transferred to Value
    return v;
}

Value native_io_write_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    FILE* file = fopen(args[0].string_val, "wb");
    if (!file) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    fwrite(args[1].string_val, 1, strlen(args[1].string_val), file);
    fclose(file);
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = true;
    return v;
}

Value native_io_file_exists(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    FILE* file = fopen(args[0].string_val, "r");
    bool exists = (file != NULL);
    if (file) fclose(file);
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = exists;
    return v;
}

// Registration
void stdlib_io_register(void) {
    register_native("io.read_file", native_io_read_file);
    register_native("io.write_file", native_io_write_file);
    register_native("io.file_exists", native_io_file_exists);
}
