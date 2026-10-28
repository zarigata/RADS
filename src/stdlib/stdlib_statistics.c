#include "stdlib_statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Statistics Module ──
 * Python-inspired statistics functions for RADS.
 * Lightweight implementation for all devices including 32-bit systems.
 *
 * Functions (registered as "statistics.<name>"):
 * - statistics.mean(data)              - arithmetic mean
 * - statistics.median(data)            - median (average of middle pair if even)
 * - statistics.median_low(data)        - low median for even-length data
 * - statistics.median_high(data)       - high median for even-length data
 * - statistics.mode(data)              - most common value
 * - statistics.stdev(data)             - sample standard deviation
 * - statistics.pstdev(data)            - population standard deviation
 * - statistics.variance(data)          - sample variance
 * - statistics.pvariance(data)         - population variance
 * - statistics.geometric_mean(data)    - geometric mean
 * - statistics.harmonic_mean(data)     - harmonic mean
 * - statistics.quantiles(data, n=4)    - array of n-1 cut points
 * - statistics.min/max/sum/count(data)
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

#define MAX_DATA 4096

/* ── Helpers ── */

/* Numeric view of a Value (ints widen to double; anything else is 0.0) */
static double val_num(const Value* v) {
    if (v->type == VAL_FLOAT) return v->float_val;
    if (v->type == VAL_INT) return (double)v->int_val;
    return 0.0;
}

/* Extract a numeric array from a Value into out[] (capped at max_len).
 * Returns element count, or -1 if not an array or a non-numeric item was found. */
static int extract_doubles(Value arr, double* out, int max_len) {
    if (arr.type != VAL_ARRAY) return -1;
    Array* a = arr.array_val;
    int len = (int)a->count < max_len ? (int)a->count : max_len;
    for (int i = 0; i < len; i++) {
        if (a->items[i].type == VAL_INT) out[i] = (double)a->items[i].int_val;
        else if (a->items[i].type == VAL_FLOAT) out[i] = a->items[i].float_val;
        else return -1;
    }
    return len;
}

/* Sort doubles ascending (qsort comparator) */
static int cmp_double(const void* a, const void* b) {
    double da = *(const double*)a, db = *(const double*)b;
    return (da > db) - (da < db);
}

/* ── Functions ── */

