#include "stdlib_statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Statistics Module ──
 * Python-inspired statistics functions for RADS.
 * Lightweight implementation for all devices including 32-bit systems.
 *
 * Functions:
 * - statistics.mean(data)              - Arithmetic mean
 * - statistics.median(data)            - Median (middle value)
 * - statistics.median_low(data)        - Low median for even-length data
 * - statistics.median_high(data)       - High median for even-length data
 * - statistics.mode(data)              - Most common value
 * - statistics.stdev(data)             - Sample standard deviation
 * - statistics.pstdev(data)            - Population standard deviation
 * - statistics.variance(data)          - Sample variance
 * - statistics.pvariance(data)         - Population variance
 * - statistics.geometric_mean(data)    - Geometric mean
 * - statistics.harmonic_mean(data)     - Harmonic mean
 * - statistics.quantiles(data, n)      - Divide data into n equal parts
 * - statistics.min(data)               - Minimum value
 * - statistics.max(data)               - Maximum value
 * - statistics.sum(data)               - Sum of values
 * - statistics.count(data)             - Count of values
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Value make_error(const char* msg);

/* ── Helpers ── */

/* Extract numeric array from Value (expects array of numbers) */
static int extract_doubles(Value arr, double* out, int max_len) {
    if (arr.type != VAL_ARRAY) return -1;
    int len = arr.as.array.length < max_len ? arr.as.array.length : max_len;
    for (int i = 0; i < len; i++) {
        Value elem = arr.as.array.values[i];
        if (elem.type == VAL_INT) out[i] = (double)elem.as.integer;
        else if (elem.type == VAL_FLOAT) out[i] = elem.as.floating;
        else return -1; /* non-numeric element */
    }
    return len;
}

