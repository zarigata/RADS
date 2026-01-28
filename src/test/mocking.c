#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "core/interpreter.h"

typedef struct MockEntry {
    char* function_name;
    Value* (*original_func)(Value*, int, Value*);
    Value* (*mock_func)(Value*, int, Value*);
    Value* mock_return_value;
    int call_count;
    struct MockEntry* next;
} MockEntry;

static MockEntry* mock_registry = NULL;
static int mock_count = 0;

void test_mock_init() {
    mock_registry = NULL;
    mock_count = 0;
}

void test_mock_register(
    const char* function_name,
    Value* (*original_func)(Value*, int, Value*),
    Value* (*mock_func)(Value*, int, Value*)
) {
    MockEntry* entry = malloc(sizeof(MockEntry));
    if (!entry) {
        fprintf(stderr, "Failed to allocate mock entry\n");
        return;
    }

    entry->function_name = strdup(function_name);
    entry->original_func = original_func;
    entry->mock_func = mock_func;
    entry->call_count = 0;
    entry->mock_return_value = NULL;
    entry->next = mock_registry;
    mock_registry = entry;
    mock_count++;

    printf("Registered mock for function: %s\n", function_name);
}

void test_mock_unregister(const char* function_name) {
    MockEntry** current = &mock_registry;
    MockEntry* prev = NULL;

    while (*current) {
        if (strcmp((*current)->function_name, function_name) == 0) {
            if (prev) {
                prev->next = (*current)->next;
            } else {
                mock_registry = (*current)->next;
            }

            free((*current)->function_name);
            if ((*current)->mock_return_value) {
                free((*current)->mock_return_value);
            }
            free(*current);
            mock_count--;

            printf("Unregistered mock for function: %s\n", function_name);
            return;
        }
        prev = *current;
        current = &((*current)->next);
    }

    printf("No mock found for function: %s\n", function_name);
}

void test_mock_set_return(const char* function_name, Value* return_value) {
    MockEntry* entry = mock_registry;
    while (entry) {
        if (strcmp(entry->function_name, function_name) == 0) {
            entry->mock_return_value = return_value;
            printf("Set mock return value for %s\n", function_name);
            return;
        }
        entry = entry->next;
    }
}

void test_mock_clear_all() {
    while (mock_registry) {
        MockEntry* next = mock_registry->next;
        free(mock_registry->function_name);
        if (mock_registry->mock_return_value) {
            free(mock_registry->mock_return_value);
        }
        free(mock_registry);
        mock_registry = next;
    }

    mock_count = 0;
    printf("Cleared all %d mocks\n", mock_count);
}

int test_mock_get_call_count(const char* function_name) {
    MockEntry* entry = mock_registry;
    while (entry) {
        if (strcmp(entry->function_name, function_name) == 0) {
            return entry->call_count;
        }
        entry = entry->next;
    }
    return 0;
}

void test_mock_reset_count(const char* function_name) {
    MockEntry* entry = mock_registry;
    while (entry) {
        if (strcmp(entry->function_name, function_name) == 0) {
            entry->call_count = 0;
            printf("Reset call count for %s\n", function_name);
            return;
        }
        entry = entry->next;
    }
}

Value test_mock_call(Value* args, int argc, Value** result_out) {
    if (argc < 1) {
        fprintf(stderr, "Mock call requires function name\n");
        return (Value){0};
    }

    const char* function_name = args[0].string_val;
    MockEntry* entry = mock_registry;

    while (entry) {
        if (strcmp(entry->function_name, function_name) == 0) {
            entry->call_count++;
            printf("Mocking call to %s (call #%d)\n", function_name, entry->call_count);

            if (entry->mock_return_value) {
                *result_out = *entry->mock_return_value;
            } else {
                *result_out = entry->mock_func(args + 1, argc - 1, result_out);
            }

            return *result_out;
        }
        entry = entry->next;
    }

    printf("No mock found for: %s\n", function_name);
    return (Value){0};
}

void test_mock_list() {
    printf("\n");
    printf("========================================\n");
    printf("Active Mocks (%d)\n", mock_count);
    printf("========================================\n");

    if (mock_count == 0) {
        printf("No active mocks\n");
        return;
    }

    MockEntry* entry = mock_registry;
    int index = 1;

    while (entry) {
        printf("%d. Function: %s\n", index, entry->function_name);
        printf("   Calls made: %d\n", entry->call_count);
        printf("   Has custom return: %s\n", entry->mock_return_value ? "Yes" : "No");

        index++;
        entry = entry->next;
    }
}

void test_mock_cleanup() {
    test_mock_clear_all();
    printf("Mocking system cleaned up\n");
}

int test_mock_registry_count() {
    return mock_count;
}

int test_mock_is_active(const char* function_name) {
    MockEntry* entry = mock_registry;
    while (entry) {
        if (strcmp(entry->function_name, function_name) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}