Value stdlib_statistics_mean(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.mean(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.mean: needs a non-empty numeric array.\n"); return make_null(); }
    double sum = 0;
    for (int i = 0; i < n; i++) sum += data[i];
    return make_float(sum / n);
}

Value stdlib_statistics_median(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.median(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.median: needs a non-empty numeric array.\n"); return make_null(); }
    qsort(data, n, sizeof(double), cmp_double);
    if (n % 2 == 1) return make_float(data[n / 2]);
    return make_float((data[n / 2 - 1] + data[n / 2]) / 2.0);
}

Value stdlib_statistics_median_low(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.median_low(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.median_low: needs a non-empty numeric array.\n"); return make_null(); }
    qsort(data, n, sizeof(double), cmp_double);
    return make_float(data[(n - 1) / 2]);
}

Value stdlib_statistics_median_high(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.median_high(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.median_high: needs a non-empty numeric array.\n"); return make_null(); }
    qsort(data, n, sizeof(double), cmp_double);
    return make_float(data[n / 2]);
}

Value stdlib_statistics_mode(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.mode(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.mode: needs a non-empty numeric array.\n"); return make_null(); }
    qsort(data, n, sizeof(double), cmp_double);
    double mode_val = data[0];
    int max_count = 1, cur_count = 1;
    for (int i = 1; i < n; i++) {
        if (data[i] == data[i - 1]) {
            cur_count++;
        } else {
            if (cur_count > max_count) { max_count = cur_count; mode_val = data[i - 1]; }
            cur_count = 1;
        }
    }
    if (cur_count > max_count) mode_val = data[n - 1];
    return make_float(mode_val);
}

Value stdlib_statistics_variance(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.variance(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 1) { fprintf(stderr, "Error: statistics.variance: needs at least 2 values.\n"); return make_null(); }
    double sum = 0;
    for (int i = 0; i < n; i++) sum += data[i];
    double mean = sum / n;
    double sq_sum = 0;
    for (int i = 0; i < n; i++) { double d = data[i] - mean; sq_sum += d * d; }
    return make_float(sq_sum / (n - 1)); /* sample variance */
}

Value stdlib_statistics_pvariance(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.pvariance(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.pvariance: needs a non-empty numeric array.\n"); return make_null(); }
    double sum = 0;
    for (int i = 0; i < n; i++) sum += data[i];
    double mean = sum / n;
    double sq_sum = 0;
    for (int i = 0; i < n; i++) { double d = data[i] - mean; sq_sum += d * d; }
    return make_float(sq_sum / n); /* population variance */
}

/* stdev = sqrt(variance); propagate the error path untouched */
Value stdlib_statistics_stdev(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    Value v = stdlib_statistics_variance(interp, argc, args);
    if (v.type != VAL_FLOAT) return v;
    return make_float(sqrt(v.float_val));
}

Value stdlib_statistics_pstdev(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    Value v = stdlib_statistics_pvariance(interp, argc, args);
    if (v.type != VAL_FLOAT) return v;
    return make_float(sqrt(v.float_val));
}

Value stdlib_statistics_geometric_mean(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.geometric_mean(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.geometric_mean: needs a non-empty numeric array.\n"); return make_null(); }
    double log_sum = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] <= 0) { fprintf(stderr, "Error: statistics.geometric_mean: all values must be positive.\n"); return make_null(); }
        log_sum += log(data[i]);
    }
    return make_float(exp(log_sum / n));
}

Value stdlib_statistics_harmonic_mean(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.harmonic_mean(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.harmonic_mean: needs a non-empty numeric array.\n"); return make_null(); }
    double inv_sum = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] == 0) { fprintf(stderr, "Error: statistics.harmonic_mean: zero value not allowed.\n"); return make_null(); }
        inv_sum += 1.0 / data[i];
    }
    return make_float((double)n / inv_sum);
}

/* quantiles(data, n=4): n-1 cut points via linear interpolation.
 * Returns a fresh RADS array, mirroring Python's list return. */
Value stdlib_statistics_quantiles(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.quantiles(data [, n]) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.quantiles: needs a non-empty numeric array.\n"); return make_null(); }
    int divisions = 4;
    if (argc >= 2 && args[1].type == VAL_INT) divisions = (int)args[1].int_val;
    if (divisions < 1) divisions = 4;
    qsort(data, n, sizeof(double), cmp_double);

    Array* out = array_create((size_t)(divisions > 1 ? divisions - 1 : 1));
    for (int i = 1; i < divisions; i++) {
        double idx = (double)(n - 1) * i / divisions;
        int lo = (int)idx;
        int hi = lo + 1 < n ? lo + 1 : lo;
        double frac = idx - lo;
        array_push(out, make_float(data[lo] + frac * (data[hi] - data[lo])));
    }
    Value res;
    res.type = VAL_ARRAY;
    res.array_val = out;
    return res;
}

Value stdlib_statistics_min(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.min(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.min: needs a non-empty numeric array.\n"); return make_null(); }
    double m = data[0];
    for (int i = 1; i < n; i++) if (data[i] < m) m = data[i];
    return make_float(m);
}

Value stdlib_statistics_max(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.max(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.max: needs a non-empty numeric array.\n"); return make_null(); }
    double m = data[0];
    for (int i = 1; i < n; i++) if (data[i] > m) m = data[i];
    return make_float(m);
}

Value stdlib_statistics_sum(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { fprintf(stderr, "Error: statistics.sum(data) needs a numeric array.\n"); return make_null(); }
    double data[MAX_DATA];
    int n = extract_doubles(args[0], data, MAX_DATA);
    if (n <= 0) { fprintf(stderr, "Error: statistics.sum: needs a non-empty numeric array.\n"); return make_null(); }
    double s = 0;
    for (int i = 0; i < n; i++) s += data[i];
    return make_float(s);
}

Value stdlib_statistics_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) { fprintf(stderr, "Error: statistics.count(data) needs an array.\n"); return make_null(); }
    return make_int((long long)args[0].array_val->count);
}

/* ── Registration ── */
void stdlib_statistics_register(void) {
    register_native("statistics.mean", (NativeFn)stdlib_statistics_mean);
    register_native("statistics.median", (NativeFn)stdlib_statistics_median);
    register_native("statistics.median_low", (NativeFn)stdlib_statistics_median_low);
    register_native("statistics.median_high", (NativeFn)stdlib_statistics_median_high);
    register_native("statistics.mode", (NativeFn)stdlib_statistics_mode);
    register_native("statistics.stdev", (NativeFn)stdlib_statistics_stdev);
    register_native("statistics.pstdev", (NativeFn)stdlib_statistics_pstdev);
    register_native("statistics.variance", (NativeFn)stdlib_statistics_variance);
    register_native("statistics.pvariance", (NativeFn)stdlib_statistics_pvariance);
    register_native("statistics.geometric_mean", (NativeFn)stdlib_statistics_geometric_mean);
    register_native("statistics.harmonic_mean", (NativeFn)stdlib_statistics_harmonic_mean);
    register_native("statistics.quantiles", (NativeFn)stdlib_statistics_quantiles);
    register_native("statistics.min", (NativeFn)stdlib_statistics_min);
    register_native("statistics.max", (NativeFn)stdlib_statistics_max);
    register_native("statistics.sum", (NativeFn)stdlib_statistics_sum);
    register_native("statistics.count", (NativeFn)stdlib_statistics_count);
}
