#define _POSIX_C_SOURCE 200809L
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> /* fmod for float modulo in binary ops */
#include <ctype.h>  /* isdigit for float-format normalization */
#include <signal.h>

static Value eval_expression(ASTNode* node);
/* forward decl: value_print (line ~230) renders floats before the full
 * definition below (line ~734) — keeps -Wimplicit-decl off our back */
static const char* format_float_shortest(double v, char* buf, size_t buflen);
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

/* Canonical error return for native modules: say what's wrong in a human
 * voice, then hand back null so the script continues or the caller checks.
 * Extending: add a VAL_ERROR tag someday if scripts need to test for it. */
Value make_error(const char* msg) {
    fprintf(stderr, "Error: %s\n", msg);
    return make_null();
}

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
            ASTNode* node = elements->nodes[i];
            if (node->type == AST_SPREAD_EXPR) {
                Value spread_val = eval_expression(node->spread_expr.expression);
                if (spread_val.type == VAL_ARRAY && spread_val.array_val) {
                    for (size_t j = 0; j < spread_val.array_val->count; j++) {
                        array_push(arr, spread_val.array_val->items[j]);
                    }
                }
                value_release(&spread_val);
            } else {
                Value ev = eval_expression(node);
                array_push(arr, ev);
                value_release(&ev);
            }
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
        case VAL_ITERATOR:
            if (v.iterator_val) v.iterator_val->refcount++;
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
        case VAL_ITERATOR:
            if (value->iterator_val) {
                Iterator* it = value->iterator_val;
                if (it->refcount > 0) it->refcount--;
                if (it->refcount == 0) {
                    if (it->vtable && it->vtable->cleanup) it->vtable->cleanup(it);
                    free(it);
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
        case VAL_FLOAT: {
            /* Python parity: shortest round-trip, ".0" on integral floats
             * ("%f" padded 2.5 to "2.500000" — fixed in nightshift 20261029) */
            char fbuf[64];
            printf("%s", format_float_shortest(value->float_val, fbuf, sizeof(fbuf)));
            break;
        }
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
        case VAL_ITERATOR:
            printf("<iterator>");
            break;
        case VAL_STRUCT_DEF:
            printf("<struct def %s>", value->struct_def->name);
            break;
        case VAL_STRUCT_INSTANCE:
            /* Anonymous instances (built by natives like regex.search) carry a
             * NULL definition — printing must not dereference it. */
            if (value->struct_instance->definition) {
                printf("<struct instance %s>", value->struct_instance->definition->name);
            } else {
                printf("<struct instance>");
            }
            break;
    }
}

void value_free(Value* value) { value_release(value); }

// Public wrapper so stdlib natives can safely return/alias arg memory:
// the interpreter frees the args array right after a native call, so any
// Value handed back must own its payload (strdup for strings, refcount++ for
// arrays, deep-copy for struct instances). Never return args[i] raw.
Value value_retain(Value v) { return value_clone(v); }

// --- Lazy iterator runtime (nightshift 20261025) ---
// Constructor: caller hands a malloc'd flavor state; iterator owns it and
// frees it via vtable->cleanup when the last reference drops.
Value make_iterator(const IteratorVTable* vtable, void* state) {
    Iterator* it = malloc(sizeof(Iterator));
    it->refcount = 1;
    it->vtable = vtable;
    it->state = state;
    Value v;
    v.type = VAL_ITERATOR;
    v.iterator_val = it;
    return v;
}

// Pull one OWNED value; false = exhausted (sticky) or not an iterator.
bool iterator_next(Value* itv, Value* out) {
    if (!itv || itv->type != VAL_ITERATOR || !itv->iterator_val || !out) return false;
    Iterator* it = itv->iterator_val;
    if (!it->vtable || !it->vtable->next) return false;
    return it->vtable->next(it, out);
}

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

/* env_remove — drop a binding created inside a finished call frame (param
 * that had no caller binding). Why: without it, calling a function would
 * permanently litter the global env with its parameter names. */
static void env_remove(const char* name) {
    Environment** link = &global_env;
    while (*link) {
        if (strcmp((*link)->name, name) == 0) {
            Environment* dead = *link;
            *link = dead->next;
            free(dead->name);
            value_free(&dead->value);
            free(dead);
            return;
        }
        link = &(*link)->next;
    }

}

