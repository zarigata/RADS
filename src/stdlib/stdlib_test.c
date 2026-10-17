#include "stdlib_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Value value_clone(Value v); // owning-reference helper for natives
extern Value interpreter_execute_callback(Value callback, int argc, Value* args);
extern char* value_to_string(Value* value); // interpreter serializer (echo format)
extern Value native_str_starts_with(struct Interpreter* interp, int argc, Value* args);
extern Value native_str_ends_with(struct Interpreter* interp, int argc, Value* args);

/* ============================================================================
 * Python-semantics serialization (run 22). str()/print() must match CPython
 * byte-for-byte, while native `echo` keeps its own (%f) format — so str()
 * gets this parallel serializer instead of piggybacking value_to_string.
 * ==========================================================================*/

/* Python repr() of a double: shortest decimal that round-trips, Python's
 * fixed-vs-exponent threshold (exponent < -4 or >= 16 forces e-notation),
 * ".0" on integral floats, "inf"/"nan" spellings.
 *   3.14 → "3.14"   2.0 → "2.0"   0.1+0.2 → "0.30000000000000004"
 *   1e-7 → "1e-07"  1e16 → "1e+16"  1e15 → "1000000000000000.0"
 */
static void py_repr_double(double v, char* out, size_t outsz) {
    if (isnan(v)) { snprintf(out, outsz, "nan"); return; }
    if (isinf(v)) { snprintf(out, outsz, v < 0 ? "-inf" : "inf"); return; }
    int neg = signbit(v);
    double a = fabs(v);
    if (a == 0.0) { snprintf(out, outsz, neg ? "-0.0" : "0.0"); return; }

    /* shortest digit count (1..17) that round-trips through strtod */
    int prec = 17;
    char probe[64];
    for (int p = 1; p <= 17; p++) {
        snprintf(probe, sizeof probe, "%.*g", p, a);
        if (strtod(probe, NULL) == a) { prec = p; break; }
    }

    /* digits + decimal exponent from %e form (locale-independent shape) */
    char ebuf[64];
    snprintf(ebuf, sizeof ebuf, "%.*e", prec - 1, a);
    char digits[32]; int nd = 0;
    for (const char* p = ebuf; *p && *p != 'e' && *p != 'E'; p++) {
        if (isdigit((unsigned char)*p)) { if (nd < 31) digits[nd++] = *p; }
    }
    while (nd > 1 && digits[nd - 1] == '0') nd--; /* strip %e zero padding */
    digits[nd] = '\0';
    int exp10 = 0;
    const char* ep = strchr(ebuf, 'e');
    if (!ep) ep = strchr(ebuf, 'E');
    if (ep) exp10 = (int)strtol(ep + 1, NULL, 10);

    char body[64];
    if (exp10 < -4 || exp10 >= 16) {
        /* Python e-notation: d[.ddd]e±XX (exponent ≥ 2 digits) */
        if (nd == 1) snprintf(body, sizeof body, "%ce%+03d", digits[0], exp10);
        else snprintf(body, sizeof body, "%c.%se%+03d", digits[0], digits + 1, exp10);
    } else if (exp10 >= nd - 1) {
        /* integral value: all digits, then trailing zeros, then ".0" */
        snprintf(body, sizeof body, "%s", digits);
        size_t blen = strlen(body);
        for (int i = 0; i < exp10 - (nd - 1) && blen + 3 < sizeof body; i++)
            body[blen++] = '0';
        body[blen] = '\0';
        strcat(body, ".0");
    } else if (exp10 >= 0) {
        /* decimal point inside the digits: 3.14 */
        snprintf(body, sizeof body, "%.*s.%s", exp10 + 1, digits, digits + exp10 + 1);
    } else {
        /* leading zeros: 0.5, 0.0001 — zero-fill, NOT %*s (space-fill) */
        int zeros = -exp10 - 1;
        if (zeros < 0) zeros = 0;
        size_t blen = 0;
        if ((size_t)(2 + zeros + nd + 1) >= sizeof body) { snprintf(out, outsz, "%g", v); return; }
        body[blen++] = '0'; body[blen++] = '.';
        for (int i = 0; i < zeros; i++) body[blen++] = '0';
        memcpy(body + blen, digits, (size_t)nd); blen += (size_t)nd;
        body[blen] = '\0';
    }
    snprintf(out, outsz, "%s%s", neg ? "-" : "", body);
}

