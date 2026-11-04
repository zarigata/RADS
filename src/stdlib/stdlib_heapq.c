#include "stdlib_heapq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── HeapQ Module ──
 * Python-inspired heap queue (priority queue) for RADS.
 * Min-heap by default, like Python's heapq.
 * Operates in-place on RADS arrays (Array* in Value.array_val).
 *
 * Functions (all registered as "heapq.<name>"):
 * - heapq.heapify(arr)       - transform array into a heap in-place
 * - heapq.heappush(arr, val) - push value, restoring heap order
 * - heapq.heappop(arr)       - pop and return smallest value
 * - heapq.heappushpop(arr, v)- push then pop (one sift only)
 * - heapq.heapreplace(arr, v)- pop then push (always replaces root)
 * - heapq.nlargest(n, arr)   - array of n largest values
 * - heapq.nsmallest(n, arr)  - array of n smallest values
 * - heapq.merge(a, b)        - merge two sorted arrays into a new one
 * - heapq.is_heap(arr)       - true if array satisfies heap invariant
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

/* ── Helpers ── */

/* Numeric view of a Value: ints widen to double so mixed heaps still order.
 * Strings and other types compare as 0.0, matching bisect's tolerance. */
static double val_num(const Value* v) {
    if (v->type == VAL_FLOAT) return v->float_val;
    if (v->type == VAL_INT) return (double)v->int_val;
    return 0.0;
}

/* Compare two Values numerically: -1, 0, or 1 */
static int cmp_values(Value a, Value b) {
    double da = val_num(&a), db = val_num(&b);
    return (da > db) - (da < db);
}

/* Swap two slots in a Value array */
static void swap_values(Value* arr, int i, int j) {
    Value tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
}

/* Sift down from start to end (inclusive) — restores min-heap below start */
static void sift_down(Value* arr, int start, int end) {
    int root = start;
    while (2 * root + 1 <= end) {
        int child = 2 * root + 1;
        int swap = root;
        if (cmp_values(arr[swap], arr[child]) > 0) swap = child;
        if (child + 1 <= end && cmp_values(arr[swap], arr[child + 1]) > 0) swap = child + 1;
        if (swap == root) return;
        swap_values(arr, root, swap);
        root = swap;
    }
}

/* Sift up from index start toward the root — used after append */
static void sift_up(Value* arr, int start) {
    int child = start;
    while (child > 0) {
        int parent = (child - 1) / 2;
        if (cmp_values(arr[child], arr[parent]) < 0) {
            swap_values(arr, child, parent);
            child = parent;
        } else {
            break;
        }
    }
}

/* Grow an Array by one slot, doubling capacity when full (same policy as bisect.insort) */
static int array_grow(Array* arr) {
    if (arr->count >= arr->capacity) {
        size_t newcap = arr->capacity ? arr->capacity * 2 : 8;
        Value* items = realloc(arr->items, newcap * sizeof(Value));
        if (!items) return 0;
        arr->items = items;
        arr->capacity = newcap;
    }
    return 1;
}

/* ── Functions ── */

Value stdlib_heapq_heapify(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.heapify(arr) needs an array — got something else.\n");
        return make_null();
    }
    Array* a = args[0].array_val;
    int n = (int)a->count;
    /* Floyd's bottom-up heap construction: O(n) */
    for (int i = (n - 2) / 2; i >= 0; i--) sift_down(a->items, i, n - 1);
    return make_null(); /* in-place, like Python */
}

Value stdlib_heapq_heappush(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.heappush(arr, val) needs an array and a value.\n");
        return make_null();
    }
    Array* a = args[0].array_val;
    if (!array_grow(a)) {
        fprintf(stderr, "Error: heapq.heappush: out of memory.\n");
        return make_null();
    }
    a->items[a->count] = value_retain(args[1]); /* args are freed by the caller after the native returns */
    sift_up(a->items, (int)a->count);
    a->count++;
    return make_null();
}

Value stdlib_heapq_heappop(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.heappop(arr) needs an array.\n");
        return make_null();
    }
    Array* a = args[0].array_val;
    if (a->count == 0) {
        fprintf(stderr, "Error: heapq.heappop: heap is empty — nothing to pop.\n");
        return make_null();
    }
    Value result = a->items[0]; /* MOVE: root payload leaves the heap; slot overwritten by last item */
    a->items[0] = a->items[a->count - 1];
    a->count--;
    if (a->count > 1) sift_down(a->items, 0, (int)a->count - 1);
    return result;
}

Value stdlib_heapq_heappushpop(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.heappushpop(arr, val) needs an array and a value.\n");
        return make_null();
    }
    Array* a = args[0].array_val;
    /* Fast path: new value can't beat the root */
    if (a->count == 0 || cmp_values(args[1], a->items[0]) <= 0) return value_retain(args[1]);
    Value result = a->items[0]; /* MOVE: displaced root leaves the heap (slot overwritten) */
    a->items[0] = value_retain(args[1]); /* CLONE into slot: interpreter frees args after the call */
    sift_down(a->items, 0, (int)a->count - 1);
    return result;
}