/* ===== Scoped call frames (nightshift 20261026) =====
 * Why: parameters and locals bound to the single global environment leaked
 *      into the caller's scope — worse, RECURSION was broken: an inner call
 *      rebound the outer frame's n mid-expression (fib(10) returned -80).
 *      Fix: each call pushes a frame of shadowed bindings; on return the
 *      old values are restored. Locals still live in the global list (the
 *      prototype's architecture), but a call can no longer vandalize the
 *      caller's variables.
 * Extend: replace with a real frame chain when closures arrive; the
 *      save/restore protocol here is the migration seam. */

typedef struct EnvShadow {
    char* name;          /* variable name as written in env (owned) */
    bool existed;        /* false = binding created by the callee, remove it */
    Value saved;         /* the caller's value (owned clone) */
    struct EnvShadow* next;
} EnvShadow;

/* frame_push_param — record the caller's binding for one param name before
 * the callee overwrites it, so frame_pop can restore it later. */
static void frame_push_param(EnvShadow** frame, const char* pname) {
    EnvShadow* sh = malloc(sizeof(EnvShadow));
    sh->name = strdup(pname);
    Environment* binding = NULL;
    for (Environment* cur = global_env; cur; cur = cur->next) {
        if (strcmp(cur->name, pname) == 0) { binding = cur; break; }
    }
    if (binding) {
        sh->existed = true;
        sh->saved = value_clone(binding->value);
    } else {
        sh->existed = false;
        sh->saved = make_null();
    }
    sh->next = *frame;
    *frame = sh;
}

/* frame_pop — restore all shadowed bindings in reverse order (LIFO), so
 * nested calls with the same param name unwind correctly. */
