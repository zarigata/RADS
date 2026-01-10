#ifndef RADS_INTERPRETER_H
#define RADS_INTERPRETER_H

#include "ast.h"
#include <uv.h>

// Execution result for statements
typedef enum {
    EXEC_OK = 0,
    EXEC_BREAK,
    EXEC_CONTINUE,
    EXEC_RETURN
} ExecResult;

// Value types for runtime
typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_ARRAY
} ValueType;

struct Value; // Forward declaration

typedef struct Array {
    size_t refcount;
    size_t count;
    size_t capacity;
    struct Value* items;
} Array;

typedef struct Value {
    ValueType type;
    union {
        bool bool_val;
        long long int_val;
        double float_val;
        char* string_val;
        ASTNode* func_node;
        Array* array_val;
    };
} Value;

typedef struct Interpreter {
    uv_loop_t* event_loop;
} Interpreter;

// Interpreter functions
int interpret(ASTNode* program);
int interpret_repl_statement(ASTNode* stmt);  // For REPL - executes single statement
void value_print(Value* value);
void value_free(Value* value);

// Native function type
struct Interpreter; // Forward decl
typedef Value (*NativeFn)(struct Interpreter* interp, int argc, Value* args);

// Function types
typedef enum {
    FUNC_USER,
    FUNC_NATIVE
} FunctionType;

void register_native(const char* name, NativeFn fn);
uv_loop_t* interpreter_init_event_loop(void);
void interpreter_cleanup_event_loop(void);
void interpreter_cleanup_environment(void);  // Clean up global environment (for REPL exit)
void interpreter_run_event_loop(void);
Value interpreter_execute_callback(Value callback, int argc, Value* args);

#endif // RADS_INTERPRETER_H
