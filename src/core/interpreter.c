#define _POSIX_C_SOURCE 200809L
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static Value eval_expression(ASTNode* node);
static ExecResult exec_statement(ASTNode* node);
static void exec_function(ASTNode* node);
static bool is_truthy(Value v);

uv_loop_t* global_event_loop = NULL;
static struct Interpreter global_interpreter_instance = {0};
static struct Interpreter* global_interpreter = NULL;

static void handle_sigint(int signo) {
    (void)signo;
    if (global_event_loop) {
        uv_stop(global_event_loop);
    }
}

uv_loop_t* interpreter_init_event_loop(void) {
    if (global_event_loop) return global_event_loop;
    global_event_loop = uv_default_loop();
    global_interpreter_instance.event_loop = global_event_loop;
    global_interpreter = &global_interpreter_instance;
#ifdef RADS_PLATFORM_WINDOWS
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    signal(SIGINT, handle_sigint);
    return global_event_loop;
}

void interpreter_run_event_loop(void) {
    if (!global_event_loop) return;
    uv_run(global_event_loop, UV_RUN_DEFAULT);
}

void interpreter_cleanup_event_loop(void) {
    if (!global_event_loop) return;
    uv_loop_close(global_event_loop);
    global_event_loop = NULL;
#ifdef RADS_PLATFORM_WINDOWS
    WSACleanup();
#endif
}

Value make_null() {
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value make_bool(bool val) { Value v = { .type = VAL_BOOL, .bool_val = val }; return v; }
Value make_int(long long val) { Value v = { .type = VAL_INT, .int_val = val }; return v; }
Value make_float(double val) { Value v = { .type = VAL_FLOAT, .float_val = val }; return v; }
Value make_string(const char* val) { Value v = { .type = VAL_STRING, .string_val = strdup(val) }; return v; }

Array* array_create(size_t capacity) {
    Array* arr = malloc(sizeof(Array));
    arr->refcount = 1;
    arr->count = 0;
    arr->capacity = capacity > 0 ? capacity : 4;
    arr->items = calloc(arr->capacity, sizeof(Value));
    return arr;
}

static Value value_clone(Value v);
static void value_release(Value* v);

void array_push(Array* arr, Value v) {
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity * 2;
        arr->items = realloc(arr->items, arr->capacity * sizeof(Value));
    }
    arr->items[arr->count++] = value_clone(v);
}

static Value make_array_from_list(ASTList* elements) {
    Array* arr = array_create(elements ? elements->count : 0);
    if (elements) {
        for (size_t i = 0; i < elements->count; i++) {
            Value ev = eval_expression(elements->nodes[i]);
            array_push(arr, ev);
            value_release(&ev);
        }
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

static Value value_clone(Value v) {
    Value out = v;
    switch (v.type) {
        case VAL_STRING:
            out.string_val = strdup(v.string_val);
            break;
        case VAL_ARRAY:
            if (v.array_val) v.array_val->refcount++;
            break;
        case VAL_STRUCT_INSTANCE:
            if (v.struct_instance) {
                StructInstance* new_instance = malloc(sizeof(StructInstance));
                new_instance->definition = v.struct_instance->definition; // Share definition
                new_instance->fields = NULL;

                // Deep copy all fields
                FieldValue* current = v.struct_instance->fields;
                FieldValue** tail = &new_instance->fields;
                while (current) {
                    FieldValue* new_field = malloc(sizeof(FieldValue));
                    new_field->name = strdup(current->name);
                    new_field->value = malloc(sizeof(Value));
                    *new_field->value = value_clone(*current->value);
                    new_field->next = NULL;
                    *tail = new_field;
                    tail = &new_field->next;
                    current = current->next;
                }
                out.struct_instance = new_instance;
            }
            break;
        default:
            break;
    }
    return out;
}

static void value_release(Value* value) {
    if (!value) return;
    switch (value->type) {
        case VAL_STRING:
            free(value->string_val);
            break;
        case VAL_ARRAY:
            if (value->array_val) {
                if (value->array_val->refcount > 0) value->array_val->refcount--;
                if (value->array_val->refcount == 0) {
                    for (size_t i = 0; i < value->array_val->count; i++) {
                        value_release(&value->array_val->items[i]);
                    }
                    free(value->array_val->items);
                    free(value->array_val);
                }
            }
            break;
        case VAL_STRUCT_DEF:
            // Handled by the struct registry
            break;
        case VAL_STRUCT_INSTANCE:
            if (value->struct_instance) {
                FieldValue* current = value->struct_instance->fields;
                while (current) {
                    FieldValue* next = current->next;
                    free(current->name);
                    value_release(current->value);
                    free(current->value);
                    free(current);
                    current = next;
                }
                free(value->struct_instance);
            }
            break;
        default:
            break;
    }
    value->type = VAL_NULL;
}

void value_print(Value* value) {
    switch (value->type) {
        case VAL_NULL:
            printf("null");
            break;
        case VAL_BOOL:
            printf("%s", value->bool_val ? "true" : "false");
            break;
        case VAL_INT:
            printf("%lld", value->int_val);
            break;
        case VAL_FLOAT:
            printf("%f", value->float_val);
            break;
        case VAL_STRING:
            printf("%s", value->string_val);
            break;
        case VAL_ARRAY: {
            printf("[");
            for (size_t i = 0; i < value->array_val->count; i++) {
                if (i > 0) printf(", ");
                value_print(&value->array_val->items[i]);
            }
            printf("]");
            break;
        }
        case VAL_FUNCTION:
            if (value->func_node->function_decl.name) {
                printf("<blast function %s>", value->func_node->function_decl.name);
            } else {
                printf("<blast anonymous function>");
            }
            break;
        case VAL_STRUCT_DEF:
            printf("<struct def %s>", value->struct_def->name);
            break;
        case VAL_STRUCT_INSTANCE:
            printf("<struct instance %s>", value->struct_instance->definition->name);
            break;
    }
}

void value_free(Value* value) { value_release(value); }

// Environment (Variables)
typedef struct Environment {
    char* name;
    Value value;
    struct Environment* next;
} Environment;

static Environment* global_env = NULL;

static void env_set(const char* name, Value value) {
    // Always clone the value to ensure proper ownership
    Value cloned_value = value_clone(value);

    // Check existing
    Environment* current = global_env;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            value_free(&current->value); // Free old value
            current->value = cloned_value;
            return;
        }
        current = current->next;
    }

    // Create new
    Environment* env = malloc(sizeof(Environment));
    env->name = strdup(name);
    env->value = cloned_value;
    env->next = global_env;
    global_env = env;
}

