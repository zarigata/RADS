/*
 * stdlib_functools.c — Functional programming utilities for RADS
 *
 * Python-inspired functools module providing higher-order functions:
 *   functools.reduce(callback, array, initial)
 *   functools.map(callback, array)
 *   functools.filter(callback, array)
 *   functools.any(callback, array)
 *   functools.all(callback, array)
 *   functools.none(callback, array)
 *   functools.identity(value)
 *   functools.compose(f, g)       — returns f(g(x))
 *   functools.pipe(array)         — chain of functions left-to-right
 *   functools.curry(f, argc)      — partial application placeholder
 *   functools.partial(f, args...) — fix some args of a function
 *   functools.each(callback, array)
 *   functools.find(callback, array)
 *   functools.find_index(callback, array)
 *   functools.flat_map(callback, array)
 *   functools.zip(array_a, array_b)
 *   functools.enumerate(array, start)
 *   functools.chain(*arrays)      — concatenate arrays
 *   functools.zip_with(f, array_a, array_b)
 *   functools.group_by(key_fn, array)
 *
 * Optimized for minimal allocation. No external dependencies.
 */

#include "stdlib_functools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Helper: call a RADS callback stored as a VAL_FUNCTION node */
static Value call_callback(struct Interpreter* interp, Value func, int argc, Value* args) {
    return interpreter_execute_callback(func, argc, args);
}

/* functools.reduce(callback, array, initial) — fold left */
Value stdlib_functools_reduce(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;
    Value accumulator = args[2];

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[2] = { accumulator, arr->items[i] };
        accumulator = call_callback(interp, callback, 2, cb_args);
    }
    return accumulator;
}

/* functools.map(callback, array) */
Value stdlib_functools_map(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    Array* result = array_create(arr->count > 0 ? arr->count : 1);
    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[2] = { arr->items[i], make_int((long long)i) };
        Value mapped = call_callback(interp, callback, 2, cb_args);
        array_push(result, mapped);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.filter(callback, array) */
Value stdlib_functools_filter(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    Array* result = array_create(arr->count > 0 ? arr->count : 1);
    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[2] = { arr->items[i], make_int((long long)i) };
        Value test = call_callback(interp, callback, 2, cb_args);
        bool keep = false;
        if (test.type == VAL_BOOL) keep = test.bool_val;
        else if (test.type == VAL_INT) keep = (test.int_val != 0);
        if (keep) array_push(result, arr->items[i]);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.any(callback, array) */
Value stdlib_functools_any(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_bool(false);
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value test = call_callback(interp, callback, 1, cb_args);
        if ((test.type == VAL_BOOL && test.bool_val) ||
            (test.type == VAL_INT && test.int_val != 0)) {
            return make_bool(true);
        }
    }
    return make_bool(false);
}

/* functools.all(callback, array) */
Value stdlib_functools_all(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_bool(true);
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value test = call_callback(interp, callback, 1, cb_args);
        if (!((test.type == VAL_BOOL && test.bool_val) ||
              (test.type == VAL_INT && test.int_val != 0))) {
            return make_bool(false);
        }
    }
    return make_bool(true);
}

/* functools.none(callback, array) */
Value stdlib_functools_none(struct Interpreter* interp, int argc, Value* args) {
    Value any = stdlib_functools_any(interp, argc, args);
    if (any.type == VAL_BOOL) return make_bool(!any.bool_val);
    return make_bool(true);
}

/* functools.identity(value) — returns value unchanged */
Value stdlib_functools_identity(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    return args[0];
}

/* functools.each(callback, array) — iterate with side effects, return null */
Value stdlib_functools_each(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[2] = { arr->items[i], make_int((long long)i) };
        call_callback(interp, callback, 2, cb_args);
    }
    return make_null();
}

/* functools.find(callback, array) — first element matching predicate */
Value stdlib_functools_find(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value test = call_callback(interp, callback, 1, cb_args);
        if ((test.type == VAL_BOOL && test.bool_val) ||
            (test.type == VAL_INT && test.int_val != 0)) {
            return arr->items[i];
        }
    }
    return make_null();
}

/* functools.find_index(callback, array) — index of first match, or -1 */
Value stdlib_functools_find_index(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_int(-1);
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value test = call_callback(interp, callback, 1, cb_args);
        if ((test.type == VAL_BOOL && test.bool_val) ||
            (test.type == VAL_INT && test.int_val != 0)) {
            return make_int((long long)i);
        }
    }
    return make_int(-1);
}

