#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define STACK_MAX 256
#define FRAMES_MAX 256

void vm_init(VM* vm) {
    vm->chunk = NULL;
    vm->frame_count = 0;
    vm_reset_stack(vm);
    vm->global_count = 0;
    vm->global_capacity = 0;
    vm->globals = NULL;
    vm->debug_mode = false;
    vm->instruction_count = 0;

    memset(&vm->natives, 0, sizeof(vm->natives));
}

void vm_free(VM* vm) {
    if (vm->chunk) {
        chunk_free(vm->chunk);
    }
    if (vm->globals) {
        free(vm->globals);
    }
}

void vm_reset_stack(VM* vm) {
    vm->stack_top = vm->stack;
}

void vm_push(VM* vm, Value value) {
    *vm->stack_top = value;
    vm->stack_top++;
}

Value vm_pop(VM* vm) {
    vm->stack_top--;
    return *vm->stack_top;
}

Value vm_peek(VM* vm, int distance) {
    return vm->stack_top[-1 - distance];
}

Chunk* chunk_create(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk->code_capacity = 0;
    chunk->code_count = 0;
    chunk->code = NULL;
    chunk->constant_capacity = 0;
    chunk->constant_count = 0;
    chunk->constants = NULL;
    chunk->lines = NULL;
    return chunk;
}

void chunk_free(Chunk* chunk) {
    if (!chunk) return;

    if (chunk->code) free(chunk->code);
    if (chunk->constants) free(chunk->constants);
    if (chunk->lines) free(chunk->lines);

    free(chunk);
}

void chunk_write(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->code_capacity < chunk->code_count + 1) {
        int old_capacity = chunk->code_capacity;
        chunk->code_capacity = old_capacity < 8 ? 8 : old_capacity * 2;
        chunk->code = (uint8_t*)realloc(chunk->code, chunk->code_capacity * sizeof(uint8_t));
        chunk->lines = (int*)realloc(chunk->lines, chunk->code_capacity * sizeof(int));
    }

    chunk->code[chunk->code_count] = byte;
    chunk->lines[chunk->code_count] = line;
    chunk->code_count++;
}

int chunk_add_constant(Chunk* chunk, Value value) {
    for (int i = 0; i < chunk->constant_count; i++) {
        if (value_equals(value, chunk->constants[i])) {
            return i;
        }
    }

    if (chunk->constant_capacity < chunk->constant_count + 1) {
        int old_capacity = chunk->constant_capacity;
        chunk->constant_capacity = old_capacity < 8 ? 8 : old_capacity * 2;
        chunk->constants = (Value*)realloc(chunk->constants, chunk->constant_capacity * sizeof(Value));
    }

    chunk->constants[chunk->constant_count] = value;
    return chunk->constant_count++;
}

