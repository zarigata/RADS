#include "stdlib_env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// env.get(name) - Get environment variable
Value stdlib_env_get(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: env.get() requires 1 argument (name)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: env.get() argument must be a string\n");
        return make_null();
    }
    
    const char* name = args[0].string_val;
    const char* value = getenv(name);
    
    if (value == NULL) {
        return make_null();
    }
    
    return make_string(value);
}

// env.set(name, value) - Set environment variable
Value stdlib_env_set(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 2) {
        fprintf(stderr, "Error: env.set() requires 2 arguments (name, value)\n");
        return make_bool(false);
    }
    
    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: env.set() arguments must be strings\n");
        return make_bool(false);
    }
    
    const char* name = args[0].string_val;
    const char* value = args[1].string_val;
    
    int result = setenv(name, value, 1);
    
    return make_bool(result == 0);
}

// env.has(name) - Check if environment variable exists
Value stdlib_env_has(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: env.has() requires 1 argument (name)\n");
        return make_bool(false);
    }
    
    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: env.has() argument must be a string\n");
        return make_bool(false);
    }
    
    const char* name = args[0].string_val;
    const char* value = getenv(name);
    
    return make_bool(value != NULL);
}

// env.unset(name) - Remove environment variable
Value stdlib_env_unset(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: env.unset() requires 1 argument (name)\n");
        return make_bool(false);
    }
    
    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: env.unset() argument must be a string\n");
        return make_bool(false);
    }
    
    const char* name = args[0].string_val;
    int result = unsetenv(name);
    
    return make_bool(result == 0);
}

// env.list() - Get all environment variables as struct
Value stdlib_env_list(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    extern char** environ;
    
    // Create a struct to hold all env vars
    StructInstance* instance = malloc(sizeof(StructInstance));
    if (!instance) return make_null();
    
    instance->fields = NULL;
    FieldValue* last_field = NULL;
    
    for (char** env = environ; *env != NULL; env++) {
        char* entry = strdup(*env);
        char* eq = strchr(entry, '=');
        
        if (eq != NULL) {
            *eq = '\0';  // Split at '='
            char* name = entry;
            char* value = eq + 1;
            
            // Create field
            FieldValue* field = malloc(sizeof(FieldValue));
            field->name = strdup(name);
            field->value = malloc(sizeof(Value));
            field->value->type = VAL_STRING;
            field->value->string_val = strdup(value);
            field->next = NULL;
            
            if (instance->fields == NULL) {
                instance->fields = field;
            } else {
                last_field->next = field;
            }
            last_field = field;
        }
        
        free(entry);
    }
    
    Value result;
    result.type = VAL_STRUCT_INSTANCE;
    result.struct_instance = instance;
    return result;
}

// env.paths() - Parse PATH into array
Value stdlib_env_paths(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    const char* path_env = getenv("PATH");
    
    if (path_env == NULL) {
        // Return empty array
        Array* arr = array_create(0);
        Value result;
        result.type = VAL_ARRAY;
        result.array_val = arr;
        return result;
    }
    
    // Split PATH by ':'
    char* path_copy = strdup(path_env);
    Array* arr = array_create(16);
    
    char* token = strtok(path_copy, ":");
    while (token != NULL) {
        Value item;
        item.type = VAL_STRING;
        item.string_val = strdup(token);
        array_push(arr, item);
        token = strtok(NULL, ":");
    }
    
    free(path_copy);
    
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// env.home() - Get HOME directory
Value stdlib_env_home(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    const char* home = getenv("HOME");
    if (home == NULL) {
        home = getenv("USERPROFILE");  // Windows fallback
    }
    
    if (home == NULL) {
        return make_null();
    }
    
    return make_string(home);
}

// env.cwd() - Get current working directory
Value stdlib_env_cwd(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return make_null();
    }
    
    return make_string(cwd);
}

// env.user() - Get current username
Value stdlib_env_user(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    const char* user = getenv("USER");
    if (user == NULL) {
        user = getenv("USERNAME");  // Windows fallback
    }
    
    if (user == NULL) {
        return make_null();
    }
    
    return make_string(user);
}

// env.shell() - Get current shell
Value stdlib_env_shell(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    const char* shell = getenv("SHELL");
    
    if (shell == NULL) {
        return make_null();
    }
    
    return make_string(shell);
}

void stdlib_env_register(void) {
    register_native("env.get", stdlib_env_get);
    register_native("env.set", stdlib_env_set);
    register_native("env.has", stdlib_env_has);
    register_native("env.unset", stdlib_env_unset);
    register_native("env.list", stdlib_env_list);
    register_native("env.paths", stdlib_env_paths);
    register_native("env.home", stdlib_env_home);
    register_native("env.cwd", stdlib_env_cwd);
    register_native("env.user", stdlib_env_user);
    register_native("env.shell", stdlib_env_shell);
}