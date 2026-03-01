#include "stdlib_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Value interpreter_execute_callback(Value callback, int argc, Value* args);

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

// Helper: Create assertion result
static void set_test_result(bool passed, const char* message) {
    current_test_passed = passed;
    if (current_test_message) {
        free(current_test_message);
    }
    current_test_message = message ? strdup(message) : NULL;
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
    return args[0];
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

// Helper: Value to string
static char* value_to_string(Value* v) {
    static char buffer[512];
    
    switch (v->type) {
        case VAL_NULL:
            return strdup("null");
        case VAL_BOOL:
            return strdup(v->bool_val ? "true" : "false");
        case VAL_INT:
            snprintf(buffer, sizeof(buffer), "%lld", v->int_val);
            return strdup(buffer);
        case VAL_FLOAT:
            snprintf(buffer, sizeof(buffer), "%g", v->float_val);
            return strdup(buffer);
        case VAL_STRING:
            return strdup(v->string_val);
        default:
            return strdup("<value>");
    }
}

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

void stdlib_test_register(void) {
    register_native("test.describe", stdlib_test_describe);
    register_native("test.it", stdlib_test_it);
    register_native("test.expect", stdlib_test_expect);
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