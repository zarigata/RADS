#define _POSIX_C_SOURCE 200809L
#include "stdlib_webengine.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

// ============================================================================
// WEB ENGINE - JavaScript Runtime, HTML/CSS Parser, Plugin System
// RADS v0.0.3 "Butterfly" - Alternative to Node.js and Python
// ============================================================================

// JavaScript Engine State (using embedded interpreter approach)
typedef struct {
    bool initialized;
    int global_count;
    int global_capacity;
    char** global_vars;
    char** global_values;
} JSEngine;

static JSEngine* js_engine = NULL;

// HTML Document Structure
typedef struct HTMLNode {
    char* tag;
    char* text;
    char** attr_names;
    char** attr_values;
    int attr_count;
    struct HTMLNode** children;
    int child_count;
    struct HTMLNode* parent;
} HTMLNode;

typedef struct {
    HTMLNode* root;
    char* original_html;
} HTMLDocument;

// Plugin Structure
typedef struct Plugin {
    char* name;
    char* version;
    char* path;
    char* main_file;
    void* handle;  // For dynamic libraries
    struct Plugin* next;
} Plugin;

static Plugin* plugin_head = NULL;

// ============================================================================
// JAVASCRIPT ENGINE FUNCTIONS
// ============================================================================

// Initialize JavaScript engine
Value native_web_engine_init(struct Interpreter* __attribute__((unused)) interp, int __attribute__((unused)) argc, Value* __attribute__((unused)) args) {
    if (!js_engine) {
        js_engine = malloc(sizeof(JSEngine));
        js_engine->initialized = true;
        js_engine->global_count = 0;
        js_engine->global_capacity = 100;
        js_engine->global_vars = malloc(sizeof(char*) * js_engine->global_capacity);
        js_engine->global_values = malloc(sizeof(char*) * js_engine->global_capacity);

        printf("\033[1;36m[WEB]\033[0m JavaScript engine initialized\n");
    }

    Value result = {0};
    result.type = VAL_BOOL;
    result.bool_val = true;
    return result;
}