/* Python str()/repr() of a Value (they agree for our types). Strings are
 * passed through bare at top level (str("x") == "x") but quoted inside
 * arrays, exactly like CPython. */
static char* py_str_value(Value* v, int depth) {
    char buf[512];
    switch (v->type) {
        case VAL_BOOL:   return strdup(v->bool_val ? "True" : "False");
        case VAL_INT:    snprintf(buf, sizeof buf, "%lld", v->int_val); return strdup(buf);
        case VAL_FLOAT:  py_repr_double(v->float_val, buf, sizeof buf); return strdup(buf);
        case VAL_NULL:   return strdup("None");
        case VAL_STRING: {
            if (depth == 0) return strdup(v->string_val ? v->string_val : "");
            /* inside a container Python shows quotes */
            const char* s = v->string_val ? v->string_val : "";
            if (strchr(s, '\'') && !strchr(s, '"')) snprintf(buf, sizeof buf, "\"%s\"", s);
            else snprintf(buf, sizeof buf, "'%s'", s);
            return strdup(buf);
        }
        case VAL_ARRAY: {
            size_t cap = 64, len = 0;
            char* out = malloc(cap); out[0] = '\0';
            len += snprintf(out + len, cap - len, "%s", "[");
            for (size_t i = 0; i < v->array_val->count; i++) {
                if (i > 0) { len += snprintf(out + len, cap - len, "%s", ", "); }
                char* elem = py_str_value(&v->array_val->items[i], depth + 1);
                size_t elen = strlen(elem);
                while (len + elen + 2 > cap) { cap *= 2; out = realloc(out, cap); }
                memcpy(out + len, elem, elen); len += elen; out[len] = '\0';
                free(elem);
            }
            if (len + 2 > cap) { cap += 2; out = realloc(out, cap); }
            out[len] = ']'; out[len + 1] = '\0';
            return out;
        }
        case VAL_DICT: {
            size_t cap = 64, len = 0;
            char* out = malloc(cap); out[0] = '\0';
            len += snprintf(out + len, cap - len, "%s", "{");
            int first = 1;
            for (size_t i = 0; i + 1 < v->array_val->count; i += 2) {
                if (!first) { len += snprintf(out + len, cap - len, "%s", ", "); }
                first = 0;
                char* k = py_str_value(&v->array_val->items[i], depth + 1);
                char* val = py_str_value(&v->array_val->items[i + 1], depth + 1);
                size_t elen = strlen(k) + strlen(val) + 2;
                while (len + elen + 2 > cap) { cap *= 2; out = realloc(out, cap); }
                len += snprintf(out + len, cap - len, "%s: %s", k, val);
                free(k); free(val);
            }
            if (len + 2 > cap) { cap += 2; out = realloc(out, cap); }
            out[len] = '}'; out[len + 1] = '\0';
            return out;
        }
        default: {
            char* s = value_to_string(v); /* functions/structs: echo format */
            return s ? s : strdup("null");
        }
    }
}


// ANSI Color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_DIM     "\033[2m"

// Test state
typedef struct TestCase {
    char* name;
    Value callback;
    struct TestCase* next;
} TestCase;

typedef struct TestSuite {
    char* name;
    Value callback;
    TestCase* tests;
    struct TestSuite* next;
    int passed;
    int failed;
} TestSuite;

static TestSuite* test_suites = NULL;
static TestSuite* current_suite = NULL;
static TestCase* current_test = NULL;