static Value env_get(const char* name) {
    Environment* current = global_env;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            // Return a clone to ensure proper ownership
            return value_clone(current->value);
        }
        current = current->next;
    }
    return make_null();
}

// Get a reference to a value in the environment (for in-place modification)
static Value* env_get_ref(const char* name) {
    Environment* current = global_env;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return &current->value;
        }
        current = current->next;
    }
    return NULL;
}

static void env_free() {
    Environment* current = global_env;
    while (current) {
        Environment* next = current->next;
        free(current->name);
        value_free(&current->value);
        free(current);
        current = next;
    }
    global_env = NULL;
}

// Native function registry
typedef struct NativeBinding {
    char* name;
    NativeFn fn;
    struct NativeBinding* next;
} NativeBinding;

static NativeBinding* native_functions = NULL;

void register_native(const char* name, NativeFn fn) {
    NativeBinding* binding = malloc(sizeof(NativeBinding));
    binding->name = strdup(name);
    binding->fn = fn;
    binding->next = native_functions;
    native_functions = binding;
}

static NativeFn find_native(const char* name) {
    NativeBinding* current = native_functions;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->fn;
        }
        current = current->next;
    }
    return NULL;
}

// Struct definition registry
typedef struct StructDefBinding {
    char* name;
    ASTNode* node;
    struct StructDefBinding* next;
} StructDefBinding;

static StructDefBinding* struct_definitions = NULL;

static void register_struct(const char* name, ASTNode* node) {
    StructDefBinding* binding = malloc(sizeof(StructDefBinding));
    binding->name = strdup(name);
    binding->node = node;
    binding->next = struct_definitions;
    struct_definitions = binding;
}

static ASTNode* find_struct(const char* name) {
    StructDefBinding* current = struct_definitions;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->node;
        }
        current = current->next;
    }
    return NULL;
}

static void env_free_structs() {
    StructDefBinding* current = struct_definitions;
    while (current) {
        StructDefBinding* next = current->next;
        free(current->name);
        // The AST node is owned by the main AST, so we don't free it here.
        free(current);
        current = next;
    }
    struct_definitions = NULL;
}

// Enum definition registry
typedef struct EnumDefBinding {
    char* name;
    ASTNode* node;
    struct EnumDefBinding* next;
} EnumDefBinding;

static EnumDefBinding* enum_definitions = NULL;

static void register_enum(const char* name, ASTNode* node) {
    EnumDefBinding* binding = malloc(sizeof(EnumDefBinding));
    binding->name = strdup(name);
    binding->node = node;
    binding->next = enum_definitions;
    enum_definitions = binding;
}

static ASTNode* find_enum(const char* name) {
    EnumDefBinding* current = enum_definitions;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->node;
        }
        current = current->next;
    }
    return NULL;
}

