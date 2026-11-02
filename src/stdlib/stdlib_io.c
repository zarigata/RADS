#define _POSIX_C_SOURCE 200809L
#include "stdlib_io.h"
#include <stdio.h>
extern void value_print(Value* value);
extern void interpreter_raise(Value v);
#include <stdlib.h>
#include <string.h>

// Helper to check argument count
static bool check_argc(int argc, int expected) {
    return argc == expected;
}

/* capture_value_render — render a Value exactly like value_print into a
 * malloc'd NUL-terminated buffer, using open_memstream (POSIX 2008 — the
 * file already defines _POSIX_C_SOURCE 200809L). Used by str() so both
 * conversions can never drift apart. Returns NULL on failure; caller owns. */
static char* capture_value_render(Value* v) {
    char* buf = NULL;
    size_t len = 0;
    FILE* ms = open_memstream(&buf, &len);
    if (!ms) return NULL;
    /* value_print writes to stdout — briefly re-point stdout at the
     * memstream so rendering is EXACTLY the echo/print rendering. */
    fflush(stdout);
    FILE* saved = stdout;
    stdout = ms;
    value_print(v);
    fflush(stdout);
    stdout = saved;
    fclose(ms);
    return buf;
}

/* chr(n) / ord(s) — Python-parity character conversions (nightshift 20261101).
 * Why: chr(10) for newline in string building (test_memory_truth uses it);
 * ord() is its inverse. chr accepts 0..255 (byte semantics — RADS strings
 * are byte-based C strings), throws outside that range like Python's
 * ValueError. ord takes the first byte of a string, throws on empty. */
Value native_io_chr(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) {
        interpreter_raise(make_string("FAIL: chr() requires an int argument"));
        return make_null();
    }
    long long code = args[0].int_val;
    if (code < 0 || code > 255) {
        char msg[64];
        snprintf(msg, sizeof(msg), "chr() argument out of range: %lld", code);
        Value err = make_string(msg); interpreter_raise(err); value_free(&err);
        return make_null();
    }
    char* out = malloc(2);
    out[0] = (char)code; out[1] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = out;
    return v;
}

Value native_io_ord(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING || !args[0].string_val || !args[0].string_val[0]) {
        interpreter_raise(make_string("ord() requires a non-empty string"));
        return make_null();
    }
    return make_int((long long)(unsigned char)args[0].string_val[0]);
}

/* print(...) — global console output (nightshift 20261101).
 * Why: JS/TS parity — echo is a STATEMENT in RADS, but test files written
 * JS-style call print(...) as a function; before honest-failure those calls
 * were silent no-ops. Renders each arg exactly like the echo statement
 * (value_print) separated by spaces, newline at end. Returns null. */
Value native_io_print(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    for (int i = 0; i < argc; i++) {
        value_print(&args[i]);
        if (i + 1 < argc) printf(" ");
    }
    printf("\n");
    return make_null();
}

/* str(value) — global to-string conversion (nightshift 20261101).
 * Why: Python parity — str(42) == "42". Stringifies any value the same way
 * concatenation does. Strings pass through; other types render via their
 * value_print forms. */
Value native_io_str(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("");
    if (args[0].type == VAL_STRING) return make_string(args[0].string_val);
    /* Render non-strings by capturing value_print output into a pipe. */
    char* buf = capture_value_render(&args[0]);
    Value v = make_string(buf ? buf : "");
    free(buf);
    return v;
}

/* type(value) — global type-name native (nightshift 20261101).
 * Why: Python/JSDoc parity — returns "null"/"bool"/"int"/"float"/
 * "string"/"function"/"array"/"struct"/"iterator", matching
 * typeof-style names already used across the test suite corpus. */
Value native_io_type(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("null");
    switch (args[0].type) {
        case VAL_NULL:            return make_string("null");
        case VAL_BOOL:            return make_string("bool");
        case VAL_INT:             return make_string("int");
        case VAL_FLOAT:           return make_string("float");
        case VAL_STRING:          return make_string("string");
        case VAL_FUNCTION:        return make_string("function");
        case VAL_ARRAY:           return make_string("array");
        case VAL_STRUCT_INSTANCE: return make_string("struct");
        case VAL_ITERATOR:        return make_string("iterator");
        case VAL_STRUCT_DEF:      return make_string("struct");
    }
    return make_string("unknown");
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

Value native_io_delete_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = (remove(args[0].string_val) == 0);
    return v;
}

Value native_io_append_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 2) || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        Value v;
        v.type = VAL_BOOL;
        v.bool_val = false;
        return v;
    }
    
    FILE* file = fopen(args[0].string_val, "ab");
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

Value native_io_file_size(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (!check_argc(argc, 1) || args[0].type != VAL_STRING) {
        Value v;
        v.type = VAL_INT;
        v.int_val = -1;
        return v;
    }
    
    FILE* file = fopen(args[0].string_val, "rb");
    if (!file) {
        Value v;
        v.type = VAL_INT;
        v.int_val = -1;
        return v;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    Value v;
    v.type = VAL_INT;
    v.int_val = size;
    return v;
}

Value native_io_read_line(struct Interpreter* __attribute__((unused)) interp, int __attribute__((unused)) argc, Value* __attribute__((unused)) args) {
    (void)interp;
    
    if (argc != 0) {
        // For now, io.read_line takes no arguments.
        // We could add a prompt string in the future.
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    char line[4096];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }

    // Remove newline
    size_t len = strlen(line);
    if (len > 0 && line[len-1] == '\n') {
        line[len-1] = '\0';
        len--;
    }

    // Create a RADS string
    char* buffer = malloc(len + 1);
    if (!buffer) {
        Value v;
        v.type = VAL_NULL;
        return v;
    }
    memcpy(buffer, line, len + 1);

    Value v;
    v.type = VAL_STRING;
    v.string_val = buffer; // Ownership transferred to Value
    return v;
}


// Registration
void stdlib_io_register(void) {
    register_native("print", native_io_print);      /* global — no module (20261101) */
    register_native("str", native_io_str);          /* global — no module (20261101) */
    register_native("type", native_io_type);        /* global — no module (20261101) */
    register_native("chr", native_io_chr);          /* global — no module (20261101) */
    register_native("ord", native_io_ord);          /* global — no module (20261101) */
    register_native("io.read_file", native_io_read_file);
    register_native("io.write_file", native_io_write_file);
    register_native("io.file_exists", native_io_file_exists);
    register_native("io.delete_file", native_io_delete_file);
    register_native("io.append_file", native_io_append_file);
    register_native("io.file_size", native_io_file_size);
    register_native("io.read_line", native_io_read_line);
}