Value stdlib_heapq_heapreplace(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.heapreplace(arr, val) needs an array and a value.\n");
        return make_null();
    }
    Array* a = args[0].array_val;
    if (a->count == 0) {
        fprintf(stderr, "Error: heapq.heapreplace: heap is empty — pop first, then push.\n");
        return make_null();
    }
    Value result = a->items[0]; /* MOVE: displaced root leaves the heap (slot overwritten) */
    a->items[0] = value_retain(args[1]); /* CLONE into slot: interpreter frees args after the call */
    sift_down(a->items, 0, (int)a->count - 1);
    return result;
}

/* Shared core for nlargest/nsmallest: copy, sort, take k from one end.
 * Returns a fresh RADS array; string elements are duplicated so the
 * result owns its memory independently of the input. */
static Value top_k(struct Interpreter* interp, int argc, Value* args, int largest) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.nlargest/nsmallest(n, arr) needs an int and an array.\n");
        return make_null();
    }
    Array* src = args[1].array_val;
    int k = (int)args[0].int_val;
    int n = (int)src->count;
    if (k > n) k = n;
    if (k < 0) k = 0;

    Value* copy = malloc((size_t)(n > 0 ? n : 1) * sizeof(Value));
    if (!copy) return make_null();
    memcpy(copy, src->items, (size_t)n * sizeof(Value));
    /* insertion sort: descending for nlargest, ascending for nsmallest;
     * in both cases the k results are simply the first k slots */
    for (int i = 1; i < n; i++) {
        Value key = copy[i];
        int j = i - 1;
        while (j >= 0 && (largest ? cmp_values(copy[j], key) < 0 : cmp_values(copy[j], key) > 0)) {
            copy[j + 1] = copy[j];
            j--;
        }
        copy[j + 1] = key;
    }
    Array* out = array_create((size_t)(k > 0 ? k : 1));
    for (int i = 0; i < k; i++) {
        Value v = copy[i];
        if (v.type == VAL_STRING) {
            Value s;
            s.type = VAL_STRING;
            s.string_val = v.string_val; /* array_push clones it — the pre-strdup leaked */
            array_push(out, s);
        } else {
            array_push(out, v);
        }
    }
    free(copy);
    Value res;
    res.type = VAL_ARRAY;
    res.array_val = out;
    return res;
}

Value stdlib_heapq_nlargest(struct Interpreter* interp, int argc, Value* args) {
    return top_k(interp, argc, args, 1);
}

Value stdlib_heapq_nsmallest(struct Interpreter* interp, int argc, Value* args) {
    return top_k(interp, argc, args, 0);
}

Value stdlib_heapq_merge(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.merge(a, b) needs two sorted arrays.\n");
        return make_null();
    }
    Array* xa = args[0].array_val;
    Array* xb = args[1].array_val;
    size_t ia = 0, ib = 0;
    Array* out = array_create(xa->count + xb->count + 1);
    /* standard two-pointer merge; duplicates kept, like Python's merge() */
    while (ia < xa->count && ib < xb->count) {
        Value* v = cmp_values(xa->items[ia], xb->items[ib]) <= 0
                       ? &xa->items[ia++] : &xb->items[ib++];
        if (v->type == VAL_STRING) {
            Value s;
            s.type = VAL_STRING;
            s.string_val = v->string_val; /* array_push clones it — the pre-strdup leaked */
            array_push(out, s);
        } else {
            array_push(out, *v);
        }
    }
    while (ia < xa->count) array_push(out, xa->items[ia++]);
    while (ib < xb->count) array_push(out, xb->items[ib++]);
    Value res;
    res.type = VAL_ARRAY;
    res.array_val = out;
    return res;
}

Value stdlib_heapq_is_heap(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: heapq.is_heap(arr) needs an array.\n");
        return make_bool(false);
    }
    Array* a = args[0].array_val;
    int n = (int)a->count;
    for (int i = 0; i < n; i++) {
        int left = 2 * i + 1, right = 2 * i + 2;
        if (left < n && cmp_values(a->items[i], a->items[left]) > 0) return make_bool(false);
        if (right < n && cmp_values(a->items[i], a->items[right]) > 0) return make_bool(false);
    }
    return make_bool(true);
}

/* ── Registration ── */
void stdlib_heapq_register(void) {
    register_native("heapq.heapify", (NativeFn)stdlib_heapq_heapify);
    register_native("heapq.heappush", (NativeFn)stdlib_heapq_heappush);
    register_native("heapq.heappop", (NativeFn)stdlib_heapq_heappop);
    register_native("heapq.heappushpop", (NativeFn)stdlib_heapq_heappushpop);
    register_native("heapq.heapreplace", (NativeFn)stdlib_heapq_heapreplace);
    register_native("heapq.nlargest", (NativeFn)stdlib_heapq_nlargest);
    register_native("heapq.nsmallest", (NativeFn)stdlib_heapq_nsmallest);
    register_native("heapq.merge", (NativeFn)stdlib_heapq_merge);
    register_native("heapq.is_heap", (NativeFn)stdlib_heapq_is_heap);
}