static void frame_pop(EnvShadow* frame) {
    while (frame) {
        EnvShadow* next = frame->next;
        if (frame->existed) {
            env_set(frame->name, frame->saved);
        } else {
            env_remove(frame->name);
        }
        free(frame->name);
        value_free(&frame->saved);
        free(frame);
        frame = next;
    }
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

void register_native_fn(const char* module, const char* name, NativeFn fn) {
    size_t mod_len = strlen(module);
    size_t name_len = strlen(name);
    char* full_name = malloc(mod_len + 1 + name_len + 1);
    memcpy(full_name, module, mod_len);
    full_name[mod_len] = '.';
    memcpy(full_name + mod_len + 1, name, name_len + 1);
    register_native(full_name, fn);
    free(full_name);
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

/* --- Interned StructDef table (backlog 3b) -------------------------------
 * Eval of a struct literal used to malloc a fresh StructDef + strdup the
 * name for EVERY instance; value_free never released them (clones share
 * the pointer, so freeing per-instance would double-free). Interning gives
 * one immortal-per-program StructDef per struct NAME -- same ownership
 * model the stdlib already uses (datetime_def() et al. are static structs)
 * -- and interpreter_cleanup_environment() frees the table at teardown.
 */
typedef struct InternedStructDef {
    StructDef def;
    struct InternedStructDef* next;
} InternedStructDef;

static InternedStructDef* interned_struct_defs = NULL;

static StructDef* struct_def_intern(const char* name, ASTNode* ast_node) {
    for (InternedStructDef* it = interned_struct_defs; it; it = it->next) {
        if (strcmp(it->def.name, name) == 0) return &it->def;
    }
    InternedStructDef* entry = malloc(sizeof(InternedStructDef));
    entry->def.name = strdup(name);
    entry->def.ast_node = ast_node;
    entry->next = interned_struct_defs;
    interned_struct_defs = entry;
    return &entry->def;
}

static void interned_struct_defs_free(void) {
    InternedStructDef* it = interned_struct_defs;
    while (it) {
        InternedStructDef* next = it->next;
        free(it->def.name);
        free(it);
        it = next;
    }
    interned_struct_defs = NULL;
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
/* value_to_owned_string — convert any Value to a malloc'd string with the
 * SAME semantics the + operator used (int %lld, float %g, bool true/false,
 * string verbatim, arrays as [a, b, c], else "null"). Replaces the old
 * 256-byte stack buffers in OP_ADD that silently clipped long values and
 * could overrun on array joins — one dynamic home for the conversion now.
 * Contract: caller owns the returned pointer; never NULL ("" on OOM). */
/* format_float_shortest — Python/TS parity float rendering.
 * Why: %g (6 sig digits) display-rounded 1000001.0 to "1e+06" and %f padded
 *      "2.5" to "2.500000". Python/JS print the SHORTEST string that
 *      round-trips to the same double, with ".0" on integral floats and
 *      scientific notation only in Python's exponent window (|x|>=1e16 or
 *      <1e-4). One home so value_print, concat, and templates never drift.
 * How to upgrade: if locale-aware decimal commas ever matter, swap snprintf
 *      for the C locale equivalents; if float128 arrives, widen the buffer.
 */
static const char* format_float_shortest(double v, char* buf, size_t buflen) {
    if (v != v) { snprintf(buf, buflen, "nan"); return buf; }
    if (v == INFINITY) { snprintf(buf, buflen, "inf"); return buf; }
    if (v == -INFINITY) { snprintf(buf, buflen, "-inf"); return buf; }
    if (v == 0.0) { snprintf(buf, buflen, "%s", signbit(v) ? "-0.0" : "0.0"); return buf; }

    int dec_exp = (int)floor(log10(fabs(v)));  /* floor(log10|x|), v!=0 here */
    bool sci = (dec_exp >= 16 || dec_exp < -4);  /* CPython repr window */

    if (sci) {
        /* %.*e is ALWAYS scientific with signed 2-digit-min exponent, exactly
         * CPython's style; increase digits-after-point until it round-trips */
        for (int p = 0; p <= 16; p++) {
            snprintf(buf, buflen, "%.*e", p, v);
            char* e = strchr(buf, 'e');
            char* end = e;
            while (end > buf && end[-1] == '0') end--;  /* mantissa zeros */
            if (end > buf && end[-1] == '.') end--;     /* trailing dot */
            if (end < e) memmove(end, e, strlen(e) + 1);
            if (strtod(buf, NULL) == v) return buf;
        }
        snprintf(buf, buflen, "%.16e", v);  /* unreachable safety net */
        return buf;
    }

    /* fixed window: %.*g flips to sci for small P — skip wrong-form tries */
    int first_p = dec_exp + 1;
    if (first_p < 1) first_p = 1;
    for (int P = first_p; P <= 17; P++) {
        snprintf(buf, buflen, "%.*g", P, v);
        if (strchr(buf, 'e')) continue;         /* wrong form for this window */
        if (strtod(buf, NULL) == v) break;
    }
    if (!strchr(buf, '.')) {
        /* Python prints 100.0, not 100 — integral floats keep ".0" */
        size_t len = strlen(buf);
        if (len + 3 <= buflen) { buf[len] = '.'; buf[len+1] = '0'; buf[len+2] = '\0'; }
    }
    return buf;
}

static char* value_to_owned_string(Value v) {
    switch (v.type) {
        case VAL_STRING:
            return strdup(v.string_val ? v.string_val : "");
        case VAL_INT: {
            int needed = snprintf(NULL, 0, "%lld", v.int_val);
            char* out = malloc((size_t)needed + 1);
            if (out) snprintf(out, (size_t)needed + 1, "%lld", v.int_val);
            return out;
        }
        case VAL_FLOAT: {
            char fbuf[64];
            format_float_shortest(v.float_val, fbuf, sizeof(fbuf));
            return strdup(fbuf);
        }
        case VAL_BOOL:
            return strdup(v.bool_val ? "true" : "false");
        case VAL_NULL:
            return strdup("null");
        case VAL_ARRAY: {
            if (!v.array_val) return strdup("[]");
            /* sizing pass: sum worst-case renders + separator slack */
            size_t cap = 8;
            for (size_t i = 0; i < v.array_val->count; i++) {
                Value* item = &v.array_val->items[i];
                if (item->type == VAL_STRING && item->string_val)
                    cap += strlen(item->string_val);
                else if (item->type == VAL_ARRAY || item->type == VAL_STRUCT_INSTANCE)
                    cap += 64;
                else
                    cap += 32;
                cap += 4;
            }
            char* out = malloc(cap);
            if (!out) return strdup("[]");
            size_t pos = 0;
            out[pos++] = '[';
            for (size_t i = 0; i < v.array_val->count; i++) {
                if (i > 0) { out[pos++] = ','; out[pos++] = ' '; }
                char* item = value_to_owned_string(v.array_val->items[i]);
                size_t ilen = item ? strlen(item) : 0;
                if (pos + ilen + 2 > cap) ilen = cap - pos - 2; /* belt+braces */
                memcpy(out + pos, item, ilen);
                pos += ilen;
                free(item);
            }
            out[pos++] = ']';
            out[pos] = '\0';
            return out;
        }
        default:
            return strdup("null");
    }
}

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
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_float((double)left.int_val + right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_float(left.float_val + (double)right.int_val);
            } else if (left.type == VAL_STRING || right.type == VAL_STRING) {
                /* String concatenation with automatic conversion — dynamic and
                 * overflow-proof (the old 256-byte stack buffers clipped long
                 * values; template literals made this path hot, so it must be
                 * correct for any length). */
                char* ls = value_to_owned_string(left);
                char* rs = value_to_owned_string(right);
                if (ls && rs) {
                    size_t len = strlen(ls) + strlen(rs) + 1;
                    char* str = malloc(len);
                    if (str) {
                        strcpy(str, ls);
                        strcat(str, rs);
                        result = make_string(str);
                        free(str);
                    }
                }
                free(ls);
                free(rs);
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
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_float(left.float_val - right.float_val);
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_float((double)left.int_val - right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_float(left.float_val - (double)right.int_val);
            }
            break;
        case OP_MUL:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val * right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_float(left.float_val * right.float_val);
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_float((double)left.int_val * right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_float(left.float_val * (double)right.int_val);
            }
            break;
        case OP_DIV:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_int(left.int_val / right.int_val);
                }
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                if (right.float_val != 0.0) {
                    result = make_float(left.float_val / right.float_val);
                }
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                if (right.float_val != 0.0) {
                    result = make_float((double)left.int_val / right.float_val);
                }
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_float(left.float_val / (double)right.int_val);
                }
            }
            break;
        case OP_MOD:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_int(left.int_val % right.int_val);
                }
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                if (right.float_val != 0.0) {
                    result = make_float(fmod(left.float_val, right.float_val));
                }
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                if (right.float_val != 0.0) {
                    result = make_float(fmod((double)left.int_val, right.float_val));
                }
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_float(fmod(left.float_val, (double)right.int_val));
                }
            }
            break;
        case OP_EQ:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val == right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val == right.float_val);
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_bool((double)left.int_val == right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_bool(left.float_val == (double)right.int_val);
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
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_bool((double)left.int_val != right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_bool(left.float_val != (double)right.int_val);
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
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_bool((double)left.int_val < right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_bool(left.float_val < (double)right.int_val);
            }
            break;
        case OP_LTE:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val <= right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val <= right.float_val);
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_bool((double)left.int_val <= right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_bool(left.float_val <= (double)right.int_val);
            }
            break;
        case OP_GT:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val > right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val > right.float_val);
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_bool((double)left.int_val > right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_bool(left.float_val > (double)right.int_val);
            }
            break;
        case OP_GTE:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_bool(left.int_val >= right.int_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                result = make_bool(left.float_val >= right.float_val);
            } else if (left.type == VAL_INT && right.type == VAL_FLOAT) {
                result = make_bool((double)left.int_val >= right.float_val);
            } else if (left.type == VAL_FLOAT && right.type == VAL_INT) {
                result = make_bool(left.float_val >= (double)right.int_val);
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
                    /* UAF fix (nightshift 20261027): the old code shallow-copied
                     * struct instances (and ignored iterator refcounts) and then
                     * value_free(&arr) below released the array, freeing the very
                     * struct this result points at — `turbo p = arr[0]; p.x`
                     * segfaulted. value_clone does the right thing for every
                     * type: strdup strings, refcount++ arrays/iterators, deep
                     * copy struct instances, trivial copy scalars. */
                    result = value_clone(arr.array_val->items[idx.int_val]);
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
                case VAL_ITERATOR: type_str = "iterator"; break;
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
                const char* sname = object.struct_instance->definition
                                      ? object.struct_instance->definition->name : "<anonymous>";
                fprintf(stderr, "Error: Struct '%s' has no member '%s'.\n", sname, member_name);
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
                                value_release(current->value); /* free old contents... */
                                free(current->value);           /* ...and the box itself (was leaked) */
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
                                value_release(current->value); /* free old contents... */
                                free(current->value);           /* ...and the box itself (was leaked) */
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
            instance->definition = struct_def_intern(node->struct_literal.name, def_node);
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

        case AST_OPTIONAL_CHAIN: {
            Value object = eval_expression(node->optional_chain.object);
            if (object.type == VAL_NULL) {
                return make_null();
            }
            
            Value result = make_null();
            if (node->optional_chain.is_member) {
                if (object.type == VAL_STRUCT_INSTANCE) {
                    const char* member_name = node->optional_chain.member;
                    FieldValue* current = object.struct_instance->fields;
                    while (current) {
                        if (strcmp(current->name, member_name) == 0) {
                            result = value_clone(*current->value);
                            break;
                        }
                        current = current->next;
                    }
                } else if (object.type == VAL_ARRAY) {
                    if (strcmp(node->optional_chain.member, "length") == 0) {
                        result = make_int((long long)object.array_val->count);
                    }
                }
            } else {
                Value idx = eval_expression(node->optional_chain.index);
                if (object.type == VAL_ARRAY && idx.type == VAL_INT) {
                    if (idx.int_val >= 0 && (size_t)idx.int_val < object.array_val->count) {
                        result = value_clone(object.array_val->items[idx.int_val]);
                    }
                }
                value_free(&idx);
            }
            value_free(&object);
            return result;
        }

        case AST_NULLISH_COALESCING: {
            Value left = eval_expression(node->nullish_coalescing.left);
            if (left.type != VAL_NULL) {
                return left;
            }
            value_free(&left);
            return eval_expression(node->nullish_coalescing.right);
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
static Value thrown_value;
static bool has_thrown_value = false;

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
        case VAL_ITERATOR:
        case VAL_NULL:
        default:
            return false;
    }
}

