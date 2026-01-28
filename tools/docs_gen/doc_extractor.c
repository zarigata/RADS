#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#define MAX_DOC_LENGTH 4096
#define MAX_PATH_LEN 512

typedef struct {
    char* function_name;
    char* file_path;
    int line_number;
    char* doc_comment;
} DocEntry;

static DocEntry* g_docs = NULL;
static int g_doc_count = 0;
static int g_doc_capacity = 0;

void doc_extractor_init() {
    g_doc_capacity = 128;
    g_docs = malloc(g_doc_capacity * sizeof(DocEntry));
    g_doc_count = 0;
}

void doc_extractor_cleanup() {
    for (int i = 0; i < g_doc_count; i++) {
        free(g_docs[i].function_name);
        free(g_docs[i].file_path);
        free(g_docs[i].doc_comment);
    }
    free(g_docs);
    g_docs = NULL;
    g_doc_count = 0;
}

void doc_extractor_add(const char* function_name, const char* file_path, int line, const char* comment) {
    if (g_doc_count >= g_doc_capacity) {
        int new_capacity = g_doc_capacity * 2;
        DocEntry* new_docs = realloc(g_docs, new_capacity * sizeof(DocEntry));
        if (!new_docs) {
            fprintf(stderr, "Failed to expand documentation storage\n");
            return;
        }
        g_docs = new_docs;
        g_doc_capacity = new_capacity;
    }

    DocEntry* entry = &g_docs[g_doc_count];
    entry->function_name = strdup(function_name);
    entry->file_path = strdup(file_path);
    entry->line_number = line;
    entry->doc_comment = comment ? strdup(comment) : NULL;
    g_doc_count++;
}

int is_rads_keyword(const char* word) {
    const char* keywords[] = {
        "blast", "turbo", "loop", "cruise", "if", "elif", "else",
        "return", "break", "continue", "import", "export", "async", "await",
        "spawn", "echo", "struct", "enum", "switch", "case", "default",
        "io", "math", "string", "array", "json", "fs", "net",
        "http", "db", "ffi", "media", "webengine", "test", "assert",
        "try", "catch", "throw", "null"
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

int extract_param_types(const char* signature, char** param_types, int max_params) {
    int count = 0;
    const char* start = signature;

    while (*start && count < max_params) {
        if (*start == '(' || *start == ',') {
            start++;
        }

        if (*start == ' ' || *start == '\t') {
            start++;
            continue;
        }

        if (*start == ':' || *start == '=') {
            if (count > 0 && *(start - 1) != '-' && *(start - 1) != '>') {
                start++;
                continue;
            }
        }

        if (*start == ' ') {
            param_types[count] = start + 1;
            count++;
        }

        start++;
    }

    return count;
}

char* extract_function_name(const char* line) {
    char name[128] = {0};
    int i = 0;

    while (line[i] && line[i] != '(' && line[i] != ':') {
        if (line[i] != ' ') {
            name[i++] = line[i];
        }
    }

    if (line[i] == '(') {
        name[i++] = line[i];
    }

    name[i] = '\0';
    return strdup(name);
}

char* extract_doc_comment(const char* line) {
    int in_comment = 0;
    char comment[MAX_DOC_LENGTH] = {0};
    int comment_len = 0;

    for (int i = 0; line[i] && i < MAX_DOC_LENGTH - 1; i++) {
        if (in_comment) {
            if (line[i] == '*' && line[i+1] == '/') {
                comment[comment_len++] = line[i];
                comment[comment_len++] = line[i+1];
                i++;
                comment[comment_len] = '\0';
                continue;
            }

            if (line[i] == '/' && line[i+1] == '*') {
                comment[comment_len++] = line[i];
                comment[comment_len++] = line[i+1];
                i++;
                comment[comment_len] = '\0';
                continue;
            }

            if (line[i] == '/' && line[i+1] == '/') {
                i++;
                comment[comment_len++] = line[i];
                comment[comment_len++] = line[i+1];
                continue;
            }
        }

        if (line[i] == '/' && i + 1 < MAX_DOC_LENGTH && line[i+1] == '*') {
            comment[comment_len++] = line[i];
            comment[comment_len++] = line[i+1];
            i++;
            comment[comment_len] = '\0';
            continue;
        }

        if (line[i] == '/' && i + 1 < MAX_DOC_LENGTH && (line[i+1] == '/' || line[i+1] == '*')) {
            in_comment = 1;
        }
    }

    comment[comment_len] = '\0';

    if (comment_len == 0) {
        return NULL;
    }

    char* result = malloc(comment_len + 1);
    strncpy(result, comment, comment_len);
    return result;
}

int doc_extractor_process_file(const char* filepath) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open: %s\n", filepath);
        return 0;
    }

    char line[1024];
    int line_num = 0;
    char* current_file = NULL;
    char* current_function = NULL;
    char current_signature[512] = {0};

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        char* end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && (*end == ' ' || *end == '\t')) end--;
        *(end + 1) = '\0';

        if (trimmed[0] == '/' && (trimmed[1] == '*' || trimmed[1] == '/')) {
            char* comment = extract_doc_comment(trimmed);
            if (comment) {
                doc_extractor_add(current_function, current_file, line_num, comment);
            }
        } else if (strstr(trimmed, "blast ") == trimmed ||
                     strstr(trimmed, "turbo ") == trimmed ||
                     strstr(trimmed, "struct ") == trimmed ||
                     strstr(trimmed, "enum ") == trimmed) {
            char* func_name = extract_function_name(trimmed);
            if (func_name) {
                strncpy(current_signature, trimmed, sizeof(current_signature) - 1);
                current_function = func_name;
            }
        } else {
            current_function = NULL;
        }
    }

    fclose(fp);
    return 1;
}