static void env_free_enums() {
    EnumDefBinding* current = enum_definitions;
    while (current) {
        EnumDefBinding* next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    enum_definitions = NULL;
}

// Evaluate call expression
static Value eval_call(ASTNode* node) {
    // For now, only support calling global native functions or simple function calls
    // Real implementation would need full environment/scope support
    
    char* name = NULL;
    char* full_name = NULL;
    
    // Member expression handling: allow dispatch of methods on string handles (e.g., server.route) and array methods
    if (node->call_expr.callee->type == AST_MEMBER_EXPR) {
        ASTNode* obj = node->call_expr.callee->member_expr.object;
        const char* member = node->call_expr.callee->member_expr.member;
        // Attempt to dispatch to native function with implicit handle argument
        Value obj_val = eval_expression(obj);

        // Array method calls
        if (obj_val.type == VAL_ARRAY && member) {
            if (strcmp(member, "push") == 0) {
                // arr.push(value) - add element to end
                int argc = node->call_expr.arguments ? (int)node->call_expr.arguments->count : 0;
                if (argc > 0) {
                    Value val_to_push = eval_expression(node->call_expr.arguments->nodes[0]);
                    array_push(obj_val.array_val, val_to_push);
                    value_free(&val_to_push);
                }
                value_free(&obj_val);
                return make_null();
            } else if (strcmp(member, "pop") == 0) {
                // arr.pop() - remove and return last element
                Value result = make_null();
                if (obj_val.array_val->count > 0) {
                    result = value_clone(obj_val.array_val->items[obj_val.array_val->count - 1]);
                    value_release(&obj_val.array_val->items[obj_val.array_val->count - 1]);
                    obj_val.array_val->count--;
                }
                value_free(&obj_val);
                return result;
            }
        }

        if (obj_val.type == VAL_STRING && member) {
            char native_name[64];
            snprintf(native_name, sizeof(native_name), "net.%s", member);
            NativeFn native = find_native(native_name);
            if (native) {
                int argc = node->call_expr.arguments ? (int)node->call_expr.arguments->count : 0;
                Value* args = malloc(sizeof(Value) * (argc + 1));
                args[0] = value_clone(obj_val);
                for (int i = 0; i < argc; i++) {
                    args[i + 1] = eval_expression(node->call_expr.arguments->nodes[i]);
                }
                Value result = native(global_interpreter, argc + 1, args);
                for (int i = 0; i < argc + 1; i++) {
                    value_free(&args[i]);
                }
                free(args);
                value_free(&obj_val);
                return result;
            }
        }
        value_free(&obj_val);

        if (obj->type == AST_IDENTIFIER && member) {
            char* obj_name = obj->identifier.name;
            int len = strlen(obj_name) + 1 + strlen(member) + 1;
            full_name = malloc(len);
            sprintf(full_name, "%s.%s", obj_name, member);
            name = full_name;
        }
    } else if (node->call_expr.callee->type == AST_IDENTIFIER) {
        name = node->call_expr.callee->identifier.name;
    }
    
    if (name) {
        // Check for native function
        NativeFn native = find_native(name);
        if (native) {
             int argc = node->call_expr.arguments ? (int)node->call_expr.arguments->count : 0;
            Value* args = malloc(sizeof(Value) * argc);

            for (int i = 0; i < argc; i++) {
                args[i] = eval_expression(node->call_expr.arguments->nodes[i]);
            }

            Value result = native(global_interpreter, argc, args);

            // Cleanup args
            for (int i = 0; i < argc; i++) {
                value_free(&args[i]);
            }
            free(args);
            if (full_name) free(full_name);
            return result;
        }
        
        // Check for user function in environment
        Value func_val = env_get(name);
        if (func_val.type == VAL_FUNCTION) {
            // Call the function with parameters and return value support
            int argc = node->call_expr.arguments ? (int)node->call_expr.arguments->count : 0;
            Value* args = malloc(sizeof(Value) * (argc > 0 ? argc : 1));

            for (int i = 0; i < argc; i++) {
                args[i] = eval_expression(node->call_expr.arguments->nodes[i]);
            }

            Value result = interpreter_execute_callback(func_val, argc, args);

            // Cleanup args
            for (int i = 0; i < argc; i++) {
                value_free(&args[i]);
            }
            free(args);
            value_free(&func_val);
            if (full_name) free(full_name);
            return result;
        }
        value_free(&func_val);

        if (full_name) free(full_name);
    }
    
    // Fallback for user functions (not fully implemented in this prototype)
    return make_null();
}

// Evaluate unary operation
static Value eval_unary_op(ASTNode* node) {
    Value operand = eval_expression(node->unary_op.operand);
    Value result = make_null();

    switch (node->unary_op.op) {
        case OP_NEG:
            // Negate numbers
            if (operand.type == VAL_INT) {
                result = make_int(-operand.int_val);
            } else if (operand.type == VAL_FLOAT) {
                result = make_float(-operand.float_val);
            }
            break;
        case OP_NOT:
            // Logical NOT
            if (operand.type == VAL_BOOL) {
                result = make_bool(!operand.bool_val);
            } else if (operand.type == VAL_INT) {
                result = make_bool(operand.int_val == 0);
            } else if (operand.type == VAL_NULL) {
                result = make_bool(true);
            }
            break;
        default:
            break;
    }

    value_free(&operand);
    return result;
}

// Evaluate binary operation
static Value eval_binary_op(ASTNode* node) {
    Value left = eval_expression(node->binary_op.left);
    Value right = eval_expression(node->binary_op.right);
    
    Value result = make_null();
    
    switch (node->binary_op.op) {
        case OP_ADD:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val + right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_float(left.float_val + right.float_val);
            } else if (left.type == VAL_STRING || right.type == VAL_STRING) {
                // String concatenation with automatic conversion
                char left_buf[256] = "";
                char right_buf[256] = "";
                const char* left_str = left_buf;
                const char* right_str = right_buf;

                // Convert left to string
                if (left.type == VAL_STRING) {
                    left_str = left.string_val;
                } else if (left.type == VAL_INT) {
                    snprintf(left_buf, sizeof(left_buf), "%lld", left.int_val);
                } else if (left.type == VAL_FLOAT) {
                    snprintf(left_buf, sizeof(left_buf), "%g", left.float_val);
                } else if (left.type == VAL_BOOL) {
                    snprintf(left_buf, sizeof(left_buf), "%s", left.bool_val ? "true" : "false");
                } else if (left.type == VAL_ARRAY) {
                    // Convert array to string representation
                    if (left.array_val) {
                        char* arr_str = malloc(1024);
                        strcpy(arr_str, "[");
                        for (size_t i = 0; i < left.array_val->count && strlen(arr_str) < 1000; i++) {
                            if (i > 0) strcat(arr_str, ", ");
                            Value* item = &left.array_val->items[i];
                            char item_buf[64];
                            if (item->type == VAL_INT) {
                                snprintf(item_buf, sizeof(item_buf), "%lld", item->int_val);
                            } else if (item->type == VAL_FLOAT) {
                                snprintf(item_buf, sizeof(item_buf), "%g", item->float_val);
                            } else if (item->type == VAL_STRING) {
                                snprintf(item_buf, sizeof(item_buf), "%s", item->string_val);
                            } else if (item->type == VAL_BOOL) {
                                strcpy(item_buf, item->bool_val ? "true" : "false");
                            } else {
                                strcpy(item_buf, "null");
                            }
                            strcat(arr_str, item_buf);
                        }
                        strcat(arr_str, "]");
                        left_str = left_buf;
                        strncpy(left_buf, arr_str, sizeof(left_buf) - 1);
                        left_buf[sizeof(left_buf) - 1] = '\0';
                        free(arr_str);
                    } else {
                        strcpy(left_buf, "[]");
                    }
                } else {
                    strcpy(left_buf, "null");
                }

                // Convert right to string
                if (right.type == VAL_STRING) {
                    right_str = right.string_val;
                } else if (right.type == VAL_INT) {
                    snprintf(right_buf, sizeof(right_buf), "%lld", right.int_val);
                } else if (right.type == VAL_FLOAT) {
                    snprintf(right_buf, sizeof(right_buf), "%g", right.float_val);
                } else if (right.type == VAL_BOOL) {
                    snprintf(right_buf, sizeof(right_buf), "%s", right.bool_val ? "true" : "false");
                } else if (right.type == VAL_ARRAY) {
                    // Convert array to string representation
                    if (right.array_val) {
                        char* arr_str = malloc(1024);
                        strcpy(arr_str, "[");
                        for (size_t i = 0; i < right.array_val->count && strlen(arr_str) < 1000; i++) {
                            if (i > 0) strcat(arr_str, ", ");
                            Value* item = &right.array_val->items[i];
                            char item_buf[64];
                            if (item->type == VAL_INT) {
                                snprintf(item_buf, sizeof(item_buf), "%lld", item->int_val);
                            } else if (item->type == VAL_FLOAT) {
                                snprintf(item_buf, sizeof(item_buf), "%g", item->float_val);
                            } else if (item->type == VAL_STRING) {
                                snprintf(item_buf, sizeof(item_buf), "%s", item->string_val);
                            } else if (item->type == VAL_BOOL) {
                                strcpy(item_buf, item->bool_val ? "true" : "false");
                            } else {
                                strcpy(item_buf, "null");
                            }
                            strcat(arr_str, item_buf);
                        }
                        strcat(arr_str, "]");
                        right_str = right_buf;
                        strncpy(right_buf, arr_str, sizeof(right_buf) - 1);
                        right_buf[sizeof(right_buf) - 1] = '\0';
                        free(arr_str);
                    } else {
                        strcpy(right_buf, "[]");
                    }
                } else {
                    strcpy(right_buf, "null");
                }

                // Concatenate
                size_t len = strlen(left_str) + strlen(right_str) + 1;
                char* str = malloc(len);
                strcpy(str, left_str);
                strcat(str, right_str);
                result = make_string(str);
                free(str);
            }
            break;
        case OP_RANGE:
            // Range materializes as array of ints for now: [left, right) if increasing, else empty
            if (left.type == VAL_INT && right.type == VAL_INT && right.int_val >= left.int_val) {
                // Placeholder: return null; range iteration handled in cruise during execution
                result = make_null();
            }
            break;
        case OP_SUB:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val - right.int_val);
            }
            break;
        case OP_MUL:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val * right.int_val);
            }
            break;
        case OP_DIV:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_int(left.int_val / right.int_val);
                }
            }
            break;
        case OP_MOD:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_int(left.int_val % right.int_val);
                }
            }
            break;
        case OP_EQ:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val == right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val == right.float_val);
            } else if (left.type == VAL_BOOL && right.type == VAL_BOOL) {
                result = make_bool(left.bool_val == right.bool_val);
            } else if (left.type == VAL_STRING && right.type == VAL_STRING) {
                result = make_bool(strcmp(left.string_val, right.string_val) == 0);
            } else if (left.type == VAL_NULL && right.type == VAL_NULL) {
                result = make_bool(true);
            } else {
                result = make_bool(false);
            }
            break;
        case OP_NEQ:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val != right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val != right.float_val);
            } else if (left.type == VAL_BOOL && right.type == VAL_BOOL) {
                result = make_bool(left.bool_val != right.bool_val);
            } else if (left.type == VAL_STRING && right.type == VAL_STRING) {
                result = make_bool(strcmp(left.string_val, right.string_val) != 0);
            } else if (left.type == VAL_NULL && right.type == VAL_NULL) {
                result = make_bool(false);
            } else {
                result = make_bool(true);
            }
            break;
        case OP_LT:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val < right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val < right.float_val);
            }
            break;
        case OP_LTE:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val <= right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val <= right.float_val);
            }
            break;
        case OP_GT:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val > right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val > right.float_val);
            }
            break;
        case OP_GTE:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val >= right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val >= right.float_val);
            }
            break;
        case OP_AND:
            result = make_bool(is_truthy(left) && is_truthy(right));
            break;
        case OP_OR:
            result = make_bool(is_truthy(left) || is_truthy(right));
            break;
        default:
            break;
    }
    
    value_free(&left);
    value_free(&right);
    
    return result;
}

