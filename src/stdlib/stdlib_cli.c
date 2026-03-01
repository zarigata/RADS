#include "stdlib_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// Global storage for command-line arguments
static int g_argc = 0;
static char** g_argv = NULL;

// Initialize CLI module with argc/argv (called from main)
void cli_init(int argc, char** argv) {
    g_argc = argc;
    g_argv = argv;
}

// cli.args() - Get raw arguments array
Value stdlib_cli_args(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    Array* arr = array_create(g_argc > 0 ? g_argc : 1);
    
    // Skip argv[0] (program name) - start from index 1
    for (int i = 1; i < g_argc; i++) {
        Value item;
        item.type = VAL_STRING;
        item.string_val = strdup(g_argv[i]);
        array_push(arr, item);
    }
    
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// cli.program() - Get program name
Value stdlib_cli_program(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    if (g_argc > 0 && g_argv != NULL) {
        return make_string(g_argv[0]);
    }
    
    return make_string("rads");
}

// cli.count() - Get number of arguments
Value stdlib_cli_count(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    // Subtract 1 for program name
    return make_int(g_argc > 0 ? g_argc - 1 : 0);
}

// cli.has_flag(name) - Check if flag is present
Value stdlib_cli_has_flag(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: cli.has_flag() requires 1 argument (name)\n");
        return make_bool(false);
    }
    
    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: cli.has_flag() argument must be a string\n");
        return make_bool(false);
    }
    
    const char* flag_name = args[0].string_val;
    
    for (int i = 1; i < g_argc; i++) {
        if (strcmp(g_argv[i], flag_name) == 0) {
            return make_bool(true);
        }
    }
    
    return make_bool(false);
}

// cli.flag(name) - Get flag value (for --name value or --name=value)
Value stdlib_cli_flag(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: cli.flag() requires 1 argument (name)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: cli.flag() argument must be a string\n");
        return make_null();
    }
    
    const char* flag_name = args[0].string_val;
    size_t flag_len = strlen(flag_name);
    
    for (int i = 1; i < g_argc; i++) {
        const char* arg = g_argv[i];
        
        // Check for --name=value format
        if (strncmp(arg, flag_name, flag_len) == 0 && arg[flag_len] == '=') {
            return make_string(arg + flag_len + 1);
        }
        
        // Check for --name value format
        if (strcmp(arg, flag_name) == 0 && i + 1 < g_argc) {
            return make_string(g_argv[i + 1]);
        }
    }
    
    return make_null();
}

