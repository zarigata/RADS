/*
 * stdlib_queue.c — RADS queue module
 *
 * FIFO, LIFO (stack), and priority queue implementations using arrays.
 * Equivalent to Python's queue module.
 *
 * Functions:
 *   queue.new()              — create a new FIFO queue
 *   queue.push(q, value)     — enqueue value (FIFO)
 *   queue.pop(q)             — dequeue and return front value
 *   queue.peek(q)            — return front value without removing
 *   queue.size(q)            — return queue length
 *   queue.is_empty(q)        — check if queue is empty
 *   queue.clear(q)           — remove all items
 *   queue.push_left(q, val)  — push to front (deque behavior)
 *   queue.pop_right(q)       — pop from back (LIFO/stack behavior)
 *   queue.push_stack(q, val) — push onto stack (LIFO)
 *   queue.pop_stack(q)       — pop from stack (LIFO)
 *   queue.push_priority(q, val, priority) — insert with priority (lower = higher)
 *   queue.pop_priority(q)    — pop highest priority item
 *   queue.to_array(q)        — convert queue to array
 *   queue.from_array(arr)    — create queue from array
 */

#include "stdlib_queue.h"
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

/*
 * Queue is represented as an array:
 *   items[0] = front, items[count-1] = back
 * This is a simple design that matches RADS' array-centric model.
 * The "queue" is just a VAL_ARRAY used with queue operations.
 */

/* ── Public API ── */

Value stdlib_queue_new(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    Array* arr = array_create(16);
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

Value stdlib_queue_push(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.push(q, value) requires a queue and a value\n");
        return make_null();
    }
    /* Clone string values */
    Value item;
    if (args[1].type == VAL_STRING) {
        item.type = VAL_STRING;
        item.string_val = strdup(args[1].string_val);
    } else {
        item = args[1];
    }
    array_push(args[0].array_val, item);
    return make_null();
}

Value stdlib_queue_pop(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.pop(q) requires a queue\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    if (arr->count == 0) return make_null();

    Value front = arr->items[0];
    /* Shift left */
    memmove(&arr->items[0], &arr->items[1], (arr->count - 1) * sizeof(Value));
    arr->count--;
    return front;
}

Value stdlib_queue_peek(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.peek(q) requires a queue\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    if (arr->count == 0) return make_null();
    return arr->items[0];
}

Value stdlib_queue_size(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.size(q) requires a queue\n");
        return make_int(0);
    }
    return make_int((long long)args[0].array_val->count);
}

Value stdlib_queue_is_empty(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_bool(true);
    return make_bool(args[0].array_val->count == 0);
}

Value stdlib_queue_clear(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    args[0].array_val->count = 0;
    return make_null();
}

Value stdlib_queue_push_left(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.push_left(q, value) requires a queue and a value\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity * 2;
        arr->items = realloc(arr->items, arr->capacity * sizeof(Value));
    }
    /* Shift right to make room at front */
    memmove(&arr->items[1], &arr->items[0], arr->count * sizeof(Value));
    Value item;
    if (args[1].type == VAL_STRING) {
        item.type = VAL_STRING;
        item.string_val = strdup(args[1].string_val);
    } else {
        item = args[1];
    }
    arr->items[0] = item;
    arr->count++;
    return make_null();
}

Value stdlib_queue_pop_right(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.pop_right(q) requires a queue\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    if (arr->count == 0) return make_null();
    arr->count--;
    return arr->items[arr->count];
}

/* Stack (LIFO) operations — aliases using push to back / pop from back */
Value stdlib_queue_push_stack(struct Interpreter* interp, int argc, Value* args) {
    return stdlib_queue_push(interp, argc, args);
}

Value stdlib_queue_pop_stack(struct Interpreter* interp, int argc, Value* args) {
    return stdlib_queue_pop_right(interp, argc, args);
}

/* ── Priority queue ──
 * Items are stored as [value, priority] pairs (arrays of length 2).
 * Lower priority number = higher priority (popped first).
 */