static int total_passed = 0;
static int total_failed = 0;
static int total_suites = 0;

// Setup/Teardown callbacks
static Value before_each_callback = {0};
static Value after_each_callback = {0};
static bool has_before_each = false;
static bool has_after_each = false;

// Current assertion state
static bool current_test_passed = true;
static char* current_test_message = NULL;
// Run-level failure latch (run 23): once any test fails, the process exit
// code must be nonzero — run_tests.sh judges by rc, and green-on-red runs
// were invisible.
static bool any_test_failed = false;

// Helper: Create assertion result
static void set_test_result(bool passed, const char* message) {
    // Sticky failure (run 23): a later passing assertion must not erase an
    // earlier failure in the same test — expect chains reported false greens.
    if (!passed) {
        current_test_passed = false;
        if (current_test_message) {
            free(current_test_message);
        }
        current_test_message = message ? strdup(message) : NULL;
    }
}

// test.describe(name, callback) - Define a test suite
Value stdlib_test_describe(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: test.describe() requires 2 arguments (name, callback)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING || args[1].type != VAL_FUNCTION) {
        fprintf(stderr, "Error: test.describe() requires string name and function callback\n");
        return make_null();
    }
    
    // Create new suite
    TestSuite* suite = malloc(sizeof(TestSuite));
    suite->name = strdup(args[0].string_val);
    suite->callback = args[1];
    suite->tests = NULL;
    suite->next = NULL;
    suite->passed = 0;
    suite->failed = 0;
    
    // Add to list
    if (test_suites == NULL) {
        test_suites = suite;
    } else {
        TestSuite* current = test_suites;
        while (current->next) {
            current = current->next;
        }
        current->next = suite;
    }
    
    return make_null();
}

// test.it(name, callback) - Define a test case
Value stdlib_test_it(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: test.it() requires 2 arguments (name, callback)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING || args[1].type != VAL_FUNCTION) {
        fprintf(stderr, "Error: test.it() requires string name and function callback\n");
        return make_null();
    }
    
    // Create test case (we'll add it to current suite during run)
    TestCase* test = malloc(sizeof(TestCase));
    test->name = strdup(args[0].string_val);
    test->callback = args[1];
    test->next = NULL;
    
    // We need to store this for later execution
    // For now, just return - actual test collection happens at describe time
    if (current_suite != NULL) {
        if (current_suite->tests == NULL) {
            current_suite->tests = test;
        } else {
            TestCase* current = current_suite->tests;
            while (current->next) {
                current = current->next;
            }
            current->next = test;
        }
    }
    
    return make_null();
}

// test.expect(value) - Start an assertion
Value stdlib_test_expect(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: test.expect() requires 1 argument (value)\n");
        return make_null();
    }
    
    // Return the value so we can chain .toBe(), etc.
    return value_clone(args[0]); // owning ref (caller frees args)
}

// Helper: Compare values for equality
static bool values_equal(Value* a, Value* b) {
    if (a->type != b->type) return false;
    
    switch (a->type) {
        case VAL_NULL:
            return true;
        case VAL_BOOL:
            return a->bool_val == b->bool_val;
        case VAL_INT:
            return a->int_val == b->int_val;
        case VAL_FLOAT:
            return a->float_val == b->float_val;
        case VAL_STRING:
            return strcmp(a->string_val, b->string_val) == 0;
        case VAL_ARRAY:
            // Shallow comparison for arrays
            if (a->array_val == NULL || b->array_val == NULL) {
                return a->array_val == b->array_val;
            }
            if (a->array_val->count != b->array_val->count) return false;
            for (size_t i = 0; i < a->array_val->count; i++) {
                if (!values_equal(&a->array_val->items[i], &b->array_val->items[i])) {
                    return false;
                }
            }
            return true;
        default:
            return false;
    }
}

