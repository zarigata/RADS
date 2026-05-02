/*
 * stdlib_copy.c — RADS copy module
 *
 * Shallow and deep copy utilities for values.
 * Equivalent to Python's copy module.
 *
 * Functions:
 *   copy.copy(value)       — shallow copy
 *   copy.deepcopy(value)   — deep copy (recursive)
 *   copy.is_primitive(val) — check if value is a primitive (no need to copy)
 */

#include "stdlib_copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* Forward declaration for recursive deep copy */
static Value deepcopy_value(Value* v);

/* ── Shallow copy a single Value ── */
static Value shallowcopy_value(Value* v) {
    Value copy;
    copy.type = v->type;

    switch (v->type) {
        case VAL_STRING:
            copy.string_val = strdup(v->string_val);
            break;
        case VAL_ARRAY:
            /* Shallow: new array with same item references */
            {
                Array* src = v->array_val;
                Array* arr = array_create(src->count);
                for (size_t i = 0; i < src->count; i++) {
                    /* Copy Value struct but don't recurse — strings still get strdup'd */
                    Value item;
                    item.type = src->items[i].type;
                    if (item.type == VAL_STRING) {
                        item.string_val = strdup(src->items[i].string_val);
                    } else {
                        item = src->items[i];
                    }
                    array_push(arr, item);
                }
                copy.array_val = arr;
            }
            break;
        case VAL_STRUCT_INSTANCE:
            /* Shallow copy struct instance — share field pointers */
            copy.struct_instance = v->struct_instance;
            break;
        default:
            /* Primitives: int, float, bool, null — copy by value */
            copy = *v;
            break;
    }
    return copy;
}

/* ── Deep copy a single Value (recursive) ── */
static Value deepcopy_value(Value* v) {
    Value copy;
    copy.type = v->type;

    switch (v->type) {
        case VAL_STRING:
            copy.string_val = strdup(v->string_val);
            break;
        case VAL_ARRAY:
            {
                Array* src = v->array_val;
                Array* arr = array_create(src->count);
                for (size_t i = 0; i < src->count; i++) {
                    Value item = deepcopy_value(&src->items[i]);
                    array_push(arr, item);
                }
                copy.array_val = arr;
            }
            break;
        case VAL_STRUCT_INSTANCE:
            /* Deep copy struct instance by copying field values */
            {
                StructInstance* src = v->struct_instance;
                StructInstance* inst = malloc(sizeof(StructInstance));
                inst->definition = src->definition;  /* Share struct definition */
                inst->fields = NULL;

                /* Copy linked list of fields */
                FieldValue* prev = NULL;
                FieldValue* cur = src->fields;
                while (cur) {
                    FieldValue* fv = malloc(sizeof(FieldValue));
                    fv->name = strdup(cur->name);
                    fv->value = malloc(sizeof(Value));
                    *(fv->value) = deepcopy_value(cur->value);
                    fv->next = NULL;

                    if (!inst->fields) {
                        inst->fields = fv;
                    } else {
                        prev->next = fv;
                    }
                    prev = fv;
                    cur = cur->next;
                }
                copy.struct_instance = inst;
            }
            break;
        default:
            copy = *v;
            break;
    }
    return copy;
}

/* ── Public API ── */

Value stdlib_copy_copy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: copy.copy(value) requires one argument\n");
        return make_null();
    }
    return shallowcopy_value(&args[0]);
}

Value stdlib_copy_deepcopy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: copy.deepcopy(value) requires one argument\n");
        return make_null();
    }
    return deepcopy_value(&args[0]);
}

Value stdlib_copy_is_primitive(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_bool(false);
    switch (args[0].type) {
        case VAL_NULL:
        case VAL_BOOL:
        case VAL_INT:
        case VAL_FLOAT:
            return make_bool(true);
        default:
            return make_bool(false);
    }
}

void stdlib_copy_register(void) {
    register_native("copy.copy", (NativeFn)stdlib_copy_copy);
    register_native("copy.deepcopy", (NativeFn)stdlib_copy_deepcopy);
    register_native("copy.is_primitive", (NativeFn)stdlib_copy_is_primitive);
}
