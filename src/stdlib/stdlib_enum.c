/*
 * stdlib_enum.c — RADS enum module
 *
 * Enumeration type support.
 * Equivalent to Python's enum module (simplified for RADS).
 *
 * Functions:
 *   enum.create(name, members)     — create an enum from name + array of member names
 *   enum.create_values(name, map)  — create an enum with explicit values
 *   enum.value(enum_val, name)     — get member by name from enum instance
 *   enum.name(enum_val)            — get the name of an enum value
 *   enum.names(enum_val)           — get all member names
 *   enum.values(enum_val)          — get all member values
 *   enum.has(enum_val, name)       — check if member exists
 *   enum.count(enum_val)           — number of members
 *   enum.from_value(enum_val, val) — lookup member by integer value
 *   enum.to_array(enum_val)        — convert to array of [name, value] pairs
 *
 * Design:
 *   Enums are represented as struct instances with:
 *   - Field "_type" = "enum"
 *   - Field "_name" = enum type name
 *   - Field "_count" = number of members
 *   - One field per member: name -> int value
 *
 * Zero external dependencies.
 */

#include "stdlib_enum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* ── Helper: create a StructInstance with given fields ── */
static StructInstance* create_struct_instance(void) {
    StructInstance* inst = malloc(sizeof(StructInstance));
    inst->definition = NULL;
    inst->fields = NULL;
    return inst;
}

static void inst_set_field(StructInstance* inst, const char* name, Value val) {
    FieldValue* fv = malloc(sizeof(FieldValue));
    fv->name = strdup(name);
    fv->value = malloc(sizeof(Value));
    *(fv->value) = val;
    fv->next = NULL;

    /* Append to end */
    if (!inst->fields) {
        inst->fields = fv;
    } else {
        FieldValue* cur = inst->fields;
        while (cur->next) cur = cur->next;
        cur->next = fv;
    }
}

static Value make_struct_val(StructInstance* inst) {
    Value v;
    v.type = VAL_STRUCT_INSTANCE;
    v.struct_instance = inst;
    return v;
}

static FieldValue* find_field(StructInstance* inst, const char* name) {
    FieldValue* cur = inst->fields;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

static StructInstance* get_enum_instance(Value* args, int argc, int idx) {
    if (idx >= argc || args[idx].type != VAL_STRUCT_INSTANCE) return NULL;
    StructInstance* inst = args[idx].struct_instance;
    /* Verify it has _type = "enum" */
    FieldValue* ft = find_field(inst, "_type");
    if (!ft || ft->value->type != VAL_STRING ||
        strcmp(ft->value->string_val, "enum") != 0) return NULL;
    return inst;
}

/* ── Public API ── */

/* enum.create(name, members) — auto-assign 0, 1, 2... */
Value stdlib_enum_create(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_ARRAY) {
        fprintf(stderr, "Error: enum.create(name, members_array) required\n");
        return make_null();
    }

    StructInstance* inst = create_struct_instance();
    inst_set_field(inst, "_type", make_string("enum"));
    inst_set_field(inst, "_name", make_string(args[0].string_val));

    Array* members = args[1].array_val;
    long long count = (long long)members->count;
    inst_set_field(inst, "_count", make_int(count));

    for (long long i = 0; i < count; i++) {
        if (members->items[i].type != VAL_STRING) {
            fprintf(stderr, "Error: enum member names must be strings\n");
            continue;
        }
        inst_set_field(inst, members->items[i].string_val, make_int(i));
    }

    return make_struct_val(inst);
}

/* enum.create_values(name, map) — explicit values from struct { member: value, ... } */
Value stdlib_enum_create_values(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRUCT_INSTANCE) {
        fprintf(stderr, "Error: enum.create_values(name, struct_map) required\n");
        return make_null();
    }

    StructInstance* inst = create_struct_instance();
    inst_set_field(inst, "_type", make_string("enum"));
    inst_set_field(inst, "_name", make_string(args[0].string_val));

    StructInstance* src = args[1].struct_instance;
    long long count = 0;
    FieldValue* cur = src->fields;
    while (cur) {
        /* Skip internal fields from source if any */
        if (cur->name[0] != '_') {
            Value v;
            if (cur->value->type == VAL_INT) {
                v = *cur->value;
            } else if (cur->value->type == VAL_STRING) {
                /* Store string values as ints by hashing - fallback to index */
                v = make_int(count);
            } else {
                v = make_int(count);
            }
            inst_set_field(inst, cur->name, v);
            count++;
        }
        cur = cur->next;
    }
    inst_set_field(inst, "_count", make_int(count));

    return make_struct_val(inst);
}