// Helper: Check if value is truthy
static bool is_truthy(Value* v) {
    switch (v->type) {
        case VAL_NULL:
            return false;
        case VAL_BOOL:
            return v->bool_val;
        case VAL_INT:
            return v->int_val != 0;
        case VAL_FLOAT:
            return v->float_val != 0.0;
        case VAL_STRING:
            return v->string_val != NULL && strlen(v->string_val) > 0;
        default:
            return true;
    }
}

// Helper: Value to string — now the interpreter's canonical serializer
// (echo format); arrays render as [1, 2, 3] in assertion failures too.
// (The old static 512-byte-buffer helper was replaced by the extern above.)

// test.expect(val).toBe(expected) - Strict equality assertion
Value stdlib_test_toBe(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: .toBe() requires internal arguments (actual, expected)\n");
        return make_null();
    }
    
    Value actual = args[0];
    Value expected = args[1];
    
    bool passed = values_equal(&actual, &expected);
    
    if (!passed) {
        char* actual_str = value_to_string(&actual);
        char* expected_str = value_to_string(&expected);
        char message[256];
        snprintf(message, sizeof(message), "Expected %s, got %s", expected_str, actual_str);
        set_test_result(false, strdup(message));
        free(actual_str);
        free(expected_str);
    } else {
        set_test_result(true, NULL);
    }
    
    return make_bool(passed);
}

// test.expect(val).toEqual(expected) - Deep equality assertion
Value stdlib_test_toEqual(Interpreter* interp, int argc, Value* args) {
    // Same as toBe for now (deep compare)
    return stdlib_test_toBe(interp, argc, args);
}

// test.expect(val).toBeTruthy() - Truthy assertion
Value stdlib_test_toBeTruthy(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: .toBeTruthy() requires value\n");
        return make_null();
    }
    
    bool passed = is_truthy(&args[0]);
    
    if (!passed) {
        char* actual_str = value_to_string(&args[0]);
        char message[256];
        snprintf(message, sizeof(message), "Expected truthy value, got %s", actual_str);
        set_test_result(false, strdup(message));
        free(actual_str);
    } else {
        set_test_result(true, NULL);
    }
    
    return make_bool(passed);
}

// test.expect(val).toBeFalsy() - Falsy assertion
Value stdlib_test_toBeFalsy(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: .toBeFalsy() requires value\n");
        return make_null();
    }
    
    bool passed = !is_truthy(&args[0]);
    
    if (!passed) {
        char* actual_str = value_to_string(&args[0]);
        char message[256];
        snprintf(message, sizeof(message), "Expected falsy value, got %s", actual_str);
        set_test_result(false, strdup(message));
        free(actual_str);
    } else {
        set_test_result(true, NULL);
    }
    
    return make_bool(passed);
}

// test.expect(val).toBeNull() - Null assertion
Value stdlib_test_toBeNull(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: .toBeNull() requires value\n");
        return make_null();
    }
    
    bool passed = args[0].type == VAL_NULL;
    
    if (!passed) {
        char* actual_str = value_to_string(&args[0]);
        char message[256];
        snprintf(message, sizeof(message), "Expected null, got %s", actual_str);
        set_test_result(false, strdup(message));
        free(actual_str);
    } else {
        set_test_result(true, NULL);
    }
    
    return make_bool(passed);
}

// test.expect(val).toBeGreaterThan(expected)
Value stdlib_test_toBeGreaterThan(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: .toBeGreaterThan() requires 2 arguments\n");
        return make_null();
    }
    
    bool passed = false;
    
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        passed = args[0].int_val > args[1].int_val;
    } else if (args[0].type == VAL_FLOAT && args[1].type == VAL_FLOAT) {
        passed = args[0].float_val > args[1].float_val;
    } else if (args[0].type == VAL_INT && args[1].type == VAL_FLOAT) {
        passed = args[0].int_val > args[1].float_val;
    } else if (args[0].type == VAL_FLOAT && args[1].type == VAL_INT) {
        passed = args[0].float_val > args[1].int_val;
    }
    
    if (!passed) {
        char* actual_str = value_to_string(&args[0]);
        char* expected_str = value_to_string(&args[1]);
        char message[256];
        snprintf(message, sizeof(message), "Expected %s > %s", actual_str, expected_str);
        set_test_result(false, strdup(message));
        free(actual_str);
        free(expected_str);
    } else {
        set_test_result(true, NULL);
    }
    
    return make_bool(passed);
}

