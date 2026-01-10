#define _POSIX_C_SOURCE 200809L
#include "interpreter.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static Value eval_expression(ASTNode* node);
static ExecResult exec_statement(ASTNode* node);
static void exec_function(ASTNode* node);

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

// Value functions
static Value make_null() {
    Value v;
    v.type = VAL_NULL;
    return v;
}

static Value make_bool(bool val) { Value v = { .type = VAL_BOOL, .bool_val = val }; return v; }
static Value make_int(long long val) { Value v = { .type = VAL_INT, .int_val = val }; return v; }
static Value make_float(double val) { Value v = { .type = VAL_FLOAT, .float_val = val }; return v; }
static Value make_string(const char* val) { Value v = { .type = VAL_STRING, .string_val = strdup(val) }; return v; }

static Array* array_create(size_t capacity) {
    Array* arr = malloc(sizeof(Array));
    arr->refcount = 1;
    arr->count = 0;
    arr->capacity = capacity > 0 ? capacity : 4;
    arr->items = calloc(arr->capacity, sizeof(Value));
    return arr;
}

static Value value_clone(Value v);
static void value_release(Value* v);

static void array_push(Array* arr, Value v) {
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
            printf("<blast function %s>", value->func_node->function_decl.name);
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
    // Check existing
    Environment* current = global_env;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            value_free(&current->value); // Free old value
            current->value = value;
            // Deep copy string if needed? Value semantics usually mean ownership transfer or copy
            // Here assuming value copy
            if (value.type == VAL_STRING) {
                current->value.string_val = strdup(value.string_val); // Copy for storage
            } else if (value.type == VAL_FUNCTION) {
                current->value.func_node = value.func_node;
            }
            return;
        }
        current = current->next;
    }
    
    // Create new
    Environment* env = malloc(sizeof(Environment));
    env->name = strdup(name);
    env->value = value;
     if (value.type == VAL_STRING) {
        env->value.string_val = strdup(value.string_val);
    } else if (value.type == VAL_FUNCTION) {
        env->value.func_node = value.func_node;
    }
    env->next = global_env;
    global_env = env;
}

static Value env_get(const char* name) {
    Environment* current = global_env;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            Value v = current->value;
            // Return copy
            if (v.type == VAL_STRING) {
                v.string_val = strdup(v.string_val);
            }
            // Functions are nodes, we assume they live as long as the AST
            return v;
        }
        current = current->next;
    }
    return make_null();
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



// Evaluate call expression
static Value eval_call(ASTNode* node) {
    // For now, only support calling global native functions or simple function calls
    // Real implementation would need full environment/scope support
    
    char* name = NULL;
    char* full_name = NULL;
    
    // Member expression handling: allow dispatch of methods on string handles (e.g., server.route)
    if (node->call_expr.callee->type == AST_MEMBER_EXPR) {
        ASTNode* obj = node->call_expr.callee->member_expr.object;
        const char* member = node->call_expr.callee->member_expr.member;
        // Attempt to dispatch to native function with implicit handle argument
        Value obj_val = eval_expression(obj);
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
            ASTNode* func = func_val.func_node;
            // Simplified execution: no args for now in prototype user calls
            // Real version would need to bind arguments to parameters and support returns
            exec_statement(func->function_decl.body);
            value_free(&func_val);
            if (full_name) free(full_name);
            return make_null(); 
        }
        value_free(&func_val);

        if (full_name) free(full_name);
    }
    
    // Fallback for user functions (not fully implemented in this prototype)
    return make_null();
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
            } else if (left.type == VAL_STRING && right.type == VAL_STRING) {
                size_t len = strlen(left.string_val) + strlen(right.string_val) + 1;
                char* str = malloc(len);
                strcpy(str, left.string_val);
                strcat(str, right.string_val);
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
        
        case AST_BINARY_OP:
            return eval_binary_op(node);
            
        case AST_CALL_EXPR:
            return eval_call(node);
            
        case AST_IDENTIFIER:
            return env_get(node->identifier.name);
        
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
        case AST_ECHO_STMT:
            exec_echo(node);
            return EXEC_OK;
        
        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statements->count; i++) {
                ExecResult r = exec_statement(node->block.statements->nodes[i]);
                if (r == EXEC_BREAK || r == EXEC_CONTINUE || r == EXEC_RETURN) {
                    return r;
                }
            }
            return EXEC_OK;
            
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
    
    // Pass 1: Register all functions
    for (size_t i = 0; i < program->program.declarations->count; i++) {
        ASTNode* decl = program->program.declarations->nodes[i];
        if (decl->type == AST_FUNCTION_DECL) {
            exec_function(decl);
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
}
