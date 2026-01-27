#ifndef RADS_VM_H
#define RADS_VM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OP_NOP = 0,
    OP_CONST,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_FIELD,
    OP_SET_FIELD,
    OP_POP,
    OP_DUP,
    OP_SWAP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_NOT,
    OP_AND,
    OP_OR,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_RETURN,
    OP_CALL_NATIVE,
    OP_ARRAY,
    OP_GET_INDEX,
    OP_SET_INDEX,
    OP_CONCAT,
    OP_IS_NULL,
    OP_IS_BOOL,
    OP_IS_NUMBER,
    OP_IS_STRING,
    OP_IS_ARRAY,
    OP_IS_STRUCT,
    OP_BREAK,
    OP_CONTINUE
} Opcode;

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_ARRAY,
    VAL_STRUCT,
    VAL_FUNCTION,
    VAL_NATIVE
} ValueType;

typedef struct VMArray {
    int capacity;
    int count;
    struct Value* values;
} VMArray;

typedef struct VMStruct {
    char* type_name;
    int field_count;
    char** field_names;
    struct Value* field_values;
} VMStruct;

typedef struct VMFunction {
    char* name;
    int arity;
    uint8_t* code;
    size_t code_size;
    int* line_numbers;
} VMFunction;

typedef struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        char* string;
        VMArray* array;
        VMStruct* object;
        VMFunction* function;
        void* native;
    } as;
} Value;

typedef Value (*NativeFn)(int arg_count, Value* args);

typedef struct CallFrame {
    VMFunction* function;
    uint8_t* ip;
    struct Value* slots;
} CallFrame;

typedef struct Chunk {
    uint8_t* code;
    size_t code_capacity;
    size_t code_count;
    struct Value* constants;
    int constant_capacity;
    int constant_count;
    int* lines;
} Chunk;

typedef struct VM {
    CallFrame frames[256];
    int frame_count;
    struct Value stack[256];
    struct Value* stack_top;
    Chunk* chunk;
    struct Value* globals;
    int global_capacity;
    int global_count;
    struct {
        NativeFn print;
        NativeFn println;
        NativeFn array_push;
        NativeFn array_pop;
        NativeFn array_length;
        NativeFn string_length;
        NativeFn string_substring;
        NativeFn math_sin;
        NativeFn math_cos;
        NativeFn math_sqrt;
    } natives;
    bool debug_mode;
    int instruction_count;
} VM;

void vm_init(VM* vm);
void vm_free(VM* vm);
Chunk* chunk_create(void);
void chunk_free(Chunk* chunk);
void chunk_write(Chunk* chunk, uint8_t byte, int line);
int chunk_add_constant(Chunk* chunk, Value value);
void vm_interpret(VM* vm, Chunk* chunk);
void vm_set_native(VM* vm, const char* name, NativeFn fn);
void vm_push(VM* vm, Value value);
Value vm_pop(VM* vm);
Value vm_peek(VM* vm, int distance);
void vm_reset_stack(VM* vm);
Value value_null(void);
Value value_bool(bool boolean);
Value value_number(double number);
Value value_string(char* string);
Value value_array(int capacity);
Value value_struct(const char* type_name, int field_count);
void value_print(Value value);
bool value_equals(Value a, Value b);
void chunk_disassemble(Chunk* chunk, const char* name);
int chunk_disassemble_instruction(Chunk* chunk, int offset);

#endif