// test.expect(val).toBeLessThan(expected)
Value stdlib_test_toBeLessThan(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: .toBeLessThan() requires 2 arguments\n");
        return make_null();
    }
    
    bool passed = false;
    
    if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
        passed = args[0].int_val < args[1].int_val;
    } else if (args[0].type == VAL_FLOAT && args[1].type == VAL_FLOAT) {
        passed = args[0].float_val < args[1].float_val;
    } else if (args[0].type == VAL_INT && args[1].type == VAL_FLOAT) {
        passed = args[0].int_val < args[1].float_val;
    } else if (args[0].type == VAL_FLOAT && args[1].type == VAL_INT) {
        passed = args[0].float_val < args[1].int_val;
    }
    
    if (!passed) {
        char* actual_str = value_to_string(&args[0]);
        char* expected_str = value_to_string(&args[1]);
        char message[256];
        snprintf(message, sizeof(message), "Expected %s < %s", actual_str, expected_str);
        set_test_result(false, strdup(message));
        free(actual_str);
        free(expected_str);
    } else {
        set_test_result(true, NULL);
    }
    
    return make_bool(passed);
}

// test.beforeEach(callback) - Setup before each test
Value stdlib_test_beforeEach(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_FUNCTION) {
        fprintf(stderr, "Error: test.beforeEach() requires a function argument\n");
        return make_null();
    }
    
    before_each_callback = args[0];
    has_before_each = true;
    
    return make_null();
}

// test.afterEach(callback) - Cleanup after each test
Value stdlib_test_afterEach(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_FUNCTION) {
        fprintf(stderr, "Error: test.afterEach() requires a function argument\n");
        return make_null();
    }
    
    after_each_callback = args[0];
    has_after_each = true;
    
    return make_null();
}

// test.skip(name, callback) - Skip a test
Value stdlib_test_skip(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    // Just do nothing - skip the test
    return make_null();
}