// Execute JavaScript code (simplified - evaluates simple expressions)
Value native_web_js_eval(struct Interpreter* __attribute__((unused)) interp, int argc, Value* args) {
    if (argc != 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: web.js.eval() requires 1 string argument\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    if (!js_engine || !js_engine->initialized) {
        fprintf(stderr, "Error: Web engine not initialized. Call web.engine.init() first\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* code = args[0].string_val;

    printf("\033[1;33m[JS]\033[0m Executing: %s\n", code);

    // Simple evaluation (in full implementation, this would use QuickJS or similar)
    // For now, we'll provide basic console.log support
    if (strstr(code, "console.log") != NULL) {
        // Extract the message from console.log('message')
        char* start = strchr(code, '(');
        char* end = strrchr(code, ')');
        if (start && end && end > start) {
            start++;  // Skip '('
            char msg[1024];
            int len = end - start;
            if (len > 1023) len = 1023;
            strncpy(msg, start, len);
            msg[len] = '\0';

            // Remove quotes if present
            char* clean_msg = msg;
            if (msg[0] == '\'' || msg[0] == '"') clean_msg++;
            int msg_len = strlen(clean_msg);
            if (msg_len > 0 && (clean_msg[msg_len-1] == '\'' || clean_msg[msg_len-1] == '"')) {
                clean_msg[msg_len-1] = '\0';
            }

            printf("\033[1;32m[CONSOLE]\033[0m %s\n", clean_msg);
        }
    }

    Value result = {0};
    result.type = VAL_BOOL;
    result.bool_val = true;
    return result;
}

// Execute JavaScript file
Value native_web_js_evalFile(struct Interpreter* interp, int argc, Value* args) {
    if (argc != 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: web.js.evalFile() requires 1 string argument\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* filepath = args[0].string_val;

    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file: %s\n", filepath);
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* code = malloc(file_size + 1);
    fread(code, 1, file_size, file);
    code[file_size] = '\0';
    fclose(file);

    printf("\033[1;36m[WEB]\033[0m Executing JavaScript file: %s\n", filepath);

    // Create args for eval
    Value eval_args[1];
    eval_args[0].type = VAL_STRING;
    eval_args[0].string_val = code;

    Value result = native_web_js_eval(interp, 1, eval_args);

    free(code);
    return result;
}

// ============================================================================
// HTML PARSER FUNCTIONS
// ============================================================================

// Simple HTML parser (basic implementation)
static HTMLNode* html_create_node(const char* tag) {
    HTMLNode* node = malloc(sizeof(HTMLNode));
    node->tag = strdup(tag);
    node->text = NULL;
    node->attr_names = NULL;
    node->attr_values = NULL;
    node->attr_count = 0;
    node->children = NULL;
    node->child_count = 0;
    node->parent = NULL;
    return node;
}

// Parse HTML string (simplified parser)
Value native_web_html_parse(struct Interpreter* __attribute__((unused)) interp, int argc, Value* args) {
    if (argc != 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: web.html.parse() requires 1 string argument\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* html = args[0].string_val;

    HTMLDocument* doc = malloc(sizeof(HTMLDocument));
    doc->original_html = strdup(html);
    doc->root = html_create_node("html");

    printf("\033[1;36m[HTML]\033[0m Parsed HTML document (%zu bytes)\n", strlen(html));

    // Return document as a string pointer (in full implementation, this would be a proper object)
    Value result = {0};
    result.type = VAL_STRING;
    result.string_val = malloc(64);
    snprintf(result.string_val, 64, "<HTMLDocument:%p>", (void*)doc);
    return result;
}

// Query selector (basic implementation)
Value native_web_html_querySelector(struct Interpreter* __attribute__((unused)) interp, int argc, Value* args) {
    if (argc != 2) {
        fprintf(stderr, "Error: web.html.querySelector() requires 2 arguments\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    printf("\033[1;36m[HTML]\033[0m querySelector: %s\n", args[1].string_val);

    Value result = {0};
    result.type = VAL_STRING;
    result.string_val = strdup("<Element>");
    return result;
}

// ============================================================================
// PLUGIN SYSTEM FUNCTIONS
// ============================================================================

// Load plugin
Value native_web_plugin_load(struct Interpreter* __attribute__((unused)) interp, int argc, Value* args) {
    if (argc != 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: web.plugin.load() requires 1 string argument\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* plugin_name = args[0].string_val;

    // Check if already loaded
    Plugin* current = plugin_head;
    while (current) {
        if (strcmp(current->name, plugin_name) == 0) {
            printf("\033[1;33m[PLUGIN]\033[0m Already loaded: %s\n", plugin_name);
            Value result = {0};
            result.type = VAL_STRING;
            result.string_val = malloc(128);
            snprintf(result.string_val, 128, "<Plugin:%s>", plugin_name);
            return result;
        }
        current = current->next;
    }

    // Create new plugin entry
    Plugin* plugin = malloc(sizeof(Plugin));
    plugin->name = strdup(plugin_name);
    plugin->version = strdup("1.0.0");
    plugin->path = NULL;
    plugin->main_file = NULL;
    plugin->handle = NULL;
    plugin->next = plugin_head;
    plugin_head = plugin;

    printf("\033[1;32m[PLUGIN]\033[0m Loaded: %s v%s\n", plugin->name, plugin->version);

    Value result = {0};
    result.type = VAL_STRING;
    result.string_val = malloc(128);
    snprintf(result.string_val, 128, "<Plugin:%s>", plugin_name);
    return result;
}

// List installed plugins
Value native_web_plugin_list(struct Interpreter* __attribute__((unused)) interp, int __attribute__((unused)) argc, Value* __attribute__((unused)) args) {
    printf("\033[1;36m[PLUGIN]\033[0m Installed plugins:\n");

    Plugin* current = plugin_head;
    int count = 0;

    while (current) {
        printf("  • %s v%s\n", current->name, current->version);
        current = current->next;
        count++;
    }

    if (count == 0) {
        printf("  (No plugins installed)\n");
    }

    Value result = {0};
    result.type = VAL_INT;
    result.int_val = count;
    return result;
}

// Install plugin from path
Value native_web_plugin_install(struct Interpreter* __attribute__((unused)) interp, int argc, Value* args) {
    if (argc != 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: web.plugin.install() requires 1 string argument\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* plugin_path = args[0].string_val;

    printf("\033[1;36m[PLUGIN]\033[0m Installing from: %s\n", plugin_path);

    // In full implementation, this would:
    // 1. Read package.json
    // 2. Copy files to plugin directory
    // 3. Install dependencies
    // 4. Register plugin

    Value result = {0};
    result.type = VAL_BOOL;
    result.bool_val = true;
    return result;
}

// ============================================================================
// CSS PARSER FUNCTIONS
// ============================================================================

// Parse CSS
Value native_web_css_parse(struct Interpreter* __attribute__((unused)) interp, int argc, Value* args) {
    if (argc != 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: web.css.parse() requires 1 string argument\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* css = args[0].string_val;
    printf("\033[1;36m[CSS]\033[0m Parsed stylesheet (%zu bytes)\n", strlen(css));

    Value result = {0};
    result.type = VAL_STRING;
    result.string_val = strdup("<CSSStyleSheet>");
    return result;
}

// ============================================================================
// CLEANUP FUNCTIONS
// ============================================================================

void stdlib_webengine_cleanup(void) {
    if (js_engine) {
        for (int i = 0; i < js_engine->global_count; i++) {
            free(js_engine->global_vars[i]);
            free(js_engine->global_values[i]);
        }
        free(js_engine->global_vars);
        free(js_engine->global_values);
        free(js_engine);
        js_engine = NULL;
    }

    Plugin* current = plugin_head;
    while (current) {
        Plugin* next = current->next;
        free(current->name);
        free(current->version);
        if (current->path) free(current->path);
        if (current->main_file) free(current->main_file);
        if (current->handle) dlclose(current->handle);
        free(current);
        current = next;
    }
    plugin_head = NULL;
}

// ============================================================================
// REGISTRATION
// ============================================================================

void stdlib_webengine_register(void) {
    // Engine initialization
    register_native("web_init", native_web_engine_init);

    // JavaScript - Use underscore naming to avoid conflicts
    register_native("js_eval", native_web_js_eval);
    register_native("js_evalFile", native_web_js_evalFile);

    // HTML - Use underscore naming
    register_native("html_parse", native_web_html_parse);
    register_native("html_querySelector", native_web_html_querySelector);

    // CSS
    register_native("css_parse", native_web_css_parse);

    // Plugins - Renamed to avoid 'plugin' keyword conflict
    register_native("pkg_load", native_web_plugin_load);
    register_native("pkg_list", native_web_plugin_list);
    register_native("pkg_install", native_web_plugin_install);

    printf("\033[1;36m[WEB]\033[0m stdlib_webengine_register\n");
}
