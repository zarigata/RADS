#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    
    fprintf(stderr, "[Line %d, Col %d] Error", token->line, token->column);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->previous, message);
}

static void error_at_current(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;
        
        error_at_current(parser, parser->current.start);
    }
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    
    error_at_current(parser, message);
}

// Forward declarations
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_declaration(Parser* parser);

// Parse primary expressions
static ASTNode* parse_primary(Parser* parser) {
    if (match(parser, TOKEN_INTEGER)) {
        char* end;
        long long value = strtoll(parser->previous.start, &end, 10);
        return ast_create_integer(value, parser->previous.line, parser->previous.column);
    }
    
    if (match(parser, TOKEN_FLOAT)) {
        char* end;
        double value = strtod(parser->previous.start, &end);
        return ast_create_float(value, parser->previous.line, parser->previous.column);
    }
    
    if (match(parser, TOKEN_STRING)) {
        // Remove quotes
        size_t len = parser->previous.length - 2;
        char* str = malloc(len + 1);
        memcpy(str, parser->previous.start + 1, len);
        str[len] = '\0';
        ASTNode* node = ast_create_string(str, parser->previous.line, parser->previous.column);
        free(str);
        return node;
    }
    
    if (match(parser, TOKEN_TRUE)) {
        return ast_create_bool(true, parser->previous.line, parser->previous.column);
    }
    
    if (match(parser, TOKEN_FALSE)) {
        return ast_create_bool(false, parser->previous.line, parser->previous.column);
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        char* name = malloc(parser->previous.length + 1);
        memcpy(name, parser->previous.start, parser->previous.length);
        name[parser->previous.length] = '\0';
        ASTNode* node = ast_create_identifier(name, parser->previous.line, parser->previous.column);
        free(name);
        return node;
    }
    
    if (match(parser, TOKEN_LEFT_PAREN)) {
        ASTNode* expr = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    error(parser, "Expected expression");
    return NULL;
}

// Parse call expressions
static ASTNode* parse_call(Parser* parser) {
    ASTNode* expr = parse_primary(parser);
    
    while (true) {
        if (match(parser, TOKEN_LEFT_PAREN)) {
            ASTList* args = ast_list_create();
            
            if (!check(parser, TOKEN_RIGHT_PAREN)) {
                do {
                    ASTNode* arg = parse_expression(parser);
                    if (arg) ast_list_append(args, arg);
                } while (match(parser, TOKEN_COMMA));
            }
            
            consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
            expr = ast_create_call(expr, args, parser->previous.line, parser->previous.column);
        } else if (match(parser, TOKEN_DOT)) {
            consume(parser, TOKEN_IDENTIFIER, "Expected property name after '.'");
            char* name = malloc(parser->previous.length + 1);
            memcpy(name, parser->previous.start, parser->previous.length);
            name[parser->previous.length] = '\0';
            expr = ast_create_member_expr(expr, name, parser->previous.line, parser->previous.column);
            free(name);
        } else {
            break;
        }
    }
    
    return expr;
}

// Parse binary expressions (simplified)
static ASTNode* parse_addition(Parser* parser) {
    ASTNode* expr = parse_call(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token op = parser->previous;
        ASTNode* right = parse_call(parser);
        OperatorType op_type = (op.type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        expr = ast_create_binary_op(op_type, expr, right, op.line, op.column);
    }
    
    return expr;
}

static ASTNode* parse_expression(Parser* parser) {
    return parse_addition(parser);
}

// Parse echo statement
static ASTNode* parse_echo_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'echo'");
    ASTNode* expr = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after statement");
    
    return ast_create_echo(expr, line, column);
}

// Parse block
static ASTNode* parse_block(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    ASTList* statements = ast_list_create();
    
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (stmt) ast_list_append(statements, stmt);
    }
    
    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after block");
    return ast_create_block(statements, line, column);
}