// test.run() - Execute all tests
Value stdlib_test_run(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    printf("\n");
    printf("%s╔════════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║          RADS Test Framework              ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════╝%s\n\n", COLOR_CYAN, COLOR_RESET);
    
    TestSuite* suite = test_suites;
    total_passed = 0;
    total_failed = 0;
    total_suites = 0;
    
    while (suite != NULL) {
        printf("%s%s%s\n", COLOR_CYAN, suite->name, COLOR_RESET);
        current_suite = suite;
        suite->passed = 0;
        suite->failed = 0;
        
        // First, run the describe callback to collect test cases
        interpreter_execute_callback(suite->callback, 0, NULL);
        
        // Then run each test
        TestCase* test = suite->tests;
        while (test != NULL) {
            // Run beforeEach if set
            if (has_before_each) {
                interpreter_execute_callback(before_each_callback, 0, NULL);
            }
            
            // Reset test state
            current_test_passed = true;
            if (current_test_message) {
                free(current_test_message);
                current_test_message = NULL;
            }
            
            // Run the test
            interpreter_execute_callback(test->callback, 0, NULL);
            
            // Run afterEach if set
            if (has_after_each) {
                interpreter_execute_callback(after_each_callback, 0, NULL);
            }
            
            // Check result
            if (current_test_passed) {
                printf("  %s✓%s %s\n", COLOR_GREEN, COLOR_RESET, test->name);
                suite->passed++;
                total_passed++;
            } else {
                any_test_failed = true;
                printf("  %s✗%s %s\n", COLOR_RED, COLOR_RESET, test->name);
                if (current_test_message) {
                    printf("    %s└─ %s%s\n", COLOR_DIM, current_test_message, COLOR_RESET);
                }
                suite->failed++;
                total_failed++;
            }
            
            test = test->next;
        }
        
        printf("\n");
        total_suites++;
        suite = suite->next;
    }
    
    // Summary
    printf("%s────────────────────────────────────────────%s\n", COLOR_DIM, COLOR_RESET);
    printf("\n");
    
    if (total_failed == 0) {
        printf("%s  ✓ All tests passed!%s\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("%s  ✗ %d test(s) failed%s\n", COLOR_RED, total_failed, COLOR_RESET);
    }
    
    printf("%s  Tests:  %d passed, %d failed, %d total%s\n", 
           COLOR_DIM, total_passed, total_failed, total_passed + total_failed, COLOR_RESET);
    printf("%s  Suites: %d%s\n\n", COLOR_DIM, total_suites, COLOR_RESET);
    
    return make_int(total_failed == 0 ? 0 : 1);
}

// Python-style bare assert(cond, msg?) — prints a red FAIL line and returns
// an int so scripts can check it (0 = pass, 1 = fail). Matches the module's
// existing natives: stderr on bad usage, make_* returns.
Value stdlib_test_assert(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: assert() requires a condition\n");
        return make_int(1);
    }
    bool cond = false;
    switch (args[0].type) {
        case VAL_BOOL:   cond = args[0].bool_val; break;
        case VAL_INT:    cond = args[0].int_val != 0; break;
        case VAL_FLOAT:  cond = args[0].float_val != 0.0; break;
        case VAL_STRING: cond = args[0].string_val && args[0].string_val[0] != '\0'; break;
        case VAL_NULL:   cond = false; break;
        default:         cond = true; break; // arrays/structs/functions: truthy
    }
    if (!cond) {
        if (argc >= 2 && args[1].type == VAL_STRING) {
            fprintf(stderr, "%sAssertion failed:%s %s\n", COLOR_RED, COLOR_RESET, args[1].string_val);
        } else {
            fprintf(stderr, "%sAssertion failed%s\n", COLOR_RED, COLOR_RESET);
        }
    }
    return make_int(cond ? 0 : 1);
}

// Python-style str(x) — stringify any value (mirror of the + concat coercion).
Value stdlib_test_str(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("");
    // Python semantics (run 22): shortest round-trip float repr ("3.14",
    // "2.0", "0.30000000000000004"), True/False, None, quoted strings
    // inside arrays. Native `echo` keeps its own format — this is str().
    char* s = py_str_value(&args[0], 0);
    Value out = make_string(s ? s : "");
    free(s);
    return out;
}

// Python-style len(x) — string length or array count (run 22).
Value stdlib_test_len(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: len() requires an argument\n");
        return make_int(0);
    }
    if (args[0].type == VAL_STRING) {
        return make_int((long long)strlen(args[0].string_val));
    }
    if (args[0].type == VAL_ARRAY && args[0].array_val) {
        return make_int((long long)args[0].array_val->count);
    }
    if (args[0].type == VAL_DICT && args[0].array_val) {
        return make_int((long long)(args[0].array_val->count / 2));
    }
    fprintf(stderr, "Error: len() requires a string, array or dict\n");
    return make_int(0);
}

// pyfmt.fixed(x, n) — Python "{x:.nf}" semantics: n decimals, round-half-
// even like printf, and Python's "inf"/"nan". Built for the transpiled
// f-string {pi:.2f} specs; echo keeps its own format.
Value stdlib_pyfmt_fixed(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    // Direct member access — callers (transpiled f-string specs) pass
    // numeric args; value_to_double is not on this TU's extern list.
    double v = (args[0].type == VAL_INT) ? (double)args[0].int_val
                                         : args[0].float_val;
    long long n = (args[1].type == VAL_INT) ? args[1].int_val : 0;
    if (n < 0) n = 0;
    if (n > 60) n = 60;
    char buf[512];
    if (v != v) { snprintf(buf, sizeof buf, "nan"); }
    else if (v == (1.0/0.0) || v == (-1.0/0.0)) { snprintf(buf, sizeof buf, v < 0 ? "-inf" : "inf"); }
    else snprintf(buf, sizeof buf, "%.*f", (int)n, v);
    return make_string(buf);
}