int doc_extractor_process_directory(const char* dirpath) {
    DIR* d = opendir(dirpath);
    if (!d) {
        fprintf(stderr, "Failed to open directory: %s\n", dirpath);
        return 0;
    }

    struct dirent* dirp;
    int processed = 0;

    while ((dirp = readdir(d)) != NULL) {
        if (strstr(dirp->d_name, ".c") || strstr(dirp->d_name, ".h")) {
            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, dirp->d_name);

            if (doc_extractor_process_file(fullpath)) {
                processed++;
            }
        }
    }

    closedir(d);
    return processed;
}

void doc_extractor_export_markdown(const char* output_path) {
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output: %s\n", output_path);
        return;
    }

    fprintf(fp, "# RADS API Documentation\n\n");
    fprintf(fp, "Generated for v0.0.7 DARK MOON\n\n");

    fprintf(fp, "## Functions\n\n");

    for (int i = 0; i < g_doc_count; i++) {
        if (g_docs[i].doc_comment) {
            fprintf(fp, "### %s (line %d)\n%s\n\n",
                    g_docs[i].function_name,
                    g_docs[i].line_number,
                    g_docs[i].doc_comment);
        } else {
            fprintf(fp, "### %s\n\n", g_docs[i].function_name);
        }
    }

    fclose(fp);
    printf("Exported %d documentation entries to %s\n", g_doc_count, output_path);
}

void doc_extractor_list() {
    printf("\n=== Documentation Entries (%d) ===\n", g_doc_count);

    for (int i = 0; i < g_doc_count; i++) {
        printf("%d. %s (line %d) in %s",
               i + 1, g_docs[i].function_name,
               g_docs[i].line_number, g_docs[i].file_path);

        if (g_docs[i].doc_comment) {
            printf("   Comment: %s\n", g_docs[i].doc_comment);
        }
    }

    printf("===================\n");
}

int doc_extractor_get_count() {
    return g_doc_count;
}

void doc_extractor_clear() {
    doc_extractor_cleanup();
    printf("Cleared %d documentation entries\n", g_doc_count);
}
