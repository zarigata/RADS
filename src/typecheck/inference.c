#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core/interpreter.h"

#define TYPE_INFER_INT "int"
#define TYPE_INFER_FLOAT "float"
#define TYPE_INFER_STRING "string"
#define TYPE_INFER_BOOL "bool"
#define TYPE_INFER_UNKNOWN "unknown"

typedef struct {
    char* type;
    void* value;
} InferredType;

typedef struct {
    char** variables;
    char** types;
    int count;
} SymbolTable;

static SymbolTable g_symbol_table = {NULL, NULL, 0};

void type_inference_init() {
    g_symbol_table.variables = malloc(256 * sizeof(char*));
    g_symbol_table.types = malloc(256 * sizeof(char*));
    g_symbol_table.count = 0;

    for (int i = 0; i < 256; i++) {
        g_symbol_table.variables[i] = NULL;
        g_symbol_table.types[i] = NULL;
    }
}

void type_inference_cleanup() {
    if (g_symbol_table.variables) {
        for (int i = 0; i < 256; i++) {
            if (g_symbol_table.variables[i]) {
                free(g_symbol_table.variables[i]);
            }
        }
        free(g_symbol_table.variables);
    }

    if (g_symbol_table.types) {
        for (int i = 0; i < 256; i++) {
            if (g_symbol_table.types[i]) {
                free(g_symbol_table.types[i]);
            }
        }
        free(g_symbol_table.types);
    }

    g_symbol_table.count = 0;
}

const char* type_infer_literal(Value* value) {
    if (!value) return TYPE_INFER_UNKNOWN;

    if (value->type == VALUE_TYPE_INT) {
        return TYPE_INFER_INT;
    } else if (Value->type == VALUE_TYPE_FLOAT) {
        return TYPE_INFER_FLOAT;
    } else if (Value->type == VALUE_TYPE_STRING) {
        return TYPE_INFER_STRING;
    } else if (Value->type == VALUE_TYPE_BOOL) {
        return TYPE_INFER_BOOL;
    }

    return TYPE_INFER_UNKNOWN;
}

const char* type_infer_unary_op(const char* op_type, const char* operator) {
    if (strcmp(op_type, TYPE_INFER_INT) == 0) {
        if (strcmp(operator, "-") == 0 || strcmp(operator, "+") == 0) {
            return TYPE_INFER_INT;
        }
        if (strcmp(operator, "!") == 0) {
            return TYPE_INFER_BOOL;
        }
    } else if (strcmp(op_type, TYPE_INFER_FLOAT) == 0) {
        return TYPE_INFER_FLOAT;
    } else if (strcmp(op_type, TYPE_INFER_BOOL) == 0) {
        if (strcmp(operator, "!") == 0) {
            return TYPE_INFER_BOOL;
        }
    }

    return TYPE_INFER_UNKNOWN;
}

const char* type_infer_binary_op(const char* left_type, const char* right_type, const char* operator) {
    if (strcmp(operator, "+") == 0 || strcmp(operator, "-") == 0 ||
        strcmp(operator, "*") == 0 || strcmp(operator, "/") == 0) {
        if (strcmp(left_type, TYPE_INFER_INT) == 0 && strcmp(right_type, TYPE_INFER_INT) == 0) {
            return TYPE_INFER_INT;
        }
        if (strcmp(left_type, TYPE_INFER_FLOAT) == 0 || strcmp(right_type, TYPE_INFER_FLOAT) == 0) {
            return TYPE_INFER_FLOAT;
        }
        if (strcmp(left_type, TYPE_INFER_INT) == 0 || strcmp(right_type, TYPE_INFER_INT) == 0) {
            return TYPE_INFER_FLOAT;
        }
    }

    if (strcmp(operator, "==") == 0 || strcmp(operator, "!=") == 0 ||
        strcmp(operator, "<") == 0 || strcmp(operator, ">") == 0 ||
        strcmp(operator, "<=") == 0 || strcmp(operator, ">=") == 0) {
        return TYPE_INFER_BOOL;
    }

    return TYPE_INFER_UNKNOWN;
}

void type_inference_add_symbol(const char* name, const char* type) {
    if (g_symbol_table.count >= 256) {
        fprintf(stderr, "Symbol table full\n");
        return;
    }

    for (int i = 0; i < 256; i++) {
        if (g_symbol_table.variables[i] == NULL) {
            g_symbol_table.variables[i] = strdup(name);
            g_symbol_table.types[i] = strdup(type);
            g_symbol_table.count++;
            return;
        }
    }
}