// Python-style type(x) — returns the value's type name as a string.
Value stdlib_test_type(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("null");
    switch (args[0].type) {
        case VAL_INT:    return make_string("integer");
        case VAL_FLOAT:  return make_string("float");
        case VAL_STRING: return make_string("string");
        case VAL_BOOL:   return make_string("bool");
        case VAL_NULL:   return make_string("null");
        case VAL_ARRAY:  return make_string("array");
        case VAL_DICT:   return make_string("dict");
        case VAL_FUNCTION: return make_string("function");
        case VAL_STRUCT_DEF: return make_string("struct_def");
        case VAL_STRUCT_INSTANCE: return make_string("struct");
        default:         return make_string("unknown");
    }
}

// Python-style abs(x) — int/float absolute value.
Value stdlib_test_abs(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_int(0);
    if (args[0].type == VAL_INT) {
        long long v = args[0].int_val;
        return make_int(v < 0 ? -v : v);
    }
    if (args[0].type == VAL_FLOAT) {
        double v = args[0].float_val;
        return make_float(v < 0.0 ? -v : v);
    }
    fprintf(stderr, "Error: abs() requires a number\n");
    return make_int(0);
}

bool stdlib_test_failed(void) { return any_test_failed; }

void stdlib_test_register(void) {
    // pythonic bare aliases (run 17: test_time needs assert/str/type/abs at top level)
    register_native("assert", stdlib_test_assert);
    register_native("str", stdlib_test_str);
    register_native("type", stdlib_test_type);
    register_native("abs", stdlib_test_abs);
    register_native("len", stdlib_test_len);           // run 22: bare Python len()
    register_native("pyfmt.fixed", stdlib_pyfmt_fixed); // run 22: {x:.nf} for f-strings
    register_native("str.startswith", native_str_starts_with); // Python spellings
    register_native("str.endswith", native_str_ends_with);
    register_native("test.describe", stdlib_test_describe);
    register_native("test.it", stdlib_test_it);
    register_native("test.expect", stdlib_test_expect);
    // Bare aliases so chained calls resolve: expect(x).toBe(y) dispatches the
    // member on a call-result value with the receiver prepended as args[0].
    register_native("toBe", stdlib_test_toBe);
    register_native("toEqual", stdlib_test_toEqual);
    register_native("toBeTruthy", stdlib_test_toBeTruthy);
    register_native("toBeFalsy", stdlib_test_toBeFalsy);
    register_native("toBeNull", stdlib_test_toBeNull);
    register_native("toBeGreaterThan", stdlib_test_toBeGreaterThan);
    register_native("toBeLessThan", stdlib_test_toBeLessThan);
    register_native("test.toBe", stdlib_test_toBe);
    register_native("test.toEqual", stdlib_test_toEqual);
    register_native("test.toBeTruthy", stdlib_test_toBeTruthy);
    register_native("test.toBeFalsy", stdlib_test_toBeFalsy);
    register_native("test.toBeNull", stdlib_test_toBeNull);
    register_native("test.toBeGreaterThan", stdlib_test_toBeGreaterThan);
    register_native("test.toBeLessThan", stdlib_test_toBeLessThan);
    register_native("test.beforeEach", stdlib_test_beforeEach);
    register_native("test.afterEach", stdlib_test_afterEach);
    register_native("test.skip", stdlib_test_skip);
    register_native("test.run", stdlib_test_run);
    
    // Initialize state
    test_suites = NULL;
    current_suite = NULL;
    total_passed = 0;
    total_failed = 0;
    has_before_each = false;
    has_after_each = false;
}