// cli.flags() - Get all flags (arguments starting with -)
Value stdlib_cli_flags(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    Array* arr = array_create(8);
    
    for (int i = 1; i < g_argc; i++) {
        const char* arg = g_argv[i];
        if (arg[0] == '-') {
            Value item;
            item.type = VAL_STRING;
            item.string_val = strdup(arg);
            array_push(arr, item);
        }
    }
    
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// cli.args_after_flag(name) - Get all arguments after a flag
Value stdlib_cli_args_after_flag(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: cli.args_after_flag() requires 1 argument (name)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: cli.args_after_flag() argument must be a string\n");
        return make_null();
    }
    
    const char* flag_name = args[0].string_val;
    
    // Find the flag
    for (int i = 1; i < g_argc; i++) {
        if (strcmp(g_argv[i], flag_name) == 0) {
            // Collect all args after this flag
            Array* arr = array_create(8);
            for (int j = i + 1; j < g_argc; j++) {
                // Stop at next flag
                if (g_argv[j][0] == '-') break;
                
                Value item;
                item.type = VAL_STRING;
                item.string_val = strdup(g_argv[j]);
                array_push(arr, item);
            }
            
            Value result;
            result.type = VAL_ARRAY;
            result.array_val = arr;
            return result;
        }
    }
    
    // Flag not found, return empty array
    Array* arr = array_create(0);
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// cli.parse(spec) - Parse arguments with specification
// spec is a struct where each field has: type, required, default, alias
Value stdlib_cli_parse(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1) {
        fprintf(stderr, "Error: cli.parse() requires 1 argument (spec)\n");
        return make_null();
    }
    
    if (args[0].type != VAL_STRUCT_INSTANCE) {
        fprintf(stderr, "Error: cli.parse() argument must be a struct\n");
        return make_null();
    }
    
    StructInstance* spec = args[0].struct_instance;
    
    // Create result struct
    StructInstance* result = malloc(sizeof(StructInstance));
    if (!result) return make_null();
    result->fields = NULL;
    FieldValue* last_result_field = NULL;
    
    // Iterate over spec fields
    FieldValue* spec_field = spec->fields;
    while (spec_field != NULL) {
        const char* name = spec_field->name;
        
        // Look for --name or -alias in args
        const char* value = NULL;
        const char* alias = NULL;
        const char* type = "string";
        bool required = false;
        const char* default_val = NULL;
        
        // Extract field options from spec
        if (spec_field->value->type == VAL_STRUCT_INSTANCE) {
            StructInstance* opts = spec_field->value->struct_instance;
            FieldValue* opt = opts->fields;
            while (opt != NULL) {
                if (strcmp(opt->name, "type") == 0 && opt->value->type == VAL_STRING) {
                    type = opt->value->string_val;
                } else if (strcmp(opt->name, "required") == 0 && opt->value->type == VAL_BOOL) {
                    required = opt->value->bool_val;
                } else if (strcmp(opt->name, "default") == 0 && opt->value->type == VAL_STRING) {
                    default_val = opt->value->string_val;
                } else if (strcmp(opt->name, "alias") == 0 && opt->value->type == VAL_STRING) {
                    alias = opt->value->string_val;
                }
                opt = opt->next;
            }
        }
        
        // Build flag names
        char long_flag[256];
        char short_flag[256];
        snprintf(long_flag, sizeof(long_flag), "--%s", name);
        if (alias) {
            snprintf(short_flag, sizeof(short_flag), "-%s", alias);
        } else {
            short_flag[0] = '\0';
        }
        
        // Search for flag in args
        for (int i = 1; i < g_argc; i++) {
            const char* arg = g_argv[i];
            
            bool found = false;
            if (strcmp(arg, long_flag) == 0) found = true;
            if (short_flag[0] && strcmp(arg, short_flag) == 0) found = true;
            
            if (found) {
                if (strcmp(type, "bool") == 0) {
                    value = "true";
                } else if (i + 1 < g_argc && g_argv[i + 1][0] != '-') {
                    value = g_argv[i + 1];
                }
                break;
            }
            
            // Check for --name=value format
            size_t long_len = strlen(long_flag);
            if (strncmp(arg, long_flag, long_len) == 0 && arg[long_len] == '=') {
                value = arg + long_len + 1;
                break;
            }
        }
        
        // Use default if not found
        if (value == NULL && default_val != NULL) {
            value = default_val;
        }
        
        // Check required
        if (required && value == NULL) {
            fprintf(stderr, "Error: Required argument '%s' not provided\n", name);
            free(result);
            return make_null();
        }
        
        // Add to result
        FieldValue* result_field = malloc(sizeof(FieldValue));
        result_field->name = strdup(name);
        result_field->value = malloc(sizeof(Value));
        result_field->next = NULL;
        
        if (value == NULL) {
            result_field->value->type = VAL_NULL;
        } else if (strcmp(type, "int") == 0) {
            result_field->value->type = VAL_INT;
            result_field->value->int_val = atoll(value);
        } else if (strcmp(type, "float") == 0) {
            result_field->value->type = VAL_FLOAT;
            result_field->value->float_val = atof(value);
        } else if (strcmp(type, "bool") == 0) {
            result_field->value->type = VAL_BOOL;
            result_field->value->bool_val = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else {
            result_field->value->type = VAL_STRING;
            result_field->value->string_val = strdup(value);
        }
        
        if (result->fields == NULL) {
            result->fields = result_field;
        } else {
            last_result_field->next = result_field;
        }
        last_result_field = result_field;
        
        spec_field = spec_field->next;
    }
    
    Value ret;
    ret.type = VAL_STRUCT_INSTANCE;
    ret.struct_instance = result;
    return ret;
}

void stdlib_cli_register(void) {
    register_native("cli.args", stdlib_cli_args);
    register_native("cli.program", stdlib_cli_program);
    register_native("cli.count", stdlib_cli_count);
    register_native("cli.has_flag", stdlib_cli_has_flag);
    register_native("cli.flag", stdlib_cli_flag);
    register_native("cli.flags", stdlib_cli_flags);
    register_native("cli.args_after_flag", stdlib_cli_args_after_flag);
    register_native("cli.parse", stdlib_cli_parse);
}