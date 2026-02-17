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

ASTNode* ast_create_null(int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NULL_LITERAL;
    node->line = line;
    node->column = column;
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

ASTNode* ast_create_typeof(ASTNode* operand, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_TYPEOF_EXPR;
    node->line = line;
    node->column = column;
    node->typeof_expr.operand = operand;
    return node;
}

ASTNode* ast_create_function_decl(const char* name, ASTList* params, TypeInfo* return_type, ASTNode* body, bool is_async, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FUNCTION_DECL;
    node->line = line;
    node->column = column;
    node->function_decl.name = name ? strdup(name) : NULL;
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
    node->variable_decl.name = name ? strdup(name) : NULL;
    node->variable_decl.var_type = type;
    node->variable_decl.initializer = initializer;
    node->variable_decl.is_turbo = is_turbo;
    node->variable_decl.destructure_pattern = NULL;
    return node;
}

ASTNode* ast_create_struct_decl(const char* name, ASTList* fields, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_STRUCT_DECL;
    node->line = line;
    node->column = column;
    node->struct_decl.name = strdup(name);
    node->struct_decl.fields = fields;
    return node;
}

ASTNode* ast_create_enum_decl(const char* name, ASTList* values, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ENUM_DECL;
    node->line = line;
    node->column = column;
    node->enum_decl.name = strdup(name);
    node->enum_decl.values = values;
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

ASTNode* ast_create_import(const char* filename, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IMPORT_STMT;
    node->line = line;
    node->column = column;
    node->import_stmt.filename = strdup(filename);
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

ASTNode* ast_create_cruise(const char* iterator, ASTNode* iterable, ASTNode* body, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CRUISE_STMT;
    node->line = line;
    node->column = column;
    node->cruise_stmt.iterator = strdup(iterator);
    node->cruise_stmt.iterable = iterable;
    node->cruise_stmt.body = body;
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

ASTNode* ast_create_struct_literal(const char* name, ASTList* fields, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_STRUCT_LITERAL;
    node->line = line;
    node->column = column;
    node->struct_literal.name = strdup(name);
    node->struct_literal.fields = fields;
    return node;
}

ASTNode* ast_create_array_literal(ASTList* elements, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ARRAY_LITERAL;
    node->line = line;
    node->column = column;
    node->array_literal.elements = elements;
    return node;
}

ASTNode* ast_create_index(ASTNode* array, ASTNode* index, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_INDEX_EXPR;
    node->line = line;
    node->column = column;
    node->index_expr.array = array;
    node->index_expr.index = index;
    return node;
}

ASTNode* ast_create_break(int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_BREAK_STMT;
    node->line = line;
    node->column = column;
    return node;
}

ASTNode* ast_create_continue(int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CONTINUE_STMT;
    node->line = line;
    node->column = column;
    return node;
}

ASTNode* ast_create_try(ASTNode* try_block, const char* catch_var, ASTNode* catch_block, ASTNode* finally_block, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_TRY_STMT;
    node->line = line;
    node->column = column;
    node->try_stmt.try_block = try_block;
    node->try_stmt.catch_var = catch_var ? strdup(catch_var) : NULL;
    node->try_stmt.catch_block = catch_block;
    node->try_stmt.finally_block = finally_block;
    return node;
}

ASTNode* ast_create_throw(ASTNode* expression, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_THROW_STMT;
    node->line = line;
    node->column = column;
    node->throw_stmt.expression = expression;
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

ASTNode* ast_create_spread(ASTNode* expression, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_SPREAD_EXPR;
    node->line = line;
    node->column = column;
    node->spread_expr.expression = expression;
    return node;
}

ASTNode* ast_create_destructure_array(ASTList* elements, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_DESTRUCTURE_ARRAY;
    node->line = line;
    node->column = column;
    node->destructure_array.elements = elements;
    return node;
}

ASTNode* ast_create_destructure_struct(ASTList* fields, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_DESTRUCTURE_STRUCT;
    node->line = line;
    node->column = column;
    node->destructure_struct.fields = fields;
    return node;
}

ASTNode* ast_create_destructure_rest(const char* name, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_DESTRUCTURE_REST;
    node->line = line;
    node->column = column;
    node->destructure_rest.name = strdup(name);
    return node;
}

ASTNode* ast_create_destructure_skip(int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_DESTRUCTURE_SKIP;
    node->line = line;
    node->column = column;
    return node;
}

ASTNode* ast_create_optional_chain_member(ASTNode* object, const char* member, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_OPTIONAL_CHAIN;
    node->line = line;
    node->column = column;
    node->optional_chain.object = object;
    node->optional_chain.is_member = true;
    node->optional_chain.member = strdup(member);
    node->optional_chain.index = NULL;
    return node;
}

ASTNode* ast_create_optional_chain_index(ASTNode* object, ASTNode* index, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_OPTIONAL_CHAIN;
    node->line = line;
    node->column = column;
    node->optional_chain.object = object;
    node->optional_chain.is_member = false;
    node->optional_chain.member = NULL;
    node->optional_chain.index = index;
    return node;
}

ASTNode* ast_create_nullish_coalescing(ASTNode* left, ASTNode* right, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NULLISH_COALESCING;
    node->line = line;
    node->column = column;
    node->nullish_coalescing.left = left;
    node->nullish_coalescing.right = right;
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
            ast_free(node->variable_decl.destructure_pattern);
            break;
        case AST_STRUCT_DECL:
            free(node->struct_decl.name);
            ast_list_free(node->struct_decl.fields);
            break;
        case AST_STRUCT_LITERAL:
            free(node->struct_literal.name);
            ast_list_free(node->struct_literal.fields);
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
        case AST_CRUISE_STMT:
            free(node->cruise_stmt.iterator);
            ast_free(node->cruise_stmt.iterable);
            ast_free(node->cruise_stmt.body);
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
        case AST_ARRAY_LITERAL:
            ast_list_free(node->array_literal.elements);
            break;
        case AST_INDEX_EXPR:
            ast_free(node->index_expr.array);
            ast_free(node->index_expr.index);
            break;
        case AST_ASSIGN_EXPR:
            ast_free(node->assign_expr.target);
            ast_free(node->assign_expr.value);
            break;
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
            break;
        case AST_TRY_STMT:
            ast_free(node->try_stmt.try_block);
            if (node->try_stmt.catch_var) free(node->try_stmt.catch_var);
            ast_free(node->try_stmt.catch_block);
            ast_free(node->try_stmt.finally_block);
            break;
        case AST_THROW_STMT:
            ast_free(node->throw_stmt.expression);
            break;
        case AST_PROGRAM:
            ast_list_free(node->program.declarations);
            break;
        case AST_SPREAD_EXPR:
            ast_free(node->spread_expr.expression);
            break;
        case AST_DESTRUCTURE_ARRAY:
            ast_list_free(node->destructure_array.elements);
            break;
        case AST_DESTRUCTURE_STRUCT:
            ast_list_free(node->destructure_struct.fields);
            break;
        case AST_DESTRUCTURE_REST:
            free(node->destructure_rest.name);
            break;
        case AST_DESTRUCTURE_SKIP:
            break;
        case AST_TYPEOF_EXPR:
            ast_free(node->typeof_expr.operand);
            break;
        case AST_ENUM_DECL:
            free(node->enum_decl.name);
            ast_list_free(node->enum_decl.values);
            break;
        case AST_MEMBER_EXPR:
            ast_free(node->member_expr.object);
            free(node->member_expr.member);
            break;
        case AST_IMPORT_STMT:
            free(node->import_stmt.filename);
            break;
        case AST_OPTIONAL_CHAIN:
            ast_free(node->optional_chain.object);
            if (node->optional_chain.member) free(node->optional_chain.member);
            ast_free(node->optional_chain.index);
            break;
        case AST_NULLISH_COALESCING:
            ast_free(node->nullish_coalescing.left);
            ast_free(node->nullish_coalescing.right);
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
