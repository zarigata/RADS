// ============================================================================
// stdlib_promise.c — Promise-style combinators over RADS functions (TS parity)
// nightshift 20261027.
//
// RADS is a synchronous interpreter: no microtask queue exists, so these
// natives "settle" every input function immediately by invoking it. What they
// preserve is the *semantics* that matter for real usage:
//   Promise.allSettled — never rejects; one {status, value/reason} record
//                        per input, in input order
//   Promise.all        — first rejection re-raised into the caller's scope
//   Promise.race       — first settled (value or throw) wins
//   Promise.any        — first fulfillment wins; AggregateError-shaped
//                        {name:"AggregateError", message, errors} if none
// Rejections are modelled exactly as TS models them: a `throw` inside the
// function body. That only works because the 20261027 throw-barrier fix lets
// a throw cross the call boundary where interpreter_take_throw() can see it.
//
// Memory law: payload Values are moved via value_retain()/value_free() only —
// never store raw args or raw array items (the 20261024 UAF law).
//
// Extend: when a real event loop lands, these can drive uv handles instead
// of invoking inline — the signatures here already match that shape.
// ============================================================================

#include "../core/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void register_native(const char* name, NativeFn fn);

// ---- shared helpers ---------------------------------------------------------

// Append a field holding an OWNED copy of `v` to an anonymous instance.
// Fields are a linked list built in reverse; order is irrelevant for lookup.
static bool add_value_field(StructInstance* instance, const char* name, Value v) {
    FieldValue* field = malloc(sizeof(FieldValue));
    if (!field) return false;
    field->name = strdup(name);
    field->value = malloc(sizeof(Value));
    if (!field->name || !field->value) {
        free(field->name); free(field->value); free(field);
        return false;
    }
    *field->value = value_retain(v); /* own payload: survives our value_free */
    field->next = instance->fields;
    instance->fields = field;
    return true;
}

// Append a string field (copied). add_value_field does the retaining.
static bool add_string_field(StructInstance* instance, const char* name, const char* text) {
    Value sv;
    sv.type = VAL_STRING;
    sv.string_val = (char*)text;
    return add_value_field(instance, name, sv);
}

// Fresh anonymous struct instance (definition NULL — same shape as
// regex.search's result). Caller frees via value_free on the wrapping Value.
static StructInstance* anon_instance(void) {
    StructInstance* instance = malloc(sizeof(StructInstance));
    if (instance) {
        instance->definition = NULL;
        instance->fields = NULL;
    }
    return instance;
}

// Invoke fn and settle it. Returns true + stores OWNED result in *result_out,
// or false + stores OWNED rejection reason in *reason_out (throw taken from
// the in-flight state, or a synthesized type-error for non-callables).
static bool settle_call(Value fn, Value* result_out, Value* reason_out) {
    if (fn.type != VAL_FUNCTION) {
        *reason_out = make_string("not a function");
        *result_out = make_null();
        return false;
    }
    Value result = interpreter_execute_callback(fn, 0, NULL);
    if (interpreter_has_throw()) {
        *reason_out = interpreter_take_throw();
        value_free(&result);
        *result_out = make_null();
        return false;
    }
    *result_out = result;
    *reason_out = make_null();
    return true;
}

// Wrap one settlement into a {status, value/reason} record Value.
// Returns VAL_NULL on OOM (caller decides how loud to be about that).
static Value make_settlement_record(bool fulfilled, Value payload) {
    StructInstance* rec = anon_instance();
    if (!rec) return make_null();
    bool ok = fulfilled
        ? add_string_field(rec, "status", "fulfilled") && add_value_field(rec, "value", payload)
        : add_string_field(rec, "status", "rejected") && add_value_field(rec, "reason", payload);
    if (!ok) return make_null();
    Value rv;
    rv.type = VAL_STRUCT_INSTANCE;
    rv.struct_instance = rec;
    return rv;
}

// ---- Promise.allSettled(fns) -------------------------------------------------
// TS: always resolves; never rejects. One record per input, input order.
Value stdlib_promise_allSettled(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: Promise.allSettled() requires an array of functions\n");
        return make_null();
    }
    Array* src = args[0].array_val;
    Array* out = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value payload;
        Value reason;
        bool fulfilled = settle_call(src->items[i], &payload, &reason);
        Value keep = fulfilled ? payload : reason;
        Value drop = fulfilled ? reason : payload;
        value_free(&drop);
        Value rec = make_settlement_record(fulfilled, keep);
        value_free(&keep);
        if (rec.type == VAL_NULL) {
            continue; /* OOM: skip the record rather than crash the program */
        }
        array_push(out, rec); /* clones internally */
        value_free(&rec);
    }
    Value ret;
    ret.type = VAL_ARRAY;
    ret.array_val = out;
    return ret;
}

