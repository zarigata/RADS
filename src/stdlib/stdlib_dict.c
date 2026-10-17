// stdlib_dict.c — Python-style dict method surface over VAL_DICT.
//
// A dict's payload is the same flat kv array json.parse produces:
// [k0, v0, k1, v1, ...]. The receiver arrives as args[0] (VAL_DICT,
// owned by the caller — we must return owning values, never steal args).
//
// Registered as dict.* natives; the interpreter's method dispatch prepends
// the receiver, so `d.keys()` calls dict.keys(d).
#include "stdlib_dict.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Payload invariants: keys at even indices are VAL_STRING; i+1 < count guard
// on every read. Missing keys are a miss (get returns the default; plain
// index read yields null) — never a crash.

static int dict_payload_ok(Value* v) {
    return v && v->type == VAL_DICT && v->array_val;
}

static size_t dict_pair_count(Value* v) {
    // floor(count/2) — a trailing malformed odd key contributes no pair
    return v->array_val->count / 2;
}

// ---- introspection -------------------------------------------------------

static Value native_dict_len(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || !dict_payload_ok(&args[0])) return make_int(0);
    return make_int((long long)dict_pair_count(&args[0]));
}

static Value native_dict_keys(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || !dict_payload_ok(&args[0])) return make_null();
    Array* out = array_create(dict_pair_count(&args[0]));
    Array* d = args[0].array_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        if (d->items[i].type == VAL_STRING) {
            array_push(out, d->items[i]); // clones
        }
    }
    Value v; v.type = VAL_ARRAY; v.array_val = out;
    return v;
}

static Value native_dict_values(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || !dict_payload_ok(&args[0])) return make_null();
    Array* out = array_create(dict_pair_count(&args[0]));
    Array* d = args[0].array_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        array_push(out, d->items[i + 1]); // clones
    }
    Value v; v.type = VAL_ARRAY; v.array_val = out;
    return v;
}

static Value native_dict_items(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || !dict_payload_ok(&args[0])) return make_null();
    Array* out = array_create(dict_pair_count(&args[0]));
    Array* d = args[0].array_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        Array* pair = array_create(2);
        array_push(pair, d->items[i]);     // key (cloned)
        array_push(pair, d->items[i + 1]); // value (cloned)
        Value pv; pv.type = VAL_ARRAY; pv.array_val = pair;
        array_push(out, pv); // clones → bumps pair refcount to 2
        value_free(&pv);      // ours released; out owns it now
    }
    Value v; v.type = VAL_ARRAY; v.array_val = out;
    return v;
}

// ---- lookup --------------------------------------------------------------

static Value native_dict_get(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || !dict_payload_ok(&args[0])) return make_null();
    const char* key = (args[1].type == VAL_STRING) ? args[1].string_val : NULL;
    if (!key) return (argc >= 3) ? value_clone(args[2]) : make_null();
    Array* d = args[0].array_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        if (d->items[i].type == VAL_STRING &&
            strcmp(d->items[i].string_val, key) == 0) {
            return value_clone(d->items[i + 1]); // owning copy for the caller
        }
    }
    return (argc >= 3) ? value_clone(args[2]) : make_null();
}

static Value native_dict_contains(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || !dict_payload_ok(&args[0])) return make_bool(false);
    const char* key = (args[1].type == VAL_STRING) ? args[1].string_val : NULL;
    if (!key) return make_bool(false);
    Array* d = args[0].array_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        if (d->items[i].type == VAL_STRING &&
            strcmp(d->items[i].string_val, key) == 0) {
            return make_bool(true);
        }
    }
    return make_bool(false);
}

// ---- mutation ------------------------------------------------------------

static Value native_dict_pop(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || !dict_payload_ok(&args[0])) return make_null();
    const char* key = (args[1].type == VAL_STRING) ? args[1].string_val : NULL;
    if (!key) return make_null();
    Array* d = args[0].array_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        if (d->items[i].type == VAL_STRING &&
            strcmp(d->items[i].string_val, key) == 0) {
            Value result = d->items[i + 1]; // steal ownership (caller owns)
            value_free(&d->items[i]);      // free key string, null the slot
            // Remove the pair: shift the tail left by 2.
            for (size_t j = i; j + 2 < d->count; j++) {
                d->items[j] = d->items[j + 2];
            }
            d->count -= 2;
            // The popped value's refcount story: the payload slot no longer
            // owns it; result goes to the caller as-is.
            return result;
        }
    }
    return (argc >= 3) ? value_clone(args[2]) : make_null();
}

static Value native_dict_update(Interpreter* interp, int argc, Value* args) {
    // d.update(other): merge other's pairs into d (other wins).
    (void)interp;
    if (argc < 2 || !dict_payload_ok(&args[0]) || !dict_payload_ok(&args[1])) {
        return make_null();
    }
    Array* d = args[0].array_val;
    Array* o = args[1].array_val;
    for (size_t i = 0; i + 1 < o->count; i += 2) {
        if (o->items[i].type != VAL_STRING) continue;
        // replace-or-append into d
        int replaced = 0;
        for (size_t j = 0; j + 1 < d->count; j += 2) {
            if (d->items[j].type == VAL_STRING &&
                strcmp(d->items[j].string_val, o->items[i].string_val) == 0) {
                value_free(&d->items[j + 1]);
                d->items[j + 1] = value_clone(o->items[i + 1]);
                replaced = 1;
                break;
            }
        }
        if (!replaced) {
            array_push(d, o->items[i]);     // clones key
            array_push(d, o->items[i + 1]); // clones value
        }
    }
    return value_clone(args[0]); // d itself (post-update)
}

static Value native_dict_setdefault(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || !dict_payload_ok(&args[0])) return make_null();
    if (args[1].type != VAL_STRING) return make_null();
    Array* d = args[0].array_val;
    const char* key = args[1].string_val;
    for (size_t i = 0; i + 1 < d->count; i += 2) {
        if (d->items[i].type == VAL_STRING &&
            strcmp(d->items[i].string_val, key) == 0) {
            return value_clone(d->items[i + 1]);
        }
    }
    Value dv = (argc >= 3) ? value_clone(args[2]) : make_null();
    array_push(d, args[1]); // clones key
    array_push(d, dv);      // clones value into the payload...
    return dv;              // ...and dv (still owned by us) goes to the caller
}

static Value native_dict_copy(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || !dict_payload_ok(&args[0])) return make_null();
    Array* src = args[0].array_val;
    Array* out = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        array_push(out, src->items[i]); // clones each (strings dup'd, nested shared+bumped)
    }
    Value v; v.type = VAL_DICT; v.array_val = out;
    return v;
}

// ---- registration --------------------------------------------------------

void stdlib_dict_register(void) {
    register_native("dict.len",        native_dict_len);
    register_native("dict.keys",       native_dict_keys);
    register_native("dict.values",     native_dict_values);
    register_native("dict.items",      native_dict_items);
    register_native("dict.get",        native_dict_get);
    register_native("dict.contains",   native_dict_contains);
    register_native("dict.pop",        native_dict_pop);
    register_native("dict.update",     native_dict_update);
    register_native("dict.setdefault", native_dict_setdefault);
    register_native("dict.copy",       native_dict_copy);
}