// Evaluate expression
static Value eval_expression(ASTNode* node) {
    if (!node) return make_null();
    
    switch (node->type) {
        case AST_INTEGER_LITERAL:
            return make_int(node->integer_literal.value);
        
        case AST_FLOAT_LITERAL:
            return make_float(node->float_literal.value);
        
        case AST_STRING_LITERAL:
            return make_string(node->string_literal.value);
        
        case AST_BOOL_LITERAL:
            return make_bool(node->bool_literal.value);
        
        case AST_ARRAY_LITERAL:
            return make_array_from_list(node->array_literal.elements);
        
        case AST_INDEX_EXPR: {
            Value arr = eval_expression(node->index_expr.array);
            Value idx = eval_expression(node->index_expr.index);
            Value result = make_null();
            if (arr.type == VAL_ARRAY && idx.type == VAL_INT) {
                if (idx.int_val >= 0 && (size_t)idx.int_val < arr.array_val->count) {
                    // return copy
                    Value elem = arr.array_val->items[idx.int_val];
                    // shallow copy ok; duplicate strings
                    if (elem.type == VAL_STRING) {
                        result = make_string(elem.string_val);
                    } else if (elem.type == VAL_ARRAY) {
                        // simplistic: not deep copy; share
                        result.type = VAL_ARRAY;
                        result.array_val = elem.array_val;
                    } else {
                        result = elem;
                    }
                }
            }
            value_free(&arr);
            value_free(&idx);
            return result;
        }

        case AST_UNARY_OP:
            return eval_unary_op(node);

        case AST_TYPEOF_EXPR: {
            Value val = eval_expression(node->typeof_expr.operand);
            const char* type_str = "unknown";
            switch (val.type) {
                case VAL_INT: type_str = "integer"; break;
                case VAL_FLOAT: type_str = "float"; break;
                case VAL_STRING: type_str = "string"; break;
                case VAL_BOOL: type_str = "bool"; break;
                case VAL_NULL: type_str = "null"; break;
                case VAL_ARRAY: type_str = "array"; break;
                case VAL_FUNCTION: type_str = "function"; break;
                case VAL_STRUCT_DEF: type_str = "struct_def"; break;
                case VAL_STRUCT_INSTANCE: type_str = "struct"; break;
            }
            value_free(&val);
            return make_string(type_str);
        }

        case AST_BINARY_OP:
            return eval_binary_op(node);
            
        case AST_CALL_EXPR:
            return eval_call(node);
            
        case AST_MEMBER_EXPR: {
            // Check for enum value access (e.g., Color.RED)
            if (node->member_expr.object->type == AST_IDENTIFIER) {
                const char* enum_name = node->member_expr.object->identifier.name;
                ASTNode* enum_node = find_enum(enum_name);
                if (enum_node) {
                    const char* value_name = node->member_expr.member;
                    // Find the value index in the enum
                    for (size_t i = 0; i < enum_node->enum_decl.values->count; i++) {
                        ASTNode* val = enum_node->enum_decl.values->nodes[i];
                        if (strcmp(val->identifier.name, value_name) == 0) {
                            return make_int((long long)i);
                        }
                    }
                    fprintf(stderr, "Error: Enum '%s' has no value '%s'.\n", enum_name, value_name);
                    return make_null();
                }
            }

            Value object = eval_expression(node->member_expr.object);
            if (object.type == VAL_STRUCT_INSTANCE) {
                const char* member_name = node->member_expr.member;
                FieldValue* current = object.struct_instance->fields;
                while (current) {
                    if (strcmp(current->name, member_name) == 0) {
                        Value result = value_clone(*current->value);
                        value_release(&object);
                        return result;
                    }
                    current = current->next;
                }
                fprintf(stderr, "Error: Struct '%s' has no member '%s'.\n", object.struct_instance->definition->name, member_name);
            } else if (object.type == VAL_ARRAY) {
                const char* member_name = node->member_expr.member;
                if (strcmp(member_name, "length") == 0) {
                    Value result = make_int((long long)object.array_val->count);
                    value_release(&object);
                    return result;
                }
                fprintf(stderr, "Error: Array has no property '%s'.\n", member_name);
            }
            value_release(&object);
            return make_null();
        }

        case AST_ASSIGN_EXPR: {
            Value value = eval_expression(node->assign_expr.value);
            ASTNode* target = node->assign_expr.target;

            if (target->type == AST_IDENTIFIER) {
                env_set(target->identifier.name, value);
            } else if (target->type == AST_MEMBER_EXPR) {
                // For member assignment, we need to modify the struct in the environment
                // If the object is a simple identifier, modify it directly in the environment
                if (target->member_expr.object->type == AST_IDENTIFIER) {
                    const char* var_name = target->member_expr.object->identifier.name;
                    Value* object_ref = env_get_ref(var_name);
                    if (object_ref && object_ref->type == VAL_STRUCT_INSTANCE) {
                        const char* member_name = target->member_expr.member;
                        FieldValue* current = object_ref->struct_instance->fields;
                        while (current) {
                            if (strcmp(current->name, member_name) == 0) {
                                value_release(current->value);
                                current->value = malloc(sizeof(Value));
                                *current->value = value_clone(value);
                                break;
                            }
                            current = current->next;
                        }
                    }
                } else {
                    // Complex expression - evaluate and modify copy (won't persist)
                    Value object = eval_expression(target->member_expr.object);
                    if (object.type == VAL_STRUCT_INSTANCE) {
                        const char* member_name = target->member_expr.member;
                        FieldValue* current = object.struct_instance->fields;
                        while (current) {
                            if (strcmp(current->name, member_name) == 0) {
                                value_release(current->value);
                                current->value = malloc(sizeof(Value));
                                *current->value = value_clone(value);
                                break;
                            }
                            current = current->next;
                        }
                    }
                    value_release(&object);
                }
            }
            return value;
        }

        case AST_IDENTIFIER:
            return env_get(node->identifier.name);
        
        case AST_STRUCT_LITERAL: {
            ASTNode* def_node = find_struct(node->struct_literal.name);
            if (!def_node) {
                fprintf(stderr, "Error: Struct '%s' not defined.\n", node->struct_literal.name);
                return make_null();
            }

            StructInstance* instance = malloc(sizeof(StructInstance));
            instance->definition = malloc(sizeof(StructDef));
            instance->definition->name = strdup(node->struct_literal.name);
            instance->definition->ast_node = def_node;
            instance->fields = NULL;

            for (size_t i = 0; i < node->struct_literal.fields->count; i++) {
                ASTNode* assign_node = node->struct_literal.fields->nodes[i];

                if (assign_node->type != AST_ASSIGN_EXPR) {
                    fprintf(stderr, "Error: Expected assignment expression in struct literal\n");
                    continue;
                }

                if (assign_node->assign_expr.target->type != AST_IDENTIFIER) {
                    fprintf(stderr, "Error: Expected identifier as field name, got type %d\n", assign_node->assign_expr.target->type);
                    continue;
                }

                FieldValue* field = malloc(sizeof(FieldValue));
                field->name = strdup(assign_node->assign_expr.target->identifier.name);
                field->value = malloc(sizeof(Value));
                *field->value = eval_expression(assign_node->assign_expr.value);
                field->next = instance->fields;
                instance->fields = field;
            }

            Value v;
            v.type = VAL_STRUCT_INSTANCE;
            v.struct_instance = instance;
            return v;
        }

        case AST_FUNCTION_DECL: {
            Value v;
            v.type = VAL_FUNCTION;
            v.func_node = node;
            return v;
        }

        default:
            return make_null();
    }
}

