#ifndef RADS_INTERPRETER_H
#define RADS_INTERPRETER_H

#include "ast.h"
#include <uv.h>

// Execution result for statements
typedef enum {
    EXEC_OK = 0,
    EXEC_BREAK,
    EXEC_CONTINUE,
    EXEC_RETURN,
    EXEC_THROW
} ExecResult;

// Value types for runtime
typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_ARRAY,
    VAL_STRUCT_DEF,
    VAL_STRUCT_INSTANCE,
    VAL_ITERATOR
} ValueType;

struct Value; // Forward declaration
typedef struct Iterator Iterator; // forward — Value union carries Iterator*

typedef struct Array {
    size_t refcount;
    size_t count;
    size_t capacity;
    struct Value* items;
} Array;

typedef struct FieldValue {
    char* name;
    struct Value* value;
    struct FieldValue* next;
} FieldValue;

typedef struct StructInstance {
    struct StructDef* definition;
    FieldValue* fields;
} StructInstance;

typedef struct StructDef {
    char* name;
    ASTNode* ast_node;
} StructDef;

typedef struct Value {
    ValueType type;
    union {
        bool bool_val;
        long long int_val;
        double float_val;
        char* string_val;
        ASTNode* func_node;
        Array* array_val;
        StructDef* struct_def;
        StructInstance* struct_instance;
        Iterator* iterator_val;
    };
} Value;

/* Lazy iterator protocol (nightshift 20261025): one-shot, pull-based.
 * vtable->next fills *out with an OWNED Value and returns true; false means
 * exhausted (and stays exhausted — one-shot like Python generators).
 * cleanup frees the flavor state exactly once, when refcount hits zero.
 * value_clone() shares the cursor (aliases observe the same position), like
 * arrays share items. State payloads that hold Values must value_retain()
 * them at construction and value_release() them in cleanup. */
typedef struct IteratorVTable {
    bool (*next)(Iterator* it, Value* out); /* pull one owned value */
    void (*cleanup)(Iterator* it);          /* free flavor state */
} IteratorVTable;

struct Iterator {
    size_t refcount;
    const IteratorVTable* vtable;
    void* state; /* per-flavor cursor; owns its Value payloads */
};

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
void register_native_fn(const char* module, const char* name, NativeFn fn);
uv_loop_t* interpreter_init_event_loop(void);
void interpreter_cleanup_event_loop(void);
void interpreter_cleanup_environment(void);
void interpreter_run_event_loop(void);
Value interpreter_execute_callback(Value callback, int argc, Value* args);

Value make_int(long long val);
Value make_float(double val);
Value make_string(const char* val);
Value make_error(const char* msg); /* prints message, returns null */
Value make_bool(bool val);
Value make_null(void);
Value value_retain(Value v); /* own-payload copy: survives caller value_free */
Value make_iterator(const IteratorVTable* vtable, void* state); /* refcount=1 */
bool iterator_next(Value* it, Value* out); /* false = exhausted or not an iterator */
Array* array_create(size_t capacity);
void array_push(Array* arr, Value v);

#endif // RADS_INTERPRETER_H
