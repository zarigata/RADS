#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// AST List functions
ASTList* ast_list_create() {
    ASTList* list = malloc(sizeof(ASTList));
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

void ast_list_append(ASTList* list, ASTNode* node) {
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->nodes = realloc(list->nodes, new_capacity * sizeof(ASTNode*));
        list->capacity = new_capacity;
    }
    list->nodes[list->count++] = node;
}

void ast_list_free(ASTList* list) {
    if (list) {
        for (size_t i = 0; i < list->count; i++) {
            ast_free(list->nodes[i]);
        }
        free(list->nodes);
        free(list);
    }
}

// Type info functions
TypeInfo* type_info_create(const char* name, bool is_array, bool is_turbo) {
    TypeInfo* type = malloc(sizeof(TypeInfo));
    type->name = strdup(name);
    type->is_array = is_array;
    type->is_turbo = is_turbo;
    return type;
}

void type_info_free(TypeInfo* type) {
    if (type) {
        free(type->name);
        free(type);
    }
}

// AST creation functions
ASTNode* ast_create_integer(long long value, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_INTEGER_LITERAL;
    node->line = line;
    node->column = column;
    node->integer_literal.value = value;
    return node;
}

ASTNode* ast_create_float(double value, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FLOAT_LITERAL;
    node->line = line;
    node->column = column;
    node->float_literal.value = value;
    return node;
}

ASTNode* ast_create_string(const char* value, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_STRING_LITERAL;
    node->line = line;
    node->column = column;
    node->string_literal.value = strdup(value);
    return node;
}

ASTNode* ast_create_bool(bool value, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_BOOL_LITERAL;
    node->line = line;
    node->column = column;
    node->bool_literal.value = value;
    return node;
}

ASTNode* ast_create_identifier(const char* name, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->line = line;
    node->column = column;
    node->identifier.name = strdup(name);
    return node;
}

ASTNode* ast_create_binary_op(OperatorType op, ASTNode* left, ASTNode* right, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_BINARY_OP;
    node->line = line;
    node->column = column;
    node->binary_op.op = op;
    node->binary_op.left = left;
    node->binary_op.right = right;
    return node;
}

ASTNode* ast_create_unary_op(OperatorType op, ASTNode* operand, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_UNARY_OP;
    node->line = line;
    node->column = column;
    node->unary_op.op = op;
    node->unary_op.operand = operand;
    return node;
}

ASTNode* ast_create_function_decl(const char* name, ASTList* params, TypeInfo* return_type, ASTNode* body, bool is_async, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FUNCTION_DECL;
    node->line = line;
    node->column = column;
    node->function_decl.name = strdup(name);
    node->function_decl.parameters = params;
    node->function_decl.return_type = return_type;
    node->function_decl.body = body;
    node->function_decl.is_async = is_async;
    return node;
}

ASTNode* ast_create_variable_decl(const char* name, TypeInfo* type, ASTNode* initializer, bool is_turbo, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_VARIABLE_DECL;
    node->line = line;
    node->column = column;
    node->variable_decl.name = strdup(name);
    node->variable_decl.var_type = type;
    node->variable_decl.initializer = initializer;
    node->variable_decl.is_turbo = is_turbo;
    return node;
}

ASTNode* ast_create_return(ASTNode* value, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_RETURN_STMT;
    node->line = line;
    node->column = column;
    node->return_stmt.value = value;
    return node;
}

ASTNode* ast_create_if(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IF_STMT;
    node->line = line;
    node->column = column;
    node->if_stmt.condition = condition;
    node->if_stmt.then_branch = then_branch;
    node->if_stmt.else_branch = else_branch;
    return node;
}

ASTNode* ast_create_loop(ASTNode* condition, ASTNode* body, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_LOOP_STMT;
    node->line = line;
    node->column = column;
    node->loop_stmt.condition = condition;
    node->loop_stmt.body = body;
    return node;
}

ASTNode* ast_create_echo(ASTNode* expression, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ECHO_STMT;
    node->line = line;
    node->column = column;
    node->echo_stmt.expression = expression;
    return node;
}

ASTNode* ast_create_block(ASTList* statements, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->line = line;
    node->column = column;
    node->block.statements = statements;
    return node;
}

ASTNode* ast_create_call(ASTNode* callee, ASTList* arguments, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CALL_EXPR;
    node->line = line;
    node->column = column;
    node->call_expr.callee = callee;
    node->call_expr.arguments = arguments;
    return node;
}