/* enum.value(enum_inst, name) — get member value by name */
Value stdlib_enum_value(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst || argc < 2 || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: enum.value(enum, name) required\n");
        return make_null();
    }
    FieldValue* fv = find_field(inst, args[1].string_val);
    if (!fv || fv->name[0] == '_') {
        fprintf(stderr, "Error: enum has no member '%s'\n", args[1].string_val);
        return make_null();
    }
    return *(fv->value);
}

/* enum.name(enum_val) — not applicable on individual values in this model; returns enum name */
Value stdlib_enum_name(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst) {
        fprintf(stderr, "Error: enum.name(enum_inst) required\n");
        return make_null();
    }
    FieldValue* fv = find_field(inst, "_name");
    if (fv && fv->value->type == VAL_STRING) {
        return make_string(fv->value->string_val);
    }
    return make_null();
}

/* enum.names(enum_inst) — get all member names */
Value stdlib_enum_names(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst) return make_null();

    Array* arr = array_create(8);
    FieldValue* cur = inst->fields;
    while (cur) {
        if (cur->name[0] != '_') {
            array_push(arr, make_string(cur->name));
        }
        cur = cur->next;
    }
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* enum.values(enum_inst) — get all member values */
Value stdlib_enum_values(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst) return make_null();

    Array* arr = array_create(8);
    FieldValue* cur = inst->fields;
    while (cur) {
        if (cur->name[0] != '_') {
            array_push(arr, *(cur->value));
        }
        cur = cur->next;
    }
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* enum.has(enum_inst, name) — check if member exists */
Value stdlib_enum_has(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst || argc < 2 || args[1].type != VAL_STRING) return make_bool(false);
    FieldValue* fv = find_field(inst, args[1].string_val);
    return make_bool(fv != NULL && fv->name[0] != '_');
}

/* enum.count(enum_inst) — number of members */
Value stdlib_enum_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst) return make_int(0);
    FieldValue* fv = find_field(inst, "_count");
    if (fv && fv->value->type == VAL_INT) return *(fv->value);
    /* Fallback: count manually */
    long long c = 0;
    FieldValue* cur = inst->fields;
    while (cur) { if (cur->name[0] != '_') c++; cur = cur->next; }
    return make_int(c);
}

/* enum.from_value(enum_inst, int_val) — reverse lookup by value */
Value stdlib_enum_from_value(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst || argc < 2 || args[1].type != VAL_INT) {
        fprintf(stderr, "Error: enum.from_value(enum_inst, int_value) required\n");
        return make_null();
    }
    long long target = args[1].int_val;
    FieldValue* cur = inst->fields;
    while (cur) {
        if (cur->name[0] != '_' && cur->value->type == VAL_INT &&
            cur->value->int_val == target) {
            return make_string(cur->name);
        }
        cur = cur->next;
    }
    return make_null();
}

/* enum.to_array(enum_inst) — convert to [[name, value], ...] */
Value stdlib_enum_to_array(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    StructInstance* inst = get_enum_instance(args, argc, 0);
    if (!inst) return make_null();

    Array* arr = array_create(8);
    FieldValue* cur = inst->fields;
    while (cur) {
        if (cur->name[0] != '_') {
            Array* pair = array_create(2);
            array_push(pair, make_string(cur->name));
            array_push(pair, *(cur->value));
            Value p; p.type = VAL_ARRAY; p.array_val = pair;
            array_push(arr, p);
        }
        cur = cur->next;
    }
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

void stdlib_enum_register(void) {
    register_native("enum.create", (NativeFn)stdlib_enum_create);
    register_native("enum.create_values", (NativeFn)stdlib_enum_create_values);
    register_native("enum.value", (NativeFn)stdlib_enum_value);
    register_native("enum.name", (NativeFn)stdlib_enum_name);
    register_native("enum.names", (NativeFn)stdlib_enum_names);
    register_native("enum.values", (NativeFn)stdlib_enum_values);
    register_native("enum.has", (NativeFn)stdlib_enum_has);
    register_native("enum.count", (NativeFn)stdlib_enum_count);
    register_native("enum.from_value", (NativeFn)stdlib_enum_from_value);
    register_native("enum.to_array", (NativeFn)stdlib_enum_to_array);
}