Value stdlib_queue_push_priority(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.push_priority(q, value, priority) requires queue, value, and priority\n");
        return make_null();
    }

    /* Create a pair [value, priority] */
    Array* pair = array_create(2);
    Value item;
    if (args[1].type == VAL_STRING) {
        item.type = VAL_STRING;
        item.string_val = strdup(args[1].string_val);
    } else {
        item = args[1];
    }
    array_push(pair, item);
    array_push(pair, args[2]);  /* priority */

    Value pair_val;
    pair_val.type = VAL_ARRAY;
    pair_val.array_val = pair;

    /* Insert sorted by priority — find position */
    Array* q = args[0].array_val;
    double new_pri = (args[2].type == VAL_FLOAT) ? args[2].float_val : (double)args[2].int_val;

    size_t pos = q->count;
    for (size_t i = 0; i < q->count; i++) {
        if (q->items[i].type == VAL_ARRAY && q->items[i].array_val->count >= 2) {
            Value* pri_val = &q->items[i].array_val->items[1];
            double pri = (pri_val->type == VAL_FLOAT) ? pri_val->float_val : (double)pri_val->int_val;
            if (new_pri < pri) {
                pos = i;
                break;
            }
        }
    }

    /* Make room */
    if (q->count >= q->capacity) {
        q->capacity = q->capacity * 2;
        q->items = realloc(q->items, q->capacity * sizeof(Value));
    }
    if (pos < q->count) {
        memmove(&q->items[pos + 1], &q->items[pos], (q->count - pos) * sizeof(Value));
    }
    q->items[pos] = pair_val;
    q->count++;
    return make_null();
}

Value stdlib_queue_pop_priority(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.pop_priority(q) requires a queue\n");
        return make_null();
    }
    Array* arr = args[0].array_val;
    if (arr->count == 0) return make_null();

    /* Pop the first item (highest priority) and return its value */
    Value front = arr->items[0];
    memmove(&arr->items[0], &arr->items[1], (arr->count - 1) * sizeof(Value));
    arr->count--;

    /* Extract value from [value, priority] pair */
    if (front.type == VAL_ARRAY && front.array_val->count >= 1) {
        return front.array_val->items[0];
    }
    return front;
}

/* ── Conversion ── */

Value stdlib_queue_to_array(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.to_array(q) requires a queue\n");
        return make_null();
    }
    Array* src = args[0].array_val;
    Array* result = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        Value item;
        if (src->items[i].type == VAL_STRING) {
            item.type = VAL_STRING;
            item.string_val = strdup(src->items[i].string_val);
        } else {
            item = src->items[i];
        }
        array_push(result, item);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = result;
    return v;
}

Value stdlib_queue_from_array(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: queue.from_array(arr) requires an array\n");
        return make_null();
    }
    Array* src = args[0].array_val;
    Array* q = array_create(src->count > 0 ? src->count : 16);
    for (size_t i = 0; i < src->count; i++) {
        Value item;
        if (src->items[i].type == VAL_STRING) {
            item.type = VAL_STRING;
            item.string_val = strdup(src->items[i].string_val);
        } else {
            item = src->items[i];
        }
        array_push(q, item);
    }
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = q;
    return v;
}

void stdlib_queue_register(void) {
    register_native("queue.new", (NativeFn)stdlib_queue_new);
    register_native("queue.push", (NativeFn)stdlib_queue_push);
    register_native("queue.pop", (NativeFn)stdlib_queue_pop);
    register_native("queue.peek", (NativeFn)stdlib_queue_peek);
    register_native("queue.size", (NativeFn)stdlib_queue_size);
    register_native("queue.is_empty", (NativeFn)stdlib_queue_is_empty);
    register_native("queue.clear", (NativeFn)stdlib_queue_clear);
    register_native("queue.push_left", (NativeFn)stdlib_queue_push_left);
    register_native("queue.pop_right", (NativeFn)stdlib_queue_pop_right);
    register_native("queue.push_stack", (NativeFn)stdlib_queue_push_stack);
    register_native("queue.pop_stack", (NativeFn)stdlib_queue_pop_stack);
    register_native("queue.push_priority", (NativeFn)stdlib_queue_push_priority);
    register_native("queue.pop_priority", (NativeFn)stdlib_queue_pop_priority);
    register_native("queue.to_array", (NativeFn)stdlib_queue_to_array);
    register_native("queue.from_array", (NativeFn)stdlib_queue_from_array);
}