ASTNode* ast_create_assign(ASTNode* target, ASTNode* value, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN_EXPR;
    node->line = line;
    node->column = column;
    node->assign_expr.target = target;
    node->assign_expr.value = value;
    return node;
}

ASTNode* ast_create_member_expr(ASTNode* object, const char* member, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_MEMBER_EXPR;
    node->line = line;
    node->column = column;
    node->member_expr.object = object;
    node->member_expr.member = strdup(member);
    return node;
}

ASTNode* ast_create_program(ASTList* declarations) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->line = 0;
    node->column = 0;
    node->program.declarations = declarations;
    return node;
}

// AST cleanup
void ast_free(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_STRING_LITERAL:
            free(node->string_literal.value);
            break;
        case AST_IDENTIFIER:
            free(node->identifier.name);
            break;
        case AST_BINARY_OP:
            ast_free(node->binary_op.left);
            ast_free(node->binary_op.right);
            break;
        case AST_UNARY_OP:
            ast_free(node->unary_op.operand);
            break;
        case AST_FUNCTION_DECL:
            free(node->function_decl.name);
            ast_list_free(node->function_decl.parameters);
            type_info_free(node->function_decl.return_type);
            ast_free(node->function_decl.body);
            break;
        case AST_VARIABLE_DECL:
            free(node->variable_decl.name);
            type_info_free(node->variable_decl.var_type);
            ast_free(node->variable_decl.initializer);
            break;
        case AST_RETURN_STMT:
            ast_free(node->return_stmt.value);
            break;
        case AST_IF_STMT:
            ast_free(node->if_stmt.condition);
            ast_free(node->if_stmt.then_branch);
            ast_free(node->if_stmt.else_branch);
            break;
        case AST_LOOP_STMT:
            ast_free(node->loop_stmt.condition);
            ast_free(node->loop_stmt.body);
            break;
        case AST_ECHO_STMT:
            ast_free(node->echo_stmt.expression);
            break;
        case AST_BLOCK:
            ast_list_free(node->block.statements);
            break;
        case AST_CALL_EXPR:
            ast_free(node->call_expr.callee);
            ast_list_free(node->call_expr.arguments);
            break;
        case AST_ASSIGN_EXPR:
            ast_free(node->assign_expr.target);
            ast_free(node->assign_expr.value);
            break;
        case AST_PROGRAM:
            ast_list_free(node->program.declarations);
            break;
        default:
            break;
    }
    
    free(node);
}

// AST printing (for debugging)
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(ASTNode* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    
    switch (node->type) {
        case AST_INTEGER_LITERAL:
            printf("INTEGER: %lld\n", node->integer_literal.value);
            break;
        case AST_FLOAT_LITERAL:
            printf("FLOAT: %f\n", node->float_literal.value);
            break;
        case AST_STRING_LITERAL:
            printf("STRING: \"%s\"\n", node->string_literal.value);
            break;
        case AST_BOOL_LITERAL:
            printf("BOOL: %s\n", node->bool_literal.value ? "true" : "false");
            break;
        case AST_IDENTIFIER:
            printf("IDENTIFIER: %s\n", node->identifier.name);
            break;
        case AST_BINARY_OP:
            printf("BINARY_OP\n");
            ast_print(node->binary_op.left, indent + 1);
            ast_print(node->binary_op.right, indent + 1);
            break;
        case AST_FUNCTION_DECL:
            printf("FUNCTION: %s\n", node->function_decl.name);
            ast_print(node->function_decl.body, indent + 1);
            break;
        case AST_ECHO_STMT:
            printf("ECHO\n");
            ast_print(node->echo_stmt.expression, indent + 1);
            break;
        case AST_BLOCK:
            printf("BLOCK (%zu statements)\n", node->block.statements->count);
            for (size_t i = 0; i < node->block.statements->count; i++) {
                ast_print(node->block.statements->nodes[i], indent + 1);
            }
            break;
        case AST_CALL_EXPR:
            printf("CALL\n");
            ast_print(node->call_expr.callee, indent + 1);
            for (size_t i = 0; i < node->call_expr.arguments->count; i++) {
                ast_print(node->call_expr.arguments->nodes[i], indent + 1);
            }
            break;
        case AST_PROGRAM:
            printf("PROGRAM (%zu declarations)\n", node->program.declarations->count);
            for (size_t i = 0; i < node->program.declarations->count; i++) {
                ast_print(node->program.declarations->nodes[i], indent + 1);
            }
            break;
        default:
            printf("UNKNOWN NODE TYPE\n");
            break;
    }
}