// ---- Promise.all(fns) --------------------------------------------------------
// TS: resolve with all values in order, or reject with the first rejection
// reason — re-raised as a real in-flight throw for the caller's try/catch.
Value stdlib_promise_all(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: Promise.all() requires an array of functions\n");
        return make_null();
    }
    Array* src = args[0].array_val;
    Array* out = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value payload;
        Value reason;
        bool fulfilled = settle_call(src->items[i], &payload, &reason);
        if (!fulfilled) {
            /* Free partial results without giving up the reason (refcount
             * law: mirrors value_release's array branch). */
            if (out) {
                for (size_t j = 0; j < out->count; j++) {
                    Value* item = &out->items[j];
                    switch (item->type) {
                        case VAL_STRING: free(item->string_val); break;
                        case VAL_ARRAY:
                            if (item->array_val) {
                                if (item->array_val->refcount > 0) item->array_val->refcount--;
                                if (item->array_val->refcount == 0) {
                                    for (size_t k = 0; k < item->array_val->count; k++) {
                                        /* nested release is recursive by shape; strings/ints are flat here */
                                        Value* inner = &item->array_val->items[k];
                                        if (inner->type == VAL_STRING) free(inner->string_val);
                                        else if (inner->type == VAL_ARRAY && inner->array_val && --inner->array_val->refcount == 0) {
                                            free(inner->array_val->items);
                                            free(inner->array_val);
                                        }
                                    }
                                    free(item->array_val->items);
                                    free(item->array_val);
                                }
                            }
                            break;
                        default: break;
                    }
                }
                free(out->items);
                free(out);
            }
            interpreter_raise(reason);
            value_free(&reason);
            return make_null();
        }
        array_push(out, payload);
        value_free(&payload);
    }
    Value ret;
    ret.type = VAL_ARRAY;
    ret.array_val = out;
    return ret;
}

// ---- Promise.race(fns) -------------------------------------------------------
// TS: first settled wins, value or rejection. Synchronously that is the first
// input that returns or throws; we surface a rejection as a real throw.
Value stdlib_promise_race(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: Promise.race() requires an array of functions\n");
        return make_null();
    }
    Array* src = args[0].array_val;
    for (size_t i = 0; i < src->count; i++) {
        Value payload;
        Value reason;
        bool fulfilled = settle_call(src->items[i], &payload, &reason);
        if (!fulfilled) {
            interpreter_raise(reason);
            value_free(&reason);
            return make_null();
        }
        return payload; /* owned; caller (eval_call) takes the reference */
    }
    return make_null(); /* empty input: TS resolves forever; null here */
}



// ---- Promise.any(fns) --------------------------------------------------------
// TS: first FULFILLED value wins; if every input rejects, reject with an
// AggregateError {name, message, errors}. Synchronously: invoke in order,
// return the first success, else raise the AggregateError-shaped record.
Value stdlib_promise_any(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: Promise.any() requires an array of functions\n");
        return make_null();
    }
    Array* src = args[0].array_val;
    Array* errors = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value payload;
        Value reason;
        bool fulfilled = settle_call(src->items[i], &payload, &reason);
        if (fulfilled) {
            /* free the reasons gathered so far, then hand back the winner */
            for (size_t j = 0; j < errors->count; j++) {
                Value* e = &errors->items[j];
                if (e->type == VAL_STRING) free(e->string_val);
            }
            free(errors->items);
            free(errors);
            return payload; /* owned */
        }
        array_push(errors, reason); /* clones internally */
        value_free(&reason);
    }
    /* all rejected: build AggregateError {name, message, errors} and raise it */
    StructInstance* rec = anon_instance();
    Value err;
    err.type = VAL_STRUCT_INSTANCE;
    err.struct_instance = rec;
    if (rec &&
        add_string_field(rec, "name", "AggregateError") &&
        add_string_field(rec, "message", "All promises were rejected")) {
        Value ev;
        ev.type = VAL_ARRAY;
        ev.array_val = errors;
        add_value_field(rec, "errors", ev); /* retains: refcount 2 now */
        if (errors->refcount > 0) errors->refcount--; /* drop our stack ref */
        interpreter_raise(err);
        value_free(&err);
        return make_null();
    }
    value_free(&err);
    for (size_t j = 0; j < errors->count; j++) {
        Value* e = &errors->items[j];
        if (e->type == VAL_STRING) free(e->string_val);
    }
    free(errors->items);
    free(errors);
    return make_null();
}

void stdlib_promise_register(void) {
    register_native("Promise.allSettled", (NativeFn)stdlib_promise_allSettled);
    register_native("Promise.all", (NativeFn)stdlib_promise_all);
    register_native("Promise.race", (NativeFn)stdlib_promise_race);
    register_native("Promise.any", (NativeFn)stdlib_promise_any);
}
