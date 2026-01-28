#include "test.h"
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// TEST FRAMEWORK - Discovery, Parallel Execution, Output, Mocking
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// Test discovery patterns
typedef enum {
    TEST_PATTERN_BASIC = "*_test.rads",
    TEST_PATTERN_UNIT = "*_unit.rads",
    TEST_PATTERN_INTEGRATION = "*_integration.rads"
    TEST_TAG_SLOW = "@slow",
    TEST_TAG_INTEGRATION = "@integration"
} TestPattern;

// Test file metadata
typedef struct {
    char* file_path;
    char* test_name;
    TestPattern pattern;
    char** tags;
    int tag_count;
    struct TestFile* next;
} TestFile;

// Test discovery state
typedef struct {
    TestFile* files;
    size_t file_count;
    int total_tests;
    int tag_count;
} TestDiscoveryState;

// Test execution mode
typedef enum {
    EXEC_MODE_SERIAL = 0,
    EXEC_MODE_PARALLEL = 1
} ExecutionMode;

// Test output format
typedef enum {
    OUTPUT_FORMAT_TEXT = 0,
    OUTPUT_FORMAT_JUNIT = 1,
    OUTPUT_FORMAT_JSON = 2
} OutputFormat;

// Mock configuration
typedef struct {
    char* function_name;
    char* original_function;
    int call_count;
    int return_value;
} MockConfig;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize test discovery
 */
TestDiscoveryState* test_discovery_init(void);

/**
 * Discover test files based on patterns
 *
 * @param state Test discovery state
 * @param pattern Test file pattern (*_test.rads, *unit.rads, etc.)
 * @param tags Optional tags to filter (array of char*)
 * @return Number of test files discovered
 */
size_t test_discover_files(
    TestDiscoveryState* state,
    const char* pattern,
    char** tags
);

/**
 * Execute tests
 *
 * @param state Test discovery state
 * @param mode Execution mode (serial or parallel)
 * @param format Output format (text, JUnit XML, JSON)
 * @param filter_tags Optional tags to include/exclude
 * @return Exit code (0 = success, non-zero on failure)
 */
int test_execute(
    TestDiscoveryState* state,
    ExecutionMode mode,
    OutputFormat format,
    char** filter_tags
);

/**
 * Execute single test file
 *
 * @param test_path Path to test file
 * @return Exit code
 */
int test_run_single(const char* test_path);

/**
 * Mock a stdlib function
 *
 * @param function_name Name of function to mock
 * @param mock_function Mock implementation
 * @param return_value Value to return
 */
void test_mock_function(
    const char* function_name,
    Value* (*mock_function)(Value*, int, Value**),
    Value* return_value
);

/**
 * Get test results summary
 *
 * @param state Test discovery state
 * @param[out] total_tests Total tests run
 * @param[out] passed Tests that passed
 * @param[out] failed Tests that failed
 */
void test_get_summary(
    TestDiscoveryState* state,
    int* total_tests,
    int* passed,
    int* failed
);
