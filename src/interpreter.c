#define _POSIX_C_SOURCE 200809L
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Value functions
static Value make_null() {
    Value v;
    v.type = VAL_NULL;
    return v;
}

static Value make_bool(bool val) {
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = val;
    return v;
}

static Value make_int(long long val) {
    Value v;
    v.type = VAL_INT;
    v.int_val = val;
    return v;
}

static Value make_float(double val) {
    Value v;
    v.type = VAL_FLOAT;
    v.float_val = val;
    return v;
}

static Value make_string(const char* val) {
    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup(val);
    return v;
}

void value_print(Value* value) {
    switch (value->type) {
        case VAL_NULL:
            printf("null");
            break;
        case VAL_BOOL:
            printf("%s", value->bool_val ? "true" : "false");
            break;
        case VAL_INT:
            printf("%lld", value->int_val);
            break;
        case VAL_FLOAT:
            printf("%f", value->float_val);
            break;
        case VAL_STRING:
            printf("%s", value->string_val);
            break;
    }
}

void value_free(Value* value) {
    if (value->type == VAL_STRING) {
        free(value->string_val);
    }
}

// Forward declaration
static Value eval_expression(ASTNode* node);

// Evaluate binary operation
static Value eval_binary_op(ASTNode* node) {
    Value left = eval_expression(node->binary_op.left);
    Value right = eval_expression(node->binary_op.right);
    
    Value result = make_null();
    
    switch (node->binary_op.op) {
        case OP_ADD:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val + right.int_val);
            } else if (left.type == VAL_STRING && right.type == VAL_STRING) {
                size_t len = strlen(left.string_val) + strlen(right.string_val) + 1;
                char* str = malloc(len);
                strcpy(str, left.string_val);
                strcat(str, right.string_val);
                result = make_string(str);
                free(str);
            }
            break;
        case OP_SUB:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val - right.int_val);
            }
            break;
        case OP_MUL:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result = make_int(left.int_val * right.int_val);
            }
            break;
        case OP_DIV:
            if (left.type == VAL_INT && right.type == VAL_INT) {
                if (right.int_val != 0) {
                    result = make_int(left.int_val / right.int_val);
                }
            }
            break;
        default:
            break;
    }
    
    value_free(&left);
    value_free(&right);
    
    return result;
}

// Evaluate expression
static Value eval_expression(ASTNode* node) {
    if (!node) return make_null();
    
    switch (node->type) {
        case AST_INTEGER_LITERAL:
            return make_int(node->integer_literal.value);
        
        case AST_FLOAT_LITERAL:
            return make_float(node->float_literal.value);
        
        case AST_STRING_LITERAL:
            return make_string(node->string_literal.value);
        
        case AST_BOOL_LITERAL:
            return make_bool(node->bool_literal.value);
        
        case AST_BINARY_OP:
            return eval_binary_op(node);
        
        default:
            return make_null();
    }
}

// Execute echo statement
static void exec_echo(ASTNode* node) {
    Value value = eval_expression(node->echo_stmt.expression);
    value_print(&value);
    printf("\n");
    value_free(&value);
}

// Execute statement
static void exec_statement(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_ECHO_STMT:
            exec_echo(node);
            break;
        
        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statements->count; i++) {
                exec_statement(node->block.statements->nodes[i]);
            }
            break;
        
        default:
            // Ignore other statements for now
            break;
    }
}

// Execute function
static void exec_function(ASTNode* node) {
    if (strcmp(node->function_decl.name, "main") == 0) {
        exec_statement(node->function_decl.body);
    }
}

// Main interpreter
int interpret(ASTNode* program) {
    if (!program || program->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid program\n");
        return 1;
    }
    
    // Find and execute main function
    for (size_t i = 0; i < program->program.declarations->count; i++) {
        ASTNode* decl = program->program.declarations->nodes[i];
        if (decl->type == AST_FUNCTION_DECL) {
            exec_function(decl);
        }
    }
    
    return 0;
}
