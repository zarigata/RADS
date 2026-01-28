#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type {
    char* name;
    char* base_type;
    char** type_params;
    int param_count;
    int is_polymorphic;
} Type;

typedef struct TypeContext {
    Type** types;
    int type_count;
    Type** instances;
    int instance_count;
} TypeContext;

static TypeContext g_type_context = {
    .types = NULL,
    .type_count = 0,
    .instances = NULL,
    .instance_count = 0
};

void typecheck_generics_init() {
    g_type_context.types = malloc(256 * sizeof(Type*));
    g_type_context.instances = malloc(1024 * sizeof(Type*));
    g_type_context.type_count = 0;
    g_type_context.instance_count = 0;

    for (int i = 0; i < 256; i++) {
        g_type_context.types[i] = NULL;
    }
    for (int i = 0; i < 1024; i++) {
        g_type_context.instances[i] = NULL;
    }
}

Type* typecheck_create_generic(const char* name, const char* base_type, char** params, int param_count) {
    if (g_type_context.type_count >= 256) {
        fprintf(stderr, "Type registry full\n");
        return NULL;
    }

    Type* type = malloc(sizeof(Type));
    if (!type) return NULL;

    type->name = strdup(name);
    type->base_type = strdup(base_type);
    type->is_polymorphic = param_count > 0;

    type->param_count = param_count;
    type->type_params = malloc(param_count * sizeof(char*));

    for (int i = 0; i < param_count; i++) {
        type->type_params[i] = strdup(params[i]);
    }

    g_type_context.types[g_type_context.type_count] = type;
    g_type_context.type_count++;

    printf("Created generic type: %s with %d type parameters\n", name, param_count);

    return type;
}

Type* typecheck_instantiate_generic(Type* generic_type, char** concrete_params, int param_count) {
    if (!generic_type || !generic_type->is_polymorphic) {
        fprintf(stderr, "Cannot instantiate non-polymorphic type\n");
        return NULL;
    }

    if (param_count != generic_type->param_count) {
        fprintf(stderr, "Type parameter count mismatch\n");
        return NULL;
    }

    Type* instance = malloc(sizeof(Type));
    if (!instance) return NULL;

    char instance_name[256];
    snprintf(instance_name, sizeof(instance_name), "%s<", generic_type->name);

    for (int i = 0; i < param_count; i++) {
        if (i > 0) strncat(instance_name, ", ", 1);
        strncat(instance_name, concrete_params[i], 255);
    }

    strncat(instance_name, ">", 255);

    instance->name = strdup(instance_name);
    instance->base_type = strdup(generic_type->base_type);
    instance->is_polymorphic = 0;
    instance->param_count = 0;
    instance->type_params = NULL;

    g_type_context.instances[g_type_context.instance_count] = instance;
    g_type_context.instance_count++;

    printf("Instantiated generic type: %s\n", instance_name);

    return instance;
}

int typecheck_check_type_compatibility(Type* type1, Type* type2) {
    if (!type1 || !type2) return 0;

    if (strcmp(type1->base_type, type2->base_type) != 0) {
        return 0;
    }

    if (type1->is_polymorphic && type2->is_polymorphic) {
        if (strcmp(type1->name, type2->name) == 0) {
            return 1;
        }
    }

    return 0;
}

Type* typecheck_find_type(const char* name) {
    for (int i = 0; i < g_type_context.instance_count; i++) {
        Type* type = g_type_context.instances[i];
        if (type && strcmp(type->name, name) == 0) {
            return type;
        }
    }
    return NULL;
}

void typecheck_generic_register_function(const char* name) {
    Type* string_list_type = typecheck_create_generic("List", "string", NULL, 0);
    Type* array_type = typecheck_create_generic("Array", "unknown", (char*[]){"T"}, 1);

    printf("Registered generic function types for: %s\n", name);
}

int typecheck_generics_test() {
    printf("\n=== Generic Types Test ===\n");

    Type* list_int = typecheck_create_generic("List", "int", NULL, 0);
    Type* list_str = typecheck_create_generic("List", "string", NULL, 0);

    Type* list_int_inst = typecheck_instantiate_generic(list_int, (char*[]){"int"}, 1);
    Type* list_str_inst = typecheck_instantiate_generic(list_str, (char*[]){"string"}, 1);

    if (!list_int_inst || !list_str_inst) {
        printf("✗ Generic instantiation test FAILED\n");
        return 0;
    }

    int compatible = typecheck_check_type_compatibility(list_int_inst, list_str_inst);
    printf("List<int> compatible with List<string>: %d\n", compatible);

    if (!compatible) {
        printf("✓ Generic types test PASSED\n");
        return 1;
    } else {
        printf("✗ Generic types test FAILED\n");
        return 0;
    }
}

void typecheck_generics_cleanup() {
    for (int i = 0; i < g_type_context.type_count; i++) {
        if (g_type_context.types[i]) {
            Type* type = g_type_context.types[i];
            if (type->name) free(type->name);
            if (type->base_type) free(type->base_type);
            for (int j = 0; j < type->param_count; j++) {
                if (type->type_params && type->type_params[j]) {
                    free(type->type_params[j]);
                }
            }
            free(type->type_params);
            free(type);
        }
    }

    for (int i = 0; i < g_type_context.instance_count; i++) {
        if (g_type_context.instances[i]) {
            Type* type = g_type_context.instances[i];
            if (type->name) free(type->name);
            if (type->base_type) free(type->base_type);
            free(type);
        }
    }

    free(g_type_context.types);
    free(g_type_context.instances);

    printf("Generic types cleaned up: %d type definitions, %d instantiations freed\n",
           g_type_context.type_count, g_type_context.instance_count);
}

void typecheck_generics_info() {
    printf("\n=== Generic Types System Info ===\n");
    printf("Registered generic types: %d\n", g_type_context.type_count);
    printf("Active instantiations: %d\n", g_type_context.instance_count);
    printf("=====================================\n");
}