Value value_null(void) {
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value value_bool(bool boolean) {
    Value v;
    v.type = VAL_BOOL;
    v.as.boolean = boolean;
    return v;
}

Value value_number(double number) {
    Value v;
    v.type = VAL_NUMBER;
    v.as.number = number;
    return v;
}

Value value_string(char* string) {
    Value v;
    v.type = VAL_STRING;
    v.as.string = strdup(string);
    return v;
}

Value value_array(int capacity) {
    Value v;
    v.type = VAL_ARRAY;
    v.as.array = (VMArray*)malloc(sizeof(VMArray));
    v.as.array->capacity = capacity;
    v.as.array->count = 0;
    v.as.array->values = (Value*)malloc(capacity * sizeof(Value));
    return v;
}

Value value_struct(const char* type_name, int field_count) {
    Value v;
    v.type = VAL_STRUCT;
    v.as.object = (VMStruct*)malloc(sizeof(VMStruct));
    v.as.object->type_name = strdup(type_name);
    v.as.object->field_count = field_count;
    v.as.object->field_names = (char**)malloc(field_count * sizeof(char*));
    v.as.object->field_values = (Value*)malloc(field_count * sizeof(Value));
    for (int i = 0; i < field_count; i++) {
        v.as.object->field_names[i] = NULL;
        v.as.object->field_values[i] = value_null();
    }
    return v;
}

void value_print(Value value) {
    switch (value.type) {
        case VAL_NULL:
            printf("null");
            break;
        case VAL_BOOL:
            printf(value.as.boolean ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf("%g", value.as.number);
            break;
        case VAL_STRING:
            printf("%s", value.as.string);
            break;
        case VAL_ARRAY:
            printf("[array with %d items]", value.as.array->count);
            break;
        case VAL_STRUCT:
            printf("%s", value.as.object->type_name);
            break;
        default:
            printf("unknown value");
    }
}

bool value_equals(Value a, Value b) {
    if (a.type != b.type) return false;

    switch (a.type) {
        case VAL_NULL: return true;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_NUMBER: return a.as.number == b.as.number;
        case VAL_STRING: return strcmp(a.as.string, b.as.string) == 0;
        default: return false;
    }
}

static void binary_op(VM* vm, char op) {
    Value b = vm_pop(vm);
    Value a = vm_pop(vm);

    if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
        fprintf(stderr, "Runtime error: Operands must be numbers\n");
        return;
    }

    double result;
    switch (op) {
        case '+': result = a.as.number + b.as.number; break;
        case '-': result = a.as.number - b.as.number; break;
        case '*': result = a.as.number * b.as.number; break;
        case '/': result = a.as.number / b.as.number; break;
        case '%': result = fmod(a.as.number, b.as.number); break;
        default: return;
    }

    vm_push(vm, value_number(result));
}

static void comparison_op(VM* vm, char op) {
    Value b = vm_pop(vm);
    Value a = vm_pop(vm);

    if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
        fprintf(stderr, "Runtime error: Operands must be numbers\n");
        return;
    }

    bool result;
    switch (op) {
        case '<': result = a.as.number < b.as.number; break;
        case '>': result = a.as.number > b.as.number; break;
        case 'L': result = a.as.number <= b.as.number; break;
        case 'G': result = a.as.number >= b.as.number; break;
        case '=': result = a.as.number == b.as.number; break;
        case '!': result = a.as.number != b.as.number; break;
        default: return;
    }

    vm_push(vm, value_bool(result));
}