// Parse variable declaration
static ASTNode* parse_variable(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    bool is_turbo = (parser->previous.type == TOKEN_TURBO);
    
    TypeInfo* type = NULL;
    
    // Check for type if not just 'turbo' used as inference (optional feature, for now explicit types)
    // If turbo is used, we might still expect a type or inference
    // Simplified: turbo <type> <name> = <value>; OR <type> <name> = <value>;
    
    if (is_turbo) {
        // Consume type
        if (match(parser, TOKEN_I32) || match(parser, TOKEN_STR) || match(parser, TOKEN_BOOL)) {
             // Basic types
             char* type_name = strdup(token_type_to_string(parser->previous.type));
             type = type_info_create(type_name, false, true);
             free(type_name);
        } else {
             // Implicit type or identifier type
             // For now assume explicit type required
        }
    } else {
         // Non-turbo type (already consumed by caller?)
         // We need to handle this in parse_declaration
         char* type_name = strdup(token_type_to_string(parser->previous.type));
         type = type_info_create(type_name, false, false);
         free(type_name);
    }
    
    consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    char* name = malloc(parser->previous.length + 1);
    memcpy(name, parser->previous.start, parser->previous.length);
    name[parser->previous.length] = '\0';
    
    ASTNode* initializer = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        initializer = parse_expression(parser);
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    
    ASTNode* node = ast_create_variable_decl(name, type, initializer, is_turbo, line, column);
    free(name);
    return node;
}

// Parse function declaration
static ASTNode* parse_function(Parser* parser) {
    bool is_async = false;
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    // Function name
    consume(parser, TOKEN_IDENTIFIER, "Expected function name");
    char* name = malloc(parser->previous.length + 1);
    memcpy(name, parser->previous.start, parser->previous.length);
    name[parser->previous.length] = '\0';
    
    // Parameters
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name");
    ASTList* params = ast_list_create();
    
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        // TODO: Parse parameters
    }
    
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    
    // Body
    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before function body");
    ASTNode* body = parse_block(parser);
    
    ASTNode* func = ast_create_function_decl(name, params, NULL, body, is_async, line, column);
    free(name);
    return func;
}

// Parse declaration
static ASTNode* parse_declaration(Parser* parser) {
    if (match(parser, TOKEN_BLAST)) {
        return parse_function(parser);
    }
    
    // Statements (including variable decls inside blocks, but here we are at top level?)
    // Actually RADS allows top level vars? Maybe.
    // For blocks, we use parse_statement.
    
    // Let's handle statements
    return parse_statement(parser);
}

// Parse statement
static ASTNode* parse_statement(Parser* parser) {
    if (match(parser, TOKEN_ECHO)) {
        return parse_echo_statement(parser);
    }
    
    if (match(parser, TOKEN_LEFT_BRACE)) {
        return parse_block(parser);
    }
    
    if (match(parser, TOKEN_IF)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
        ASTNode* condition = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after condition");
        ASTNode* then_branch = parse_statement(parser);
        ASTNode* else_branch = NULL;
        if (match(parser, TOKEN_ELSE)) {
            else_branch = parse_statement(parser);
        }
        return ast_create_if(condition, then_branch, else_branch, line, column);
    }
    
    // Variable declaration checks
    if (match(parser, TOKEN_TURBO) || match(parser, TOKEN_STR) || match(parser, TOKEN_I32) || match(parser, TOKEN_BOOL)) {
        return parse_variable(parser);
    }
    
    // Expression statement
    ASTNode* expr = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
    return expr;
}

// Initialize parser
void parser_init(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    advance(parser);
}

// Main parse function
ASTNode* parser_parse(Parser* parser) {
    ASTList* declarations = ast_list_create();
    
    while (!match(parser, TOKEN_EOF)) {
        ASTNode* decl = parse_declaration(parser);
        if (decl) ast_list_append(declarations, decl);
    }
    
    if (parser->had_error) {
        ast_list_free(declarations);
        return NULL;
    }
    
    return ast_create_program(declarations);
}
