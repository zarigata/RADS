#include "stdlib_heapq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── HeapQ Module ──
 * Python-inspired heap queue (priority queue) for RADS.
 * Uses min-heap by default (like Python's heapq).
 * Lightweight O(n log n) operations, suitable for all devices.
 *
 * Functions:
 * - heapq.heapify(arr)       - Transform array into a heap in-place
 * - heapq.heappush(arr, val) - Push value onto heap
 * - heapq.heappop(arr)       - Pop smallest value from heap
 * - heapq.heappushpop(arr, val) - Push then pop (more efficient than separate ops)
 * - heapq.heapreplace(arr, val) - Pop then push (more efficient than separate ops)
 * - heapq.nlargest(n, arr)   - Get n largest elements
 * - heapq.nsmallest(n, arr)  - Get n smallest elements
 * - heapq.merge(a, b)        - Merge two sorted arrays
 * - heapq.is_heap(arr)       - Check if array satisfies heap invariant
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Value make_error(const char* msg);

/* ── Helpers ── */

static int arr_length(Value arr) {
    return (arr.type == VAL_ARRAY) ? arr.as.array.length : 0;
}

static double arr_get_double(Value arr, int i) {
    Value v = arr.as.array.values[i];
    return (v.type == VAL_FLOAT) ? v.as.floating : (v.type == VAL_INT) ? (double)v.as.integer : 0;
}

/* For heap, we need to compare and swap elements in the array */
static int cmp_values(Value a, Value b) {
    double da = (a.type == VAL_FLOAT) ? a.as.floating : (a.type == VAL_INT) ? (double)a.as.integer : 0;
    double db = (b.type == VAL_FLOAT) ? b.as.floating : (b.type == VAL_INT) ? (double)b.as.integer : 0;
    return (da > db) - (da < db);
}

static void swap_values(Value* arr, int i, int j) {
    Value tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
}

/* ── Sift down (heapify helper) ── */
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

/* ── Sift up (for push) ── */
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

/* ── Functions ── */

static Value native_heapify(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_ARRAY)
        return make_error("heapq.heapify(arr) requires an array");

    int n = arr_length(argv[0]);
    Value* arr = argv[0].as.array.values;

    /* Floyd's heap construction: O(n) */
    for (int i = (n - 2) / 2; i >= 0; i--) {
        sift_down(arr, i, n - 1);
    }

    return make_null(); /* in-place */
}

static Value native_heappush(int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_ARRAY)
        return make_error("heapq.heappush(arr, val) requires array and value");

    Value* arr_val = &argv[0];
    int n = arr_val->as.array.length;

    /* Resize array */
    Value* new_vals = realloc(arr_val->as.array.values, sizeof(Value) * (n + 1));
    if (!new_vals) return make_error("heapq.heappush: out of memory");

    arr_val->as.array.values = new_vals;
    arr_val->as.array.values[n] = argv[1];
    arr_val->as.array.length = n + 1;

    sift_up(arr_val->as.array.values, n);
    return make_null();
}

static Value native_heappop(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_ARRAY)
        return make_error("heapq.heappop(arr) requires an array");

    Value* arr_val = &argv[0];
    int n = arr_val->as.array.length;
    if (n == 0) return make_error("heapq.heappop: heap is empty");

    Value result = arr_val->as.array.values[0];
    arr_val->as.array.values[0] = arr_val->as.array.values[n - 1];
    arr_val->as.array.length = n - 1;

    if (n > 1) sift_down(arr_val->as.array.values, 0, n - 2);

    return result;
}

static Value native_heappushpop(int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_ARRAY)
        return make_error("heapq.heappushpop(arr, val) requires array and value");

    /* Push */
    native_heappush(argc, argv);
    /* Pop */
    return native_heappop(1, argv);
}

static Value native_heapreplace(int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_ARRAY)
        return make_error("heapq.heapreplace(arr, val) requires array and value");

    Value* arr_val = &argv[0];
    int n = arr_val->as.array.length;
    if (n == 0) return make_error("heapq.heapreplace: heap is empty");

    Value result = arr_val->as.array.values[0];
    arr_val->as.array.values[0] = argv[1];
    sift_down(arr_val->as.array.values, 0, n - 1);

    return result;
}

static int cmp_for_sort(const void* a, const void* b) {
    double da, db;
    Value* va = (Value*)a;
    Value* vb = (Value*)b;
    da = (va->type == VAL_FLOAT) ? va->as.floating : (va->type == VAL_INT) ? (double)va->as.integer : 0;
    db = (vb->type == VAL_FLOAT) ? vb->as.floating : (vb->type == VAL_INT) ? (double)vb->as.integer : 0;
    return (da > db) - (da < db);
}