// Execute echo statement
static void exec_echo(ASTNode* node) {
    Value value = eval_expression(node->echo_stmt.expression);
    value_print(&value);
    printf("\n");
    value_free(&value);
}



static Value current_return_value;
static bool has_return_value = false;

static bool is_truthy(Value v) {
    switch (v.type) {
        case VAL_BOOL:
            return v.bool_val;
        case VAL_INT:
            return v.int_val != 0;
        case VAL_FLOAT:
            return v.float_val != 0.0;
        case VAL_STRING:
            return v.string_val && v.string_val[0] != '\0';
        case VAL_FUNCTION:
        case VAL_NULL:
        default:
            return false;
    }
}

// Execute statement
static ExecResult exec_statement(ASTNode* node) {
    if (!node) return EXEC_OK;
    
    switch (node->type) {
        case AST_STRUCT_DECL:
            register_struct(node->struct_decl.name, node);
            return EXEC_OK;

        case AST_ENUM_DECL:
            register_enum(node->enum_decl.name, node);
            return EXEC_OK;

        case AST_ECHO_STMT:
            exec_echo(node);
            return EXEC_OK;

        case AST_IMPORT_STMT: {
            // Basic import: read file, parse, and execute its declarations
            const char* filename = node->import_stmt.filename;
            FILE* file = fopen(filename, "r");
            if (!file) {
                fprintf(stderr, "Error: Cannot import '%s': file not found\n", filename);
                return EXEC_OK;
            }

            // Read entire file
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            char* source = malloc(file_size + 1);
            fread(source, 1, file_size, file);
            source[file_size] = '\0';
            fclose(file);

            // Parse the imported file
            Lexer lexer;
            lexer_init(&lexer, source);
            Parser parser;
            parser_init(&parser, &lexer);
            ASTNode* imported_ast = parser_parse(&parser);
            free(source);

            if (imported_ast && imported_ast->type == AST_PROGRAM && !parser.had_error) {
                // Execute all declarations in the imported file
                for (size_t i = 0; i < imported_ast->program.declarations->count; i++) {
                    ASTNode* decl = imported_ast->program.declarations->nodes[i];
                    if (decl->type == AST_FUNCTION_DECL) {
                        exec_function(decl);
                    } else if (decl->type == AST_STRUCT_DECL) {
                        exec_statement(decl);
                    }
                }
            } else if (parser.had_error) {
                fprintf(stderr, "Error: Failed to parse imported file '%s'\n", filename);
            }

            return EXEC_OK;
        }

        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statements->count; i++) {
                ExecResult r = exec_statement(node->block.statements->nodes[i]);
                if (r == EXEC_BREAK || r == EXEC_CONTINUE || r == EXEC_RETURN) {
                    return r;
                }
            }
            return EXEC_OK;

        case AST_IF_STMT: {
            Value cond = eval_expression(node->if_stmt.condition);
            bool truthy = is_truthy(cond);
            value_free(&cond);

            if (truthy) {
                return exec_statement(node->if_stmt.then_branch);
            } else if (node->if_stmt.else_branch) {
                return exec_statement(node->if_stmt.else_branch);
            }
            return EXEC_OK;
        }

        case AST_LOOP_STMT: {
            ExecResult r = EXEC_OK;
            for (;;) {
                Value cond = eval_expression(node->loop_stmt.condition);
                bool truthy = is_truthy(cond);
                value_free(&cond);
                if (!truthy) break;
                
                r = exec_statement(node->loop_stmt.body);
                if (r == EXEC_BREAK) {
                    return EXEC_OK;
                }
                if (r == EXEC_CONTINUE) {
                    continue;
                }
                if (r == EXEC_RETURN) {
                    return EXEC_RETURN;
                }
            }
            return EXEC_OK;
        }
        
        case AST_CRUISE_STMT: {
            // Expect iterable to be a range binary op or something providing ints; currently handle range only
            ASTNode* iter = node->cruise_stmt.iterable;
            if (iter && iter->type == AST_BINARY_OP && iter->binary_op.op == OP_RANGE) {
                Value start_v = eval_expression(iter->binary_op.left);
                Value end_v = eval_expression(iter->binary_op.right);
                long long start = (start_v.type == VAL_INT) ? start_v.int_val : 0;
                long long end = (end_v.type == VAL_INT) ? end_v.int_val : 0;
                value_free(&start_v);
                value_free(&end_v);
                
                for (long long i = start; i < end; i++) {
                    Value idx = make_int(i);
                    env_set(node->cruise_stmt.iterator, idx);
                    value_free(&idx);
                    
                    ExecResult r = exec_statement(node->cruise_stmt.body);
                    if (r == EXEC_BREAK) {
                        return EXEC_OK;
                    }
                    if (r == EXEC_CONTINUE) {
                        continue;
                    }
                }
            }
            return EXEC_OK;
        }
        
        case AST_BREAK_STMT:
            return EXEC_BREAK;
        case AST_CONTINUE_STMT:
            return EXEC_CONTINUE;
        case AST_RETURN_STMT: {
            Value rv = make_null();
            if (node->return_stmt.value) {
                rv = eval_expression(node->return_stmt.value);
            }
            if (has_return_value) {
                value_free(&current_return_value);
            }
            current_return_value = value_clone(rv);
            has_return_value = true;
            value_free(&rv);
            return EXEC_RETURN;
        }
        
        case AST_VARIABLE_DECL: {
            Value val = make_null();
            if (node->variable_decl.initializer) {
                val = eval_expression(node->variable_decl.initializer);
            }
            env_set(node->variable_decl.name, val);
            value_free(&val); // env_set makes a copy
            return EXEC_OK;
        }
        
        default: {
            Value v = eval_expression(node);
            value_free(&v);
            return EXEC_OK;
        }
    }
}