// Execute statement
static ExecResult exec_statement(ASTNode* node) {
    if (!node) return EXEC_OK;

    /* Throw barrier: an exception in flight from a previous statement (e.g. a
     * bare call to a throwing function) unwinds from the next statement on.
     * Try statements are exempt — their block handles entry with pending state
     * by catching it, which keeps `boom(); try {...} catch` coherent. */
    if (has_thrown_value && node->type != AST_TRY_STMT) {
        return EXEC_THROW;
    }

    switch (node->type) {
        case AST_FUNCTION_DECL:
            /* Nested `blast` inside another function body: register it in the
             * global env instead of letting it fall to `default:` where the
             * VAL_FUNCTION value was evaluated and discarded. Local scoping
             * (shadowing an outer name) is future work with closures; this
             * restores the documented semantics that a declared function is
             * callable for the rest of the program. */
            exec_function(node);
            return EXEC_OK;

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
                if (r == EXEC_BREAK || r == EXEC_CONTINUE || r == EXEC_RETURN || r == EXEC_THROW) {
                    return r;
                }
                /* Throw barrier (nightshift 20261027): a callee's throw leaves
                 * pending state even when the call expression itself returned
                 * EXEC_OK — unwind here so enclosing try/catch can see it. */
                if (has_thrown_value) {
                    return EXEC_THROW;
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
                if (r == EXEC_THROW) {
                    return EXEC_THROW;
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
                    if (r == EXEC_RETURN) {
                        return EXEC_RETURN;
                    }
                    if (r == EXEC_THROW) {
                        return EXEC_THROW;
                    }
                }
            }
            // General path (nightshift 20261025): evaluate the iterable once and
            // drive any array or lazy iterator value. The old code only handled
            // the `a..b` range op — `cruise (x in [1,2,3])` silently ran zero
            // times. Arrays keep snapshot semantics (count fixed at loop start).
            {
                Value iterable = eval_expression(iter);
                ExecResult r = EXEC_OK;
                if (iterable.type == VAL_ARRAY && iterable.array_val) {
                    size_t n = iterable.array_val->count;
                    for (size_t i = 0; i < n; i++) {
                        env_set(node->cruise_stmt.iterator, iterable.array_val->items[i]); /* env_set clones */
                        r = exec_statement(node->cruise_stmt.body);
                        if (r == EXEC_BREAK) { r = EXEC_OK; break; }
                        if (r == EXEC_RETURN || r == EXEC_THROW) break;
                    }
                } else if (iterable.type == VAL_ITERATOR && iterable.iterator_val) {
                    Value item;
                    while (iterator_next(&iterable, &item)) {
                        env_set(node->cruise_stmt.iterator, item); /* env owns it now */
                        r = exec_statement(node->cruise_stmt.body);
                        value_free(&item);
                        if (r == EXEC_BREAK) { r = EXEC_OK; break; }
                        if (r == EXEC_RETURN || r == EXEC_THROW) break;
                     }
                }
                value_free(&iterable);
                return r;
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
        
        case AST_THROW_STMT: {
            Value tv = make_null();
            if (node->throw_stmt.expression) {
                tv = eval_expression(node->throw_stmt.expression);
            }
            if (has_thrown_value) {
                value_free(&thrown_value);
            }
            thrown_value = value_clone(tv);
            has_thrown_value = true;
            value_free(&tv);
            return EXEC_THROW;
        }
        
        case AST_TRY_STMT: {
            ExecResult r = exec_statement(node->try_stmt.try_block);
            
            if (r == EXEC_THROW && node->try_stmt.catch_block) {
                if (has_thrown_value) {
                    if (node->try_stmt.catch_var) {
                        env_set(node->try_stmt.catch_var, thrown_value);
                    }
                    value_free(&thrown_value);
                    has_thrown_value = false;
                }
                r = exec_statement(node->try_stmt.catch_block);
            }
            
            if (node->try_stmt.finally_block) {
                /* Park an in-flight throw so the finally body actually runs
                 * (the statement-level barrier would skip it otherwise). */
                bool parked_pending = has_thrown_value;
                Value parked_value = thrown_value;
                if (parked_pending) {
                    has_thrown_value = false;
                }
                ExecResult fr = exec_statement(node->try_stmt.finally_block);
                if (parked_pending) {
                    if (has_thrown_value) {
                        /* TS semantics: a throw from finally REPLACES the
                         * in-flight exception — the newer one wins. */
                        value_free(&parked_value);
                    } else {
                        thrown_value = parked_value;
                        has_thrown_value = true;
                    }
                }
                if (fr == EXEC_RETURN) {
                    return EXEC_RETURN;
                }
                if (fr == EXEC_THROW) {
                    return EXEC_THROW;
                }
            }
            
            return r;
        }
        
        case AST_VARIABLE_DECL: {
            Value val = make_null();
            if (node->variable_decl.initializer) {
                val = eval_expression(node->variable_decl.initializer);
            }
            
            if (node->variable_decl.destructure_pattern) {
                ASTNode* pattern = node->variable_decl.destructure_pattern;
                if (pattern->type == AST_DESTRUCTURE_ARRAY) {
                    if (val.type != VAL_ARRAY || !val.array_val) {
                        fprintf(stderr, "Error: Cannot destructure non-array value\n");
                    } else {
                        Array* arr = val.array_val;
                        ASTList* elements = pattern->destructure_array.elements;
                        size_t array_index = 0;
                        
                        for (size_t i = 0; i < elements->count; i++) {
                            ASTNode* elem = elements->nodes[i];
                            
                            if (elem->type == AST_DESTRUCTURE_REST) {
                                Array* rest_arr = array_create(arr->count - array_index);
                                while (array_index < arr->count) {
                                    array_push(rest_arr, arr->items[array_index]);
                                    array_index++;
                                }
                                Value rest_val;
                                rest_val.type = VAL_ARRAY;
                                rest_val.array_val = rest_arr;
                                env_set(elem->destructure_rest.name, rest_val);
                                value_free(&rest_val);
                            } else if (elem->type == AST_IDENTIFIER) {
                                if (array_index < arr->count) {
                                    env_set(elem->identifier.name, arr->items[array_index]);
                                } else {
                                    env_set(elem->identifier.name, make_null());
                                }
                                array_index++;
                            }
                        }
                    }
                } else if (pattern->type == AST_DESTRUCTURE_STRUCT) {
                    if (val.type != VAL_STRUCT_INSTANCE || !val.struct_instance) {
                        fprintf(stderr, "Error: Cannot destructure non-struct value\n");
                    } else {
                        StructInstance* instance = val.struct_instance;
                        ASTList* fields = pattern->destructure_struct.fields;
                        
                        for (size_t i = 0; i < fields->count; i++) {
                            ASTNode* field_assign = fields->nodes[i];
                            if (field_assign->type == AST_ASSIGN_EXPR) {
                                const char* field_name = field_assign->assign_expr.target->identifier.name;
                                const char* var_name = field_assign->assign_expr.value->identifier.name;
                                
                                FieldValue* fv = instance->fields;
                                while (fv) {
                                    if (strcmp(fv->name, field_name) == 0) {
                                        env_set(var_name, *fv->value);
                                        break;
                                    }
                                    fv = fv->next;
                                }
                                if (!fv) {
                                    env_set(var_name, make_null());
                                }
                            }
                        }
                    }
                }
            } else if (node->variable_decl.name) {
                env_set(node->variable_decl.name, val);
            }
            value_free(&val);
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
    /* Scoped call frames (nightshift 20261026): save each param's caller
     * binding, bind for the callee, restore on exit — fixes param leakage
     * into the caller AND makes recursion correct (inner calls no longer
     * clobber the outer frame's bindings mid-expression). */
    EnvShadow* frame = NULL;
    if (func->function_decl.parameters) {
        int param_count = (int)func->function_decl.parameters->count;
        for (int i = 0; i < param_count; i++) {
            ASTNode* param_node = func->function_decl.parameters->nodes[i];
            const char* pname;
            Value arg;

            if (param_node->type == AST_VARIABLE_DECL) {
                pname = param_node->variable_decl.name;
                if (i < argc && args) {
                    arg = value_clone(args[i]);
                } else if (param_node->variable_decl.initializer) {
                    arg = eval_expression(param_node->variable_decl.initializer);
                } else {
                    arg = make_null();
                }
            } else {
                pname = param_node->identifier.name;
                arg = (i < argc && args) ? value_clone(args[i]) : make_null();
            }
            frame_push_param(&frame, pname);
            env_set(pname, arg);
            value_free(&arg);
        }
    }
    ExecResult r = exec_statement(func->function_decl.body);
    frame_pop(frame);
    if (r == EXEC_RETURN && has_return_value) {
        Value rv = value_clone(current_return_value);
        value_free(&current_return_value);
        has_return_value = false;
        return rv;
    }
    return make_null();
}

/* Report an exception that reached the top of the program without a
 * matching catch — Python/TS parity: message on stderr, nonzero exit. */
static void report_uncaught_throw(void) {
    fprintf(stderr, "Uncaught");
    switch (thrown_value.type) {
        case VAL_STRING: fprintf(stderr, " error: %s", thrown_value.string_val); break;
        case VAL_INT:    fprintf(stderr, " error: %lld", thrown_value.int_val); break;
        case VAL_FLOAT: {
            /* shortest round-trip like echo/concat (was %f padding) */
            char fbuf[64];
            fprintf(stderr, " error: %s", format_float_shortest(thrown_value.float_val, fbuf, sizeof(fbuf)));
            break;
        }
        case VAL_BOOL:   fprintf(stderr, " error: %s", thrown_value.bool_val ? "true" : "false"); break;
        default:         fprintf(stderr, " error: <value>"); break;
    }
    fprintf(stderr, "\n");
    value_free(&thrown_value);
    has_thrown_value = false;
}

/* Peek: is an exception currently in flight? (Promise natives need to
 * distinguish "nothing thrown" from "null was thrown" before taking.) */
bool interpreter_has_throw(void) {
    return has_thrown_value;
}

/* Take ownership of a pending throw (if any): returns a clone of the thrown
 * value and clears the pending state. Natives like Promise.allSettled use
 * this to convert a callback's throw into a rejection record. */
Value interpreter_take_throw(void) {
    if (!has_thrown_value) {
        return make_null();
    }
    Value out = value_clone(thrown_value);
    value_free(&thrown_value);
    has_thrown_value = false;
    return out;
}

/* Raise a value as an in-flight exception from native code (e.g. Promise.all
 * re-raising the first rejection so the caller's try/catch sees it). */
void interpreter_raise(Value v) {
    if (has_thrown_value) {
        value_free(&thrown_value);
    }
    thrown_value = value_clone(v);
    has_thrown_value = true;
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
        if (has_thrown_value) {
            report_uncaught_throw();
            interpreter_cleanup_environment();
            interpreter_cleanup_event_loop();
            return 1; /* Python/Node parity: uncaught exception -> nonzero exit */
        }
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


    /* Full teardown on the file path too: env + struct/enum registries +
     * interned StructDefs. Previously only env_free() ran here, so every
     * file run leaked the struct/enum binding tables (LSan-visible). */
    interpreter_cleanup_environment();
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
    /* A throw the user never caught must not poison the next REPL line —
     * report it and clear, the way Python/Node REPLs do. */
    if (has_thrown_value) {
        report_uncaught_throw();
    }
    return 0;
}

// Clean up the global environment (call when exiting REPL)
void interpreter_cleanup_environment(void) {
    env_free();
    interned_struct_defs_free();
    env_free_structs();
    env_free_enums();
}