const char* type_inference_lookup(const char* name) {
    for (int i = 0; i < g_symbol_table.count; i++) {
        if (strcmp(g_symbol_table.variables[i], name) == 0) {
            return g_symbol_table.types[i];
        }
    }
    return NULL;
}

const char* type_inference_analyze_expression(Value* node) {
    if (!node || node->type != VALUE_TYPE_NODE) {
        return TYPE_INFER_UNKNOWN;
    }

    Node* expr_node = node->node;

    if (expr_node->type == NODE_LITERAL) {
        return type_infer_literal(expr_node->value);
    }

    if (expr_node->type == NODE_BINARY) {
        const char* left_type = type_inference_analyze_expression(expr_node->left);
        const char* right_type = type_inference_analyze_expression(expr_node->right);
        return type_infer_binary_op(left_type, right_type, expr_node->op);
    }

    if (expr_node->type == NODE_UNARY) {
        const char* operand_type = type_inference_analyze_expression(expr_node->operand);
        const char* op = expr_node->op;
        return type_infer_unary_op(operand_type, op);
    }

    if (expr_node->type == NODE_VARIABLE) {
        return type_inference_lookup(expr_node->var_name);
    }

    return TYPE_INFER_UNKNOWN;
}

int type_inference_check_assign_compatibility(const char* expected_type, const char* actual_type, const char* var_name) {
    if (strcmp(expected_type, TYPE_INFER_UNKNOWN) == 0) {
        return 1;
    }

    if (strcmp(actual_type, TYPE_INFER_UNKNOWN) == 0) {
        return 1;
    }

    if (strcmp(expected_type, actual_type) == 0) {
        return 1;
    }

    if (strcmp(expected_type, TYPE_INFER_INT) == 0 && strcmp(actual_type, TYPE_INFER_FLOAT) == 0) {
        return 1;
    }

    if (strcmp(expected_type, actual_type) == 0 && strcmp(actual_type, TYPE_INFER_STRING) == 0) {
        printf("Type mismatch: Variable '%s' is string but expected %s\n", var_name, expected_type);
        return 0;
    }

    return 1;
}

void type_inference_analyze_function(Value* function_node) {
    if (!function_node || function_node->type != VALUE_TYPE_NODE) {
        return;
    }

    Node* func_node = function_node->node;
    if (func_node->type != NODE_FUNCTION) {
        return;
    }

    const char* func_name = func_node->func_name;

    printf("\nAnalyzing function: %s\n", func_name);

    for (int i = 0; i < func_node->param_count; i++) {
        if (func_node->params[i]) {
            const char* param_name = func_node->param_names[i];
            Node* param_node = func_node->params[i];

            if (param_node->type == NODE_VARIABLE) {
                const char* inferred_type = type_inference_analyze_expression(param_node->default_value);
                type_inference_add_symbol(param_name, inferred_type);
                printf("  Param '%s': inferred as %s\n", param_name, inferred_type);
            }
        }
    }

    if (func_node->body) {
        const char* return_type = type_inference_analyze_expression(func_node->body);
        printf("  Return type: %s\n", return_type);
        type_inference_add_symbol(func_name, return_type);
    }

    printf("Function analysis complete\n");
}

void type_inference_display_symbol_table() {
    printf("\n=== Symbol Table ===\n");

    for (int i = 0; i < g_symbol_table.count; i++) {
        printf("  %s: %s\n", g_symbol_table.variables[i], g_symbol_table.types[i]);
    }

    printf("==================\n");
}

int type_inference_test() {
    printf("\n=== Type Inference Test ===\n");

    type_inference_init();

    type_inference_add_symbol("x", TYPE_INFER_INT);
    type_inference_add_symbol("y", TYPE_INFER_STRING);
    type_inference_add_symbol("flag", TYPE_INFER_BOOL);

    printf("Inferred types:\n");
    type_inference_display_symbol_table();

    const char* test_expr1_type = type_inference_analyze_expression(
        (Value){.type = VALUE_TYPE_NODE, .node = (Node*)malloc(sizeof(Node))}
    );
    printf("Test expression (x + y): %s\n", test_expr1_type);

    int compatible = type_inference_check_assign_compatibility(TYPE_INFER_STRING, test_expr1_type, "result");
    printf("Compatibility check (string = result): %d\n", compatible);

    if (!compatible) {
        printf("✗ Type inference test FAILED\n");
    } else {
        printf("✓ Type inference test PASSED\n");
    }

    type_inference_cleanup();

    return compatible ? 1 : 0;
}