// Execute function
static void exec_function(ASTNode* node) {
    // Store in environment for lookup (including as first-class values)
    Value v;
    v.type = VAL_FUNCTION;
    v.func_node = node;
    env_set(node->function_decl.name, v);
}

Value interpreter_execute_callback(Value callback, int argc, Value* args) {
    if (callback.type != VAL_FUNCTION) {
        return make_null();
    }
    // Reset return tracking for this invocation
    if (has_return_value) {
        value_free(&current_return_value);
        has_return_value = false;
    }

    ASTNode* func = callback.func_node;
    // Bind parameters naively to globals for now
    if (func->function_decl.parameters) {
        int param_count = (int)func->function_decl.parameters->count;
        for (int i = 0; i < param_count; i++) {
            const char* pname = func->function_decl.parameters->nodes[i]->identifier.name;
            Value arg = (i < argc && args) ? value_clone(args[i]) : make_null();
            env_set(pname, arg);
            value_free(&arg);
        }
    }
    ExecResult r = exec_statement(func->function_decl.body);
    if (r == EXEC_RETURN && has_return_value) {
        Value rv = value_clone(current_return_value);
        value_free(&current_return_value);
        has_return_value = false;
        return rv;
    }
    return make_null();
}

// Main interpreter
int interpret(ASTNode* program) {
    if (!program || program->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid program\n");
        return 1;
    }

    interpreter_init_event_loop();

    // Pass 0: Process all imports first
    for (size_t i = 0; i < program->program.declarations->count; i++) {
        ASTNode* decl = program->program.declarations->nodes[i];
        if (decl->type == AST_IMPORT_STMT) {
            exec_statement(decl);
        }
    }

    // Pass 1: Register all functions, structs, and enums
    for (size_t i = 0; i < program->program.declarations->count; i++) {
        ASTNode* decl = program->program.declarations->nodes[i];
        if (decl->type == AST_FUNCTION_DECL) {
            exec_function(decl);
        } else if (decl->type == AST_STRUCT_DECL) {
            exec_statement(decl);  // This will call register_struct
        } else if (decl->type == AST_ENUM_DECL) {
            exec_statement(decl);  // This will call register_enum
        }
    }
    
    // Pass 2: Execute main function if it exists
    Value main_val = env_get("main");
    if (main_val.type == VAL_FUNCTION) {
        exec_statement(main_val.func_node->function_decl.body);
        value_free(&main_val);
    } else {
        // Fallback: search declarations (legacy)
        for (size_t i = 0; i < program->program.declarations->count; i++) {
            ASTNode* decl = program->program.declarations->nodes[i];
            if (decl->type == AST_FUNCTION_DECL && strcmp(decl->function_decl.name, "main") == 0) {
                exec_statement(decl->function_decl.body);
                break;
            }
        }
    }


    env_free();
    interpreter_cleanup_event_loop();
    return 0;
}

// REPL-specific interpreter - executes single statement without clearing environment
int interpret_repl_statement(ASTNode* stmt) {
    if (!stmt) {
        return 1;
    }

    // Execute the statement directly
    // Environment persists across calls for REPL
    exec_statement(stmt);
    return 0;
}

// Clean up the global environment (call when exiting REPL)
void interpreter_cleanup_environment(void) {
    env_free();
    env_free_structs();
    env_free_enums();
}