void vm_interpret(VM* vm, Chunk* chunk) {
    vm->chunk = chunk;

    uint8_t* ip = chunk->code;

    #define READ_BYTE() (*ip++)
    #define READ_CONSTANT() (chunk->constants[READ_BYTE()])
    #define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

    for (;;) {
        uint8_t instruction = READ_BYTE();
        vm->instruction_count++;

        switch (instruction) {
            case OP_CONST: {
                Value constant = READ_CONSTANT();
                vm_push(vm, constant);
                break;
            }
            case OP_NULL:
                vm_push(vm, value_null());
                break;
            case OP_TRUE:
                vm_push(vm, value_bool(true));
                break;
            case OP_FALSE:
                vm_push(vm, value_bool(false));
                break;
            case OP_POP:
                vm_pop(vm);
                break;
            case OP_ADD:
                binary_op(vm, '+');
                break;
            case OP_SUB:
                binary_op(vm, '-');
                break;
            case OP_MUL:
                binary_op(vm, '*');
                break;
            case OP_DIV:
                binary_op(vm, '/');
                break;
            case OP_MOD:
                binary_op(vm, '%');
                break;
            case OP_NEG: {
                Value v = vm_pop(vm);
                if (v.type != VAL_NUMBER) {
                    fprintf(stderr, "Runtime error: Operand must be a number\n");
                    return;
                }
                vm_push(vm, value_number(-v.as.number));
                break;
            }
            case OP_EQ:
                comparison_op(vm, '=');
                break;
            case OP_NEQ:
                comparison_op(vm, '!');
                break;
            case OP_LT:
                comparison_op(vm, '<');
                break;
            case OP_LTE:
                comparison_op(vm, 'L');
                break;
            case OP_GT:
                comparison_op(vm, '>');
                break;
            case OP_GTE:
                comparison_op(vm, 'G');
                break;
            case OP_NOT: {
                Value v = vm_pop(vm);
                if (v.type != VAL_BOOL) {
                    fprintf(stderr, "Runtime error: Operand must be a boolean\n");
                    return;
                }
                vm_push(vm, value_bool(!v.as.boolean));
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (vm_peek(vm, 0).as.boolean == false) {
                    ip += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
                break;
            }
            case OP_CALL: {
                int arg_count = READ_BYTE();
                Value func = vm_peek(vm, arg_count);

                if (func.type == VAL_NATIVE) {
                    NativeFn fn = (NativeFn)func.as.native;
                    Value result = fn(arg_count, vm->stack_top - arg_count);

                    vm->stack_top -= arg_count + 1;
                    vm_push(vm, result);
                } else {
                    fprintf(stderr, "Runtime error: Can only call native functions\n");
                    return;
                }
                break;
            }
            case OP_RETURN: {
                Value result = vm_pop(vm);
                value_print(result);
                printf("\n");
                return;
            }
            case OP_CONCAT: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);

                if (a.type != VAL_STRING || b.type != VAL_STRING) {
                    fprintf(stderr, "Runtime error: Operands must be strings\n");
                    return;
                }

                int len_a = strlen(a.as.string);
                int len_b = strlen(b.as.string);
                char* result = (char*)malloc(len_a + len_b + 1);
                strcpy(result, a.as.string);
                strcat(result, b.as.string);

                vm_push(vm, value_string(result));
                free(result);
                break;
            }
            case OP_ARRAY: {
                int count = READ_BYTE();
                Value arr = value_array(count);
                arr.as.array->count = count;
                for (int i = count - 1; i >= 0; i--) {
                    arr.as.array->values[i] = vm_pop(vm);
                }
                vm_push(vm, arr);
                break;
            }
            default:
                fprintf(stderr, "Unknown opcode: %d\n", instruction);
                return;
        }

        if (vm->stack_top - vm->stack > STACK_MAX) {
            fprintf(stderr, "Stack overflow\n");
            return;
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_SHORT
}

void vm_set_native(VM* vm, const char* name, NativeFn fn) {
    vm_push(vm, value_string((char*)name));
    Value func_val;
    func_val.type = VAL_NATIVE;
    func_val.as.native = (void*)fn;
    vm_push(vm, func_val);
}

static const char* opcode_names[] = {
    "OP_NOP", "OP_CONST", "OP_NULL", "OP_TRUE", "OP_FALSE",
    "OP_GET_LOCAL", "OP_SET_LOCAL", "OP_GET_GLOBAL", "OP_SET_GLOBAL",
    "OP_GET_UPVALUE", "OP_SET_UPVALUE", "OP_GET_FIELD", "OP_SET_FIELD",
    "OP_POP", "OP_DUP", "OP_SWAP", "OP_ADD", "OP_SUB",
    "OP_MUL", "OP_DIV", "OP_MOD", "OP_NEG", "OP_EQ",
    "OP_NEQ", "OP_LT", "OP_LTE", "OP_GT", "OP_GTE",
    "OP_NOT", "OP_AND", "OP_OR", "OP_JUMP", "OP_JUMP_IF_FALSE",
    "OP_JUMP_IF_TRUE", "OP_LOOP", "OP_CALL", "OP_INVOKE", "OP_RETURN",
    "OP_CALL_NATIVE", "OP_ARRAY", "OP_GET_INDEX", "OP_SET_INDEX",
    "OP_CONCAT", "OP_IS_NULL", "OP_IS_BOOL", "OP_IS_NUMBER",
    "OP_IS_STRING", "OP_IS_ARRAY", "OP_IS_STRUCT", "OP_BREAK", "OP_CONTINUE"
};

static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constant_instruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    value_print(chunk->constants[constant]);
    printf("'\n");
    return offset + 2;
}

static int byte_instruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jump_instruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

void chunk_disassemble(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->code_count;) {
        offset = chunk_disassemble_instruction(chunk, offset);
    }

    printf("\n");
}

int chunk_disassemble_instruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];

    switch (instruction) {
        case OP_CONST:
            return constant_instruction("OP_CONST", chunk, offset);
        case OP_NULL:
        case OP_TRUE:
        case OP_FALSE:
        case OP_POP:
        case OP_NEG:
        case OP_NOT:
        case OP_RETURN:
            return simple_instruction(opcode_names[instruction], offset);
        case OP_ARRAY:
            return byte_instruction("OP_ARRAY", chunk, offset);
        case OP_CALL:
            return byte_instruction("OP_CALL", chunk, offset);
        case OP_JUMP:
            return jump_instruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jump_instruction("OP_LOOP", -1, chunk, offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
