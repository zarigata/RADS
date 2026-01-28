#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include "core/interpreter.h"

#define MAX_TESTS 128
#define MAX_PATH_LEN 512

typedef struct {
    char name[MAX_PATH_LEN];
    char path[MAX_PATH_LEN];
    int passed;
    int failed;
    char error_msg[256];
} TestResult;

typedef struct {
    TestResult tests[MAX_TESTS];
    int count;
    int passed;
    int failed;
} TestSuite;

TestSuite g_suite;
pthread_mutex_t suite_mutex;

void init_test_suite() {
    g_suite.count = 0;
    g_suite.passed = 0;
    g_suite.failed = 0;
    pthread_mutex_init(&suite_mutex, NULL);
}

void add_test_result(const char* name, const char* path, int passed, const char* error) {
    pthread_mutex_lock(&suite_mutex);

    if (g_suite.count < MAX_TESTS) {
        strncpy(g_suite.tests[g_suite.count].name, name, MAX_PATH_LEN - 1);
        strncpy(g_suite.tests[g_suite.count].path, path, MAX_PATH_LEN - 1);
        g_suite.tests[g_suite.count].passed = passed;
        g_suite.tests[g_suite.count].failed = !passed;

        if (passed) {
            g_suite.passed++;
        } else {
            g_suite.failed++;
            if (error) {
                strncpy(g_suite.tests[g_suite.count].error_msg, error, 255);
            } else {
                strcpy(g_suite.tests[g_suite.count].error_msg, "Unknown error");
            }
        }

        g_suite.count++;
    }

    pthread_mutex_unlock(&suite_mutex);
}

typedef struct {
    const char* test_name;
    const char* test_path;
} TestTask;

void* run_single_test(void* arg) {
    TestTask* task = (TestTask*)arg;

    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "./bin/rads %s", task->test_path);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        add_test_result(task->test_name, task->test_path, 0, "Failed to execute");
        return NULL;
    }

    char output[4096];
    int success = 1;
    char error_msg[256] = {0};

    while (fgets(output, sizeof(output), fp)) {
        if (strstr(output, "PASSED")) {
            success = 1;
        } else if (strstr(output, "FAILED")) {
            success = 0;
            strncpy(error_msg, output + 6, 255);
        }
    }

    int status = pclose(fp);
    if (status != 0 && success) {
        success = 0;
        strcpy(error_msg, "Runtime error");
    }

    add_test_result(task->test_name, task->test_path, success, success ? NULL : error_msg);

    return NULL;
}

void discover_tests(const char* dir) {
    DIR* d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Failed to open directory: %s\n", dir);
        return;
    }

    struct dirent* dirp;
    while ((dirp = readdir(d)) != NULL) {
        if (strstr(dirp->d_name, ".rads")) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path), "%s/%s", dir, dirp->d_name);
            add_test_result(dirp->d_name, path, 0, "Not run yet");
        }
    }

    closedir(d);
}

void run_tests_parallel(int max_threads) {
    if (g_suite.count == 0) {
        printf("No tests discovered!\n");
        return;
    }

    printf("Running %d tests with %d parallel workers\n", g_suite.count, max_threads);

    pthread_t* threads = malloc(g_suite.count * sizeof(pthread_t));
    TestTask* tasks = malloc(g_suite.count * sizeof(TestTask));

    for (int i = 0; i < g_suite.count; i++) {
        strncpy(tasks[i].test_name, g_suite.tests[i].name, MAX_PATH_LEN - 1);
        strncpy(tasks[i].test_path, g_suite.tests[i].path, MAX_PATH_LEN - 1);
    }

    int thread_count = 0;
    int test_index = 0;

    while (test_index < g_suite.count) {
        int batch_size = max_threads;
        int remaining = g_suite.count - test_index;
        if (remaining < batch_size) {
            batch_size = remaining;
        }

        for (int i = 0; i < batch_size; i++) {
            int idx = test_index + i;
            if (pthread_create(&threads[thread_count], NULL, run_single_test, &tasks[idx]) != 0) {
                fprintf(stderr, "Failed to create thread for test: %s\n", tasks[idx].test_name);
                add_test_result(tasks[idx].test_name, tasks[idx].test_path, 0, "Thread creation failed");
            } else {
                thread_count++;
            }
        }

        for (int i = 0; i < batch_size; i++) {
            int idx = test_index + i;
            if (idx < thread_count) {
                pthread_join(threads[idx], NULL);
            }
        }

        test_index += batch_size;
    }

    free(threads);
    free(tasks);
}

void print_results() {
    printf("\n");
    printf("========================================\n");
    printf("Test Results\n");
    printf("========================================\n");
    printf("Total: %d\n", g_suite.count);
    printf("Passed: %d\n", g_suite.passed);
    printf("Failed: %d\n", g_suite.failed);
    printf("========================================\n");

    if (g_suite.failed > 0) {
        printf("✅ All tests passed!\n");
    } else {
        printf("\n❌ Failed tests:\n");
        for (int i = 0; i < g_suite.count; i++) {
            if (!g_suite.tests[i].passed) {
                printf("  - %s: %s\n", g_suite.tests[i].name, g_suite.tests[i].error_msg);
            }
        }
    }
}

void cleanup() {
    pthread_mutex_destroy(&suite_mutex);
}

int main(int argc, char* argv[]) {
    int max_threads = 4;

    if (argc > 1) {
        max_threads = atoi(argv[1]);
        if (max_threads < 1) max_threads = 1;
        if (max_threads > 32) max_threads = 32;
    }

    const char* test_dir = "tests";
    if (argc > 2) {
        test_dir = argv[2];
    }

    init_test_suite();
    discover_tests(test_dir);
    run_tests_parallel(max_threads);
    print_results();
    cleanup();

    return g_suite.failed > 0 ? 1 : 0;
}