/* Comparator for qsort */
static int cmp_double(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

#define MAX_DATA 4096

/* ── Functions ── */

static Value native_mean(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.mean(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.mean: non-empty numeric array required");

    double sum = 0;
    for (int i = 0; i < n; i++) sum += data[i];
    return make_float(sum / n);
}

static Value native_median(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.median(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.median: non-empty numeric array required");

    qsort(data, n, sizeof(double), cmp_double);
    if (n % 2 == 1) return make_float(data[n / 2]);
    return make_float((data[n / 2 - 1] + data[n / 2]) / 2.0);
}

static Value native_median_low(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.median_low(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.median_low: non-empty numeric array required");

    qsort(data, n, sizeof(double), cmp_double);
    return make_float(data[(n - 1) / 2]);
}

static Value native_median_high(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.median_high(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.median_high: non-empty numeric array required");

    qsort(data, n, sizeof(double), cmp_double);
    return make_float(data[n / 2]);
}

static Value native_mode(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.mode(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.mode: non-empty numeric array required");

    qsort(data, n, sizeof(double), cmp_double);

    double mode_val = data[0];
    int max_count = 1, cur_count = 1;
    for (int i = 1; i < n; i++) {
        if (data[i] == data[i - 1]) {
            cur_count++;
        } else {
            if (cur_count > max_count) {
                max_count = cur_count;
                mode_val = data[i - 1];
            }
            cur_count = 1;
        }
    }
    if (cur_count > max_count) mode_val = data[n - 1];

    return make_float(mode_val);
}

static Value native_variance(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.variance(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 1) return make_error("statistics.variance: needs at least 2 values");

    double sum = 0;
    for (int i = 0; i < n; i++) sum += data[i];
    double mean = sum / n;

    double sq_sum = 0;
    for (int i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sq_sum += diff * diff;
    }
    return make_float(sq_sum / (n - 1)); /* sample variance */
}

static Value native_pvariance(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.pvariance(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.pvariance: non-empty numeric array required");

    double sum = 0;
    for (int i = 0; i < n; i++) sum += data[i];
    double mean = sum / n;

    double sq_sum = 0;
    for (int i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sq_sum += diff * diff;
    }
    return make_float(sq_sum / n); /* population variance */
}

static Value native_stdev(int argc, Value* argv) {
    Value v = native_variance(argc, argv);
    if (v.type == VAL_ERROR) return v;
    return make_float(sqrt(v.as.floating));
}

static Value native_pstdev(int argc, Value* argv) {
    Value v = native_pvariance(argc, argv);
    if (v.type == VAL_ERROR) return v;
    return make_float(sqrt(v.as.floating));
}

static Value native_geometric_mean(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.geometric_mean(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.geometric_mean: non-empty numeric array required");

    double log_sum = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] <= 0) return make_error("statistics.geometric_mean: all values must be positive");
        log_sum += log(data[i]);
    }
    return make_float(exp(log_sum / n));
}

static Value native_harmonic_mean(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.harmonic_mean(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.harmonic_mean: non-empty numeric array required");

    double inv_sum = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] == 0) return make_error("statistics.harmonic_mean: zero value not allowed");
        inv_sum += 1.0 / data[i];
    }
    return make_float((double)n / inv_sum);
}

static Value native_quantiles(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.quantiles(data [, n]) requires at least 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.quantiles: non-empty numeric array required");

    int divisions = 4; /* default: quartiles */
    if (argc >= 2 && argv[1].type == VAL_INT) divisions = (int)argv[1].as.integer;
    if (divisions < 1) divisions = 4;

    qsort(data, n, sizeof(double), cmp_double);

    /* Return array of cut points (n-1 values for n divisions) */
    int result_count = divisions - 1;
    /* For simplicity, return as array; caller creates the array value */
    /* We return a comma-separated string since creating arrays from C is complex */
    char buf[1024] = "";
    int pos = 0;
    for (int i = 1; i < divisions; i++) {
        double idx = (double)(n - 1) * i / divisions;
        int lo = (int)idx;
        int hi = lo + 1 < n ? lo + 1 : lo;
        double frac = idx - lo;
        double val = data[lo] + frac * (data[hi] - data[lo]);
        if (i > 1) pos += snprintf(buf + pos, sizeof(buf) - pos, ", ");
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%.6g", val);
    }
    return make_string(buf);
}

static Value native_stat_min(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.min(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.min: non-empty numeric array required");

    double m = data[0];
    for (int i = 1; i < n; i++) if (data[i] < m) m = data[i];
    return make_float(m);
}

static Value native_stat_max(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.max(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.max: non-empty numeric array required");

    double m = data[0];
    for (int i = 1; i < n; i++) if (data[i] > m) m = data[i];
    return make_float(m);
}

static Value native_stat_sum(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.sum(data) requires 1 argument");
    double data[MAX_DATA];
    int n = extract_doubles(argv[0], data, MAX_DATA);
    if (n <= 0) return make_error("statistics.sum: non-empty numeric array required");

    double s = 0;
    for (int i = 0; i < n; i++) s += data[i];
    return make_float(s);
}

static Value native_stat_count(int argc, Value* argv) {
    if (argc < 1) return make_error("statistics.count(data) requires 1 argument");
    if (argv[0].type != VAL_ARRAY) return make_error("statistics.count: array required");
    return make_int(argv[0].as.array.length);
}

/* ── Registration ── */
void stdlib_statistics_register(void) {
    register_native("statistics_mean", native_mean);
    register_native("statistics_median", native_median);
    register_native("statistics_median_low", native_median_low);
    register_native("statistics_median_high", native_median_high);
    register_native("statistics_mode", native_mode);
    register_native("statistics_stdev", native_stdev);
    register_native("statistics_pstdev", native_pstdev);
    register_native("statistics_variance", native_variance);
    register_native("statistics_pvariance", native_pvariance);
    register_native("statistics_geometric_mean", native_geometric_mean);
    register_native("statistics_harmonic_mean", native_harmonic_mean);
    register_native("statistics_quantiles", native_quantiles);
    register_native("statistics_min", native_stat_min);
    register_native("statistics_max", native_stat_max);
    register_native("statistics_sum", native_stat_sum);
    register_native("statistics_count", native_stat_count);
}