static Value native_nlargest(int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_INT || argv[1].type != VAL_ARRAY)
        return make_error("heapq.nlargest(n, arr) requires int and array");

    int k = (int)argv[0].as.integer;
    int n = argv[1].as.array.length;
    if (k > n) k = n;
    if (k <= 0) return make_string("");

    /* Copy and sort descending */
    Value* copy = malloc(sizeof(Value) * n);
    if (!copy) return make_error("heapq.nlargest: out of memory");
    memcpy(copy, argv[1].as.array.values, sizeof(Value) * n);
    qsort(copy, n, sizeof(Value), cmp_for_sort);

    /* Build result string (largest first) */
    char buf[1024] = "";
    int pos = 0;
    for (int i = 0; i < k; i++) {
        if (i > 0) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
        Value v = copy[n - 1 - i];
        if (v.type == VAL_INT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%lld", v.as.integer);
        else if (v.type == VAL_FLOAT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", v.as.floating);
    }
    free(copy);
    return make_string(buf);
}

static Value native_nsmallest(int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_INT || argv[1].type != VAL_ARRAY)
        return make_error("heapq.nsmallest(n, arr) requires int and array");

    int k = (int)argv[0].as.integer;
    int n = argv[1].as.array.length;
    if (k > n) k = n;
    if (k <= 0) return make_string("");

    Value* copy = malloc(sizeof(Value) * n);
    if (!copy) return make_error("heapq.nsmallest: out of memory");
    memcpy(copy, argv[1].as.array.values, sizeof(Value) * n);
    qsort(copy, n, sizeof(Value), cmp_for_sort);

    char buf[1024] = "";
    int pos = 0;
    for (int i = 0; i < k; i++) {
        if (i > 0) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
        Value v = copy[i];
        if (v.type == VAL_INT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%lld", v.as.integer);
        else if (v.type == VAL_FLOAT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", v.as.floating);
    }
    free(copy);
    return make_string(buf);
}

static Value native_merge(int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_ARRAY || argv[1].type != VAL_ARRAY)
        return make_error("heapq.merge(a, b) requires two arrays");

    int na = argv[0].as.array.length;
    int nb = argv[1].as.array.length;

    char buf[2048] = "";
    int pos = 0, ia = 0, ib = 0, first = 1;

    while (ia < na && ib < nb) {
        if (!first) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
        first = 0;
        if (cmp_values(argv[0].as.array.values[ia], argv[1].as.array.values[ib]) <= 0) {
            Value v = argv[0].as.array.values[ia++];
            if (v.type == VAL_INT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%lld", v.as.integer);
            else pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", v.as.floating);
        } else {
            Value v = argv[1].as.array.values[ib++];
            if (v.type == VAL_INT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%lld", v.as.integer);
            else pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", v.as.floating);
        }
    }
    while (ia < na) {
        if (!first) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
        first = 0;
        Value v = argv[0].as.array.values[ia++];
        if (v.type == VAL_INT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%lld", v.as.integer);
        else pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", v.as.floating);
    }
    while (ib < nb) {
        if (!first) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
        first = 0;
        Value v = argv[1].as.array.values[ib++];
        if (v.type == VAL_INT) pos += snprintf(buf + pos, sizeof(buf) - pos, "%lld", v.as.integer);
        else pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", v.as.floating);
    }
    return make_string(buf);
}

static Value native_is_heap(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_ARRAY)
        return make_error("heapq.is_heap(arr) requires an array");

    int n = argv[0].as.array.length;
    Value* arr = argv[0].as.array.values;

    for (int i = 0; i < n; i++) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        if (left < n && cmp_values(arr[i], arr[left]) > 0) return make_int(0);
        if (right < n && cmp_values(arr[i], arr[right]) > 0) return make_int(0);
    }
    return make_int(1);
}

/* ── Registration ── */
void stdlib_heapq_register(void) {
    register_native("heapq_heapify", native_heapify);
    register_native("heapq_heappush", native_heappush);
    register_native("heapq_heappop", native_heappop);
    register_native("heapq_heappushpop", native_heappushpop);
    register_native("heapq_heapreplace", native_heapreplace);
    register_native("heapq_nlargest", native_nlargest);
    register_native("heapq_nsmallest", native_nsmallest);
    register_native("heapq_merge", native_merge);
    register_native("heapq_is_heap", native_is_heap);
}
