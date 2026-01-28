#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
    OUTPUT_PLAIN = 0,
    OUTPUT_VERBOSE = 1,
    OUTPUT_QUIET = 2,
    OUTPUT_JSON = 3,
    OUTPUT_JUNIT = 4
} OutputFormat;

typedef struct {
    OutputFormat format;
    int show_timestamps;
    int show_durations;
    int colorize;
} OutputConfig;

static OutputConfig g_output = {
    .format = OUTPUT_PLAIN,
    .show_timestamps = 0,
    .show_durations = 0,
    .colorize = 1
};

const char* COLOR_RESET = "\033[0m";
const char* COLOR_GREEN = "\033[32m";
const char* COLOR_RED = "\033[31m";
const char* COLOR_YELLOW = "\033[33m";
const char* COLOR_CYAN = "\033[36m";

void print_test_header(const char* name) {
    if (g_output.format == OUTPUT_VERBOSE) {
        printf("\n=== %s ===\n", name);
    } else if (g_output.format == OUTPUT_PLAIN) {
        printf("\n--- %s ---\n", name);
    }
}

void print_test_result(int passed, const char* test_name, double duration) {
    if (g_output.format == OUTPUT_QUIET) return;

    if (passed) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
    }

    if (g_output.format == OUTPUT_VERBOSE && g_output.show_durations) {
        printf("  Duration: %.3f ms\n", duration);
    }

    if (g_output.format == OUTPUT_VERBOSE && g_output.show_timestamps) {
        char timestamp[32];
        time_t now = time(NULL);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        printf("  Time: %s\n", timestamp);
    }
}
}

void print_test_result(int passed, const char* test_name, double duration) {
    if (g_output.format == OUTPUT_QUIET) return;

    if (passed) {
        colorize("✓ PASSED", COLOR_GREEN);
    } else {
        colorize("✗ FAILED", COLOR_RED);
    }

    if (g_output.format == OUTPUT_VERBOSE && g_output.show_durations) {
        printf("  Duration: %.3f ms\n", duration);
    }

    if (g_output.format == OUTPUT_VERBOSE && g_output.show_timestamps) {
        char timestamp[32];
        time_t now = time(NULL);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        printf("  Time: %s\n", timestamp);
    }
}

void print_summary(int total, int passed, int failed) {
    printf("\n");
    printf("======================================\n");
    printf("Test Summary\n");
    printf("======================================\n");

    if (g_output.format == OUTPUT_JSON) {
        printf("{\n");
        printf("  \"total\": %d,\n", total);
        printf("  \"passed\": %d,\n", passed);
        printf("  \"failed\": %d,\n", failed);
        printf("  \"success_rate\": %.2f\n", total > 0 ? (double)passed / total * 100.0 : 0.0);
        printf("}\n");
    } else if (g_output.format == OUTPUT_JUNIT) {
        printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        printf("<testsuite tests=\"%d\" failures=\"%d\" errors=\"0\">\n", total, failed);
        printf("</testsuite>\n");
    } else {
        printf("Total:   %d\n", total);
        printf("Passed:  %d\n", passed);
        printf("Failed:  %d\n", failed);

        if (total > 0) {
            double rate = (double)passed / total * 100.0;
            printf("Success: %.1f%%\n", rate);
        }

        if (failed > 0) {
            printf("\n" COLOR_RED "Some tests failed!" COLOR_RESET "\n");
        } else {
            printf("\n" COLOR_GREEN "All tests passed!" COLOR_RESET "\n");
        }
    }
}

void parse_output_args(int argc, char* argv[], int* test_start_idx) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            g_output.format = OUTPUT_VERBOSE;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            g_output.format = OUTPUT_QUIET;
        } else if (strcmp(argv[i], "--json") == 0) {
            g_output.format = OUTPUT_JSON;
            g_output.colorize = 0;
        } else if (strcmp(argv[i], "--junit") == 0) {
            g_output.format = OUTPUT_JUNIT;
            g_output.colorize = 0;
        } else if (strcmp(argv[i], "--timestamps") == 0) {
            g_output.show_timestamps = 1;
        } else if (strcmp(argv[i], "--durations") == 0) {
            g_output.show_durations = 1;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            g_output.colorize = 0;
        } else {
            *test_start_idx = i;
            return;
        }
    }

    *test_start_idx = 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s [options] <test_file>...\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --verbose      Show verbose output\n");
        printf("  --quiet        Suppress non-essential output\n");
        printf("  --json          Output results as JSON\n");
        printf("  --junit         Output results as JUnit XML\n");
        printf("  --timestamps    Show timestamps for each test\n");
        printf("  --durations     Show duration for each test\n");
        printf("  --no-color      Disable colored output\n");
        return 1;
    }

    int test_start_idx = 1;
    parse_output_args(argc, argv, &test_start_idx);

    clock_t start_total = clock();

    printf("RADS v0.0.7 Test Output Formatter\n");
    printf("====================================\n\n");

    int total = 0;
    int passed = 0;
    int failed = 0;

    print_test_header("Running Tests");

    for (int i = test_start_idx; i < argc; i++) {
        const char* test_file = argv[i];
        clock_t test_start = clock();

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "./bin/rads %s 2>&1", test_file);

        FILE* fp = popen(cmd, "r");
        if (!fp) {
            fprintf(stderr, "Failed to run test: %s\n", test_file);
            failed++;
            continue;
        }

        char line[256];
        int test_passed = 0;
        char test_name[128] = {0};

        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "PASSED") != NULL) {
                test_passed = 1;
            } else if (strstr(line, "FAILED") != NULL) {
                test_passed = 0;
            } else if (strstr(line, "Testing:") != NULL) {
                sscanf(line, "Testing:%*[ \t]%127[^\n]", test_name);
            }
        }

        pclose(fp);

        clock_t test_end = clock();
        double duration = ((double)(test_end - test_start)) / CLOCKS_PER_SEC * 1000.0;

        print_test_result(test_passed, test_name, duration);

        total++;
        if (test_passed) {
            passed++;
        } else {
            failed++;
        }
    }

    print_summary(total, passed, failed);

    return failed > 0 ? 1 : 0;
}
