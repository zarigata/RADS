/**
 * RADS Masquerading - AST Implementation
 *
 * Implementation of language-agnostic AST construction and management.
 *
 * Version: 0.0.5 "CHAMELEON"
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Helper function to allocate and initialize a node
// ============================================================================

static MaskASTNode* mask_ast_alloc(MaskNodeType type) {
    MaskASTNode* node = (MaskASTNode*)calloc(1, sizeof(MaskASTNode));
    if (!node) {
        fprintf(stderr, "Error: Failed to allocate AST node\n");
        exit(1);
    }
    node->type = type;
    return node;
}

// ============================================================================
// Literal Constructors
// ============================================================================

MaskASTNode* mask_ast_create_int(long long value) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_INT_LITERAL);
    node->data.int_literal.int_value = value;
    return node;
}

MaskASTNode* mask_ast_create_float(double value) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_FLOAT_LITERAL);
    node->data.float_literal.float_value = value;
    return node;
}

MaskASTNode* mask_ast_create_string(const char* value) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_STRING_LITERAL);
    node->data.string_literal.string_value = strdup(value);
    return node;
}

MaskASTNode* mask_ast_create_bool(int value) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_BOOL_LITERAL);
    node->data.bool_literal.bool_value = value;
    return node;
}

MaskASTNode* mask_ast_create_null(void) {
    return mask_ast_alloc(MASK_NODE_NULL_LITERAL);
}

MaskASTNode* mask_ast_create_identifier(const char* name) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_IDENTIFIER);
    node->data.identifier.name = strdup(name);
    return node;
}

// ============================================================================
// Operator Constructors
// ============================================================================

MaskASTNode* mask_ast_create_binary_op(MaskOperator op, MaskASTNode* left, MaskASTNode* right) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_BINARY_OP);
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

MaskASTNode* mask_ast_create_unary_op(MaskOperator op, MaskASTNode* operand) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_UNARY_OP);
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

MaskASTNode* mask_ast_create_assign(MaskASTNode* target, MaskASTNode* value) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_ASSIGN);
    node->data.assign.target = target;
    node->data.assign.value = value;
    return node;
}

// ============================================================================
// Control Flow Constructors
// ============================================================================

MaskASTNode* mask_ast_create_if(MaskASTNode* condition, MaskASTNode* then_block, MaskASTNode* else_block) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_IF_STMT);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_block = then_block;
    node->data.if_stmt.else_block = else_block;
    return node;
}

MaskASTNode* mask_ast_create_while(MaskASTNode* condition, MaskASTNode* body) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_WHILE_LOOP);
    node->data.while_loop.condition = condition;
    node->data.while_loop.body = body;
    return node;
}

MaskASTNode* mask_ast_create_for(MaskASTNode* init, MaskASTNode* condition, MaskASTNode* increment, MaskASTNode* body) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_FOR_LOOP);
    node->data.for_loop.init = init;
    node->data.for_loop.condition = condition;
    node->data.for_loop.increment = increment;
    node->data.for_loop.body = body;
    return node;
}

MaskASTNode* mask_ast_create_return(MaskASTNode* value) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_RETURN);
    node->data.return_stmt.value = value;
    return node;
}

MaskASTNode* mask_ast_create_break(void) {
    return mask_ast_alloc(MASK_NODE_BREAK);
}

MaskASTNode* mask_ast_create_continue(void) {
    return mask_ast_alloc(MASK_NODE_CONTINUE);
}

// ============================================================================
// Function Constructors
// ============================================================================

MaskASTNode* mask_ast_create_function_def(const char* name, MaskASTNode* params, MaskASTNode* body) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_FUNCTION_DEF);
    node->data.function_def.name = strdup(name);
    node->data.function_def.params = params;
    node->data.function_def.body = body;
    return node;
}

MaskASTNode* mask_ast_create_function_call(MaskASTNode* function, MaskASTNode* args) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_FUNCTION_CALL);
    node->data.function_call.function = function;
    node->data.function_call.args = args;
    return node;
}

// ============================================================================
// Data Structure Constructors
// ============================================================================

MaskASTNode* mask_ast_create_array(MaskASTList* elements) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_ARRAY_LITERAL);
    node->data.array_literal.elements = elements;
    return node;
}

MaskASTNode* mask_ast_create_dict(MaskASTList* keys, MaskASTList* values) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_DICT_LITERAL);
    node->data.dict_literal.keys = keys;
    node->data.dict_literal.values = values;
    return node;
}

// ============================================================================
// Access Constructors
// ============================================================================

MaskASTNode* mask_ast_create_member_access(MaskASTNode* object, const char* member) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_MEMBER_ACCESS);
    node->data.member_access.object = object;
    node->data.member_access.member = strdup(member);
    return node;
}

MaskASTNode* mask_ast_create_index_access(MaskASTNode* object, MaskASTNode* index) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_INDEX_ACCESS);
    node->data.index_access.object = object;
    node->data.index_access.index = index;
    return node;
}

// ============================================================================
// Variable and Statement Constructors
// ============================================================================

MaskASTNode* mask_ast_create_var_decl(const char* name, MaskASTNode* init_value, const char* type_hint) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_VAR_DECL);
    node->data.var_decl.name = strdup(name);
    node->data.var_decl.init_value = init_value;
    node->data.var_decl.type_hint = type_hint ? strdup(type_hint) : NULL;
    return node;
}

MaskASTNode* mask_ast_create_expr_stmt(MaskASTNode* expr) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_EXPR_STMT);
    node->data.expr_stmt.expr = expr;
    return node;
}

MaskASTNode* mask_ast_create_import(const char* module_name, MaskASTList* imported_names) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_IMPORT);
    node->data.import.module_name = strdup(module_name);
    node->data.import.imported_names = imported_names;
    return node;
}

MaskASTNode* mask_ast_create_block(MaskASTList* statements) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_BLOCK);
    node->data.block.statements = statements;
    return node;
}

MaskASTNode* mask_ast_create_program(MaskASTList* statements) {
    MaskASTNode* node = mask_ast_alloc(MASK_NODE_PROGRAM);
    node->data.program.statements = statements;
    return node;
}

// ============================================================================
// List Functions
// ============================================================================

MaskASTList* mask_ast_list_create(MaskASTNode* node) {
    MaskASTList* list = (MaskASTList*)malloc(sizeof(MaskASTList));
    if (!list) {
        fprintf(stderr, "Error: Failed to allocate AST list\n");
        exit(1);
    }
    list->node = node;
    list->next = NULL;
    return list;
}

MaskASTList* mask_ast_list_append(MaskASTList* list, MaskASTNode* node) {
    if (!list) {
        return mask_ast_list_create(node);
    }

    MaskASTList* current = list;
    while (current->next) {
        current = current->next;
    }

    current->next = mask_ast_list_create(node);
    return list;
}

int mask_ast_list_length(MaskASTList* list) {
    int count = 0;
    MaskASTList* current = list;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// ============================================================================
// Cleanup Functions
// ============================================================================

void mask_ast_free(MaskASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case MASK_NODE_STRING_LITERAL:
            free(node->data.string_literal.string_value);
            break;

        case MASK_NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;

        case MASK_NODE_BINARY_OP:
            mask_ast_free(node->data.binary_op.left);
            mask_ast_free(node->data.binary_op.right);
            break;

        case MASK_NODE_UNARY_OP:
            mask_ast_free(node->data.unary_op.operand);
            break;

        case MASK_NODE_ASSIGN:
            mask_ast_free(node->data.assign.target);
            mask_ast_free(node->data.assign.value);
            break;

        case MASK_NODE_IF_STMT:
            mask_ast_free(node->data.if_stmt.condition);
            mask_ast_free(node->data.if_stmt.then_block);
            mask_ast_free(node->data.if_stmt.else_block);
            break;

        case MASK_NODE_WHILE_LOOP:
            mask_ast_free(node->data.while_loop.condition);
            mask_ast_free(node->data.while_loop.body);
            break;

        case MASK_NODE_FOR_LOOP:
            mask_ast_free(node->data.for_loop.init);
            mask_ast_free(node->data.for_loop.condition);
            mask_ast_free(node->data.for_loop.increment);
            mask_ast_free(node->data.for_loop.body);
            break;

        case MASK_NODE_RETURN:
            mask_ast_free(node->data.return_stmt.value);
            break;

        case MASK_NODE_FUNCTION_DEF:
            free(node->data.function_def.name);
            mask_ast_free(node->data.function_def.params);
            mask_ast_free(node->data.function_def.body);
            break;

        case MASK_NODE_FUNCTION_CALL:
            mask_ast_free(node->data.function_call.function);
            mask_ast_free(node->data.function_call.args);
            break;

        case MASK_NODE_ARRAY_LITERAL:
            mask_ast_list_free(node->data.array_literal.elements);
            break;

        case MASK_NODE_DICT_LITERAL:
            mask_ast_list_free(node->data.dict_literal.keys);
            mask_ast_list_free(node->data.dict_literal.values);
            break;

        case MASK_NODE_MEMBER_ACCESS:
            mask_ast_free(node->data.member_access.object);
            free(node->data.member_access.member);
            break;

        case MASK_NODE_INDEX_ACCESS:
            mask_ast_free(node->data.index_access.object);
            mask_ast_free(node->data.index_access.index);
            break;

        case MASK_NODE_VAR_DECL:
            free(node->data.var_decl.name);
            mask_ast_free(node->data.var_decl.init_value);
            if (node->data.var_decl.type_hint) {
                free(node->data.var_decl.type_hint);
            }
            break;

        case MASK_NODE_EXPR_STMT:
            mask_ast_free(node->data.expr_stmt.expr);
            break;

        case MASK_NODE_IMPORT:
            free(node->data.import.module_name);
            mask_ast_list_free(node->data.import.imported_names);
            break;

        case MASK_NODE_BLOCK:
            mask_ast_list_free(node->data.block.statements);
            break;

        case MASK_NODE_PROGRAM:
            mask_ast_list_free(node->data.program.statements);
            break;

        default:
            // Nodes with no dynamic memory (literals, break, continue, null)
            break;
    }

    free(node);
}

void mask_ast_list_free(MaskASTList* list) {
    while (list) {
        MaskASTList* next = list->next;
        mask_ast_free(list->node);
        free(list);
        list = next;
    }
}