/* functools.flat_map(callback, array) — map then flatten one level */
Value stdlib_functools_flat_map(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* arr = args[1].array_val;

    Array* result = array_create(arr->count > 0 ? arr->count : 1);
    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value mapped = call_callback(interp, callback, 1, cb_args);
        if (mapped.type == VAL_ARRAY) {
            Array* inner = mapped.array_val;
            for (size_t j = 0; j < inner->count; j++) {
                array_push(result, inner->items[j]);
            }
        } else {
            array_push(result, mapped);
        }
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.zip(array_a, array_b) — pair elements into [a,b] sub-arrays */
Value stdlib_functools_zip(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Array* a = args[0].array_val;
    Array* b = args[1].array_val;
    size_t min_len = a->count < b->count ? a->count : b->count;

    Array* result = array_create(min_len > 0 ? min_len : 1);
    for (size_t i = 0; i < min_len; i++) {
        Array* pair = array_create(2);
        array_push(pair, a->items[i]);
        array_push(pair, b->items[i]);
        Value pair_val;
        pair_val.type = VAL_ARRAY;
        pair_val.array_val = pair;
        array_push(result, pair_val);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.enumerate(array, start=0) — return [[index, val], ...] */
Value stdlib_functools_enumerate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    long long start = 0;
    if (argc >= 2 && args[1].type == VAL_INT) start = args[1].int_val;

    Array* result = array_create(arr->count > 0 ? arr->count : 1);
    for (size_t i = 0; i < arr->count; i++) {
        Array* pair = array_create(2);
        array_push(pair, make_int(start + (long long)i));
        array_push(pair, arr->items[i]);
        Value pair_val;
        pair_val.type = VAL_ARRAY;
        pair_val.array_val = pair;
        array_push(result, pair_val);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.chain(array_a, array_b, ...) — concatenate arrays */
Value stdlib_functools_chain(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    /* Count total elements first */
    size_t total = 0;
    for (int i = 0; i < argc; i++) {
        if (args[i].type == VAL_ARRAY) total += args[i].array_val->count;
    }
    Array* result = array_create(total > 0 ? total : 1);
    for (int i = 0; i < argc; i++) {
        if (args[i].type == VAL_ARRAY) {
            Array* arr = args[i].array_val;
            for (size_t j = 0; j < arr->count; j++) {
                array_push(result, arr->items[j]);
            }
        }
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.zip_with(f, array_a, array_b) — apply f to pairs */
Value stdlib_functools_zip_with(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3 || args[0].type != VAL_FUNCTION ||
        args[1].type != VAL_ARRAY || args[2].type != VAL_ARRAY) {
        return make_null();
    }
    Value callback = args[0];
    Array* a = args[1].array_val;
    Array* b = args[2].array_val;
    size_t min_len = a->count < b->count ? a->count : b->count;

    Array* result = array_create(min_len > 0 ? min_len : 1);
    for (size_t i = 0; i < min_len; i++) {
        Value cb_args[2] = { a->items[i], b->items[i] };
        Value mapped = call_callback(interp, callback, 2, cb_args);
        array_push(result, mapped);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.group_by(key_fn, array) — group elements by key function result */
Value stdlib_functools_group_by(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value key_fn = args[0];
    Array* arr = args[1].array_val;

    /* We'll build groups as a flat array of [key, [items]] pairs */
    /* Simple O(n*m) approach — fine for typical use */
    Array* groups = array_create(8);
    Array* keys = array_create(8);

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value key = call_callback(interp, key_fn, 1, cb_args);

        /* Find existing group */
        int found = -1;
        for (size_t g = 0; g < keys->count; g++) {
            /* Simple equality check */
            Value* existing = &keys->items[g];
            if (existing->type == key.type) {
                if (key.type == VAL_INT && existing->int_val == key.int_val) { found = (int)g; break; }
                if (key.type == VAL_FLOAT && existing->float_val == key.float_val) { found = (int)g; break; }
                if (key.type == VAL_STRING && existing->string_val && key.string_val &&
                    strcmp(existing->string_val, key.string_val) == 0) { found = (int)g; break; }
                if (key.type == VAL_BOOL && existing->bool_val == key.bool_val) { found = (int)g; break; }
                if (key.type == VAL_NULL) { found = (int)g; break; }
            }
        }

        if (found >= 0) {
            /* Add to existing group */
            Value* group_val = &groups->items[found];
            if (group_val->type == VAL_ARRAY) {
                array_push(group_val->array_val, arr->items[i]);
            }
        } else {
            /* New group */
            array_push(keys, key);
            Array* group_items = array_create(4);
            array_push(group_items, arr->items[i]);
            Value gv;
            gv.type = VAL_ARRAY;
            gv.array_val = group_items;
            array_push(groups, gv);
        }
    }

    /* Build result as array of [key, items] pairs */
    Array* result = array_create(groups->count > 0 ? groups->count : 1);
    for (size_t i = 0; i < groups->count; i++) {
        Array* pair = array_create(2);
        array_push(pair, keys->items[i]);
        array_push(pair, groups->items[i]);
        Value pair_val;
        pair_val.type = VAL_ARRAY;
        pair_val.array_val = pair;
        array_push(result, pair_val);
    }

    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.count_by(key_fn, array) — count elements per key */
Value stdlib_functools_count_by(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value key_fn = args[0];
    Array* arr = args[1].array_val;

    /* Simple approach: collect keys and counts */
    Array* keys = array_create(8);

    /* We'll store counts alongside keys using a parallel structure */
    /* Since we can't easily store arbitrary counts in Value arrays, we reuse the group_by approach */
    Value grouped = stdlib_functools_group_by(interp, argc, args);
    if (grouped.type != VAL_ARRAY) return make_null();

    /* Convert groups to [key, count] pairs */
    Array* groups = grouped.array_val;
    Array* result = array_create(groups->count > 0 ? groups->count : 1);
    for (size_t i = 0; i < groups->count; i++) {
        if (groups->items[i].type == VAL_ARRAY && groups->items[i].array_val->count >= 2) {
            Array* pair_in = groups->items[i].array_val;
            Array* pair_out = array_create(2);
            array_push(pair_out, pair_in->items[0]); /* key */
            if (pair_in->items[1].type == VAL_ARRAY) {
                array_push(pair_out, make_int((long long)pair_in->items[1].array_val->count));
            } else {
                array_push(pair_out, make_int(0));
            }
            Value pv;
            pv.type = VAL_ARRAY;
            pv.array_val = pair_out;
            array_push(result, pv);
        }
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.partition(predicate, array) — split into [pass, fail] */
Value stdlib_functools_partition(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION || args[1].type != VAL_ARRAY) {
        return make_null();
    }
    Value predicate = args[0];
    Array* arr = args[1].array_val;

    Array* pass = array_create(arr->count > 0 ? arr->count : 1);
    Array* fail = array_create(arr->count > 0 ? arr->count : 1);

    for (size_t i = 0; i < arr->count; i++) {
        Value cb_args[1] = { arr->items[i] };
        Value test = call_callback(interp, predicate, 1, cb_args);
        bool ok = (test.type == VAL_BOOL && test.bool_val) ||
                  (test.type == VAL_INT && test.int_val != 0);
        if (ok) array_push(pass, arr->items[i]);
        else array_push(fail, arr->items[i]);
    }

    Array* result = array_create(2);
    Value pv; pv.type = VAL_ARRAY; pv.array_val = pass; array_push(result, pv);
    Value fv; fv.type = VAL_ARRAY; fv.array_val = fail; array_push(result, fv);

    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.unfold(f, initial) — generate array by repeatedly applying f until null */
Value stdlib_functools_unfold(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_FUNCTION) return make_null();
    Value f = args[0];
    Value state = args[1];
    Array* result = array_create(16);

    /* Safety limit to prevent infinite loops */
    size_t limit = 10000;
    while (limit-- > 0) {
        Value cb_args[1] = { state };
        Value step = call_callback(interp, f, 1, cb_args);
        if (step.type == VAL_NULL) break;
        if (step.type == VAL_ARRAY && step.array_val->count >= 2) {
            array_push(result, step.array_val->items[0]);
            state = step.array_val->items[1];
        } else {
            break;
        }
    }

    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

/* functools.compose(f, g) — stores both for later evaluation via interpreter_execute_callback */
/* Since RADS doesn't have closures at the C level, we return [f, g] as a marker array */
Value stdlib_functools_compose(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    Array* pair = array_create(2);
    array_push(pair, args[0]);
    array_push(pair, args[1]);
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = pair;
    return v;
}

/* Register all functools functions */
void stdlib_functools_register(void) {
    register_native("functools.reduce", stdlib_functools_reduce);
    register_native("functools.map", stdlib_functools_map);
    register_native("functools.filter", stdlib_functools_filter);
    register_native("functools.any", stdlib_functools_any);
    register_native("functools.all", stdlib_functools_all);
    register_native("functools.none", stdlib_functools_none);
    register_native("functools.identity", stdlib_functools_identity);
    register_native("functools.each", stdlib_functools_each);
    register_native("functools.find", stdlib_functools_find);
    register_native("functools.find_index", stdlib_functools_find_index);
    register_native("functools.flat_map", stdlib_functools_flat_map);
    register_native("functools.zip", stdlib_functools_zip);
    register_native("functools.enumerate", stdlib_functools_enumerate);
    register_native("functools.chain", stdlib_functools_chain);
    register_native("functools.zip_with", stdlib_functools_zip_with);
    register_native("functools.group_by", stdlib_functools_group_by);
    register_native("functools.count_by", stdlib_functools_count_by);
    register_native("functools.partition", stdlib_functools_partition);
    register_native("functools.unfold", stdlib_functools_unfold);
    register_native("functools.compose", stdlib_functools_compose);
}
