#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ANSI color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_GRAY    "\033[0;90m"
#define COLOR_WHITE   "\033[1;37m"

// Extract a specific line from source code
static const char* get_source_line(const char* source, int target_line, int* line_length) {
    const char* line_start = source;
    int current_line = 1;

    // Find the start of the target line
    while (*line_start && current_line < target_line) {
        if (*line_start == '\n') {
            current_line++;
            line_start++;
        } else {
            line_start++;
        }
    }

    // Find the end of the line
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }

    *line_length = (int)(line_end - line_start);
    return line_start;
}

// Suggest fixes for common typos
static const char* suggest_fix(const char* token_str, size_t token_len) {
    // Common keyword typos
    struct { const char* typo; const char* fix; } suggestions[] = {
        {"function", "blast"},
        {"var", "turbo"},
        {"let", "turbo"},
        {"const", "turbo"},
        {"print", "echo"},
        {"println", "echo"},
        {"while", "loop"},
        {"for", "cruise"},
        {"elif", "elif"},
        {"elseif", "elif"},
        {"else if", "elif"},
        {NULL, NULL}
    };

    for (int i = 0; suggestions[i].typo != NULL; i++) {
        if (strlen(suggestions[i].typo) == token_len &&
            strncmp(token_str, suggestions[i].typo, token_len) == 0) {
            return suggestions[i].fix;
        }
    }

    return NULL;
}

/* error_at - Enhanced error reporting with colors and context
 * Shows:
 * - Colored error header with file location
 * - Source code line with the error
 * - Column pointer (^) indicating exact position
 * - Helpful suggestions for common mistakes
 */
static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;

    int line = (token && token->line > 0) ? token->line : 1;
    int col = (token && token->column > 0) ? token->column : 1;

    // Error header with color
    fprintf(stderr, "\n%sError:%s ", COLOR_RED, COLOR_RESET);
    fprintf(stderr, "%s\n", message);

    // Location info
    fprintf(stderr, "%s  --> %sLine %d, Column %d%s\n",
            COLOR_GRAY, COLOR_CYAN, line, col, COLOR_RESET);

    // Show source code context if available
    if (parser->lexer && parser->lexer->source) {
        int line_length = 0;
        const char* source_line = get_source_line(parser->lexer->source, line, &line_length);

        if (line_length > 0) {
            // Line number prefix
            fprintf(stderr, "%s%4d |%s ", COLOR_GRAY, line, COLOR_RESET);

            // Source line
            fprintf(stderr, "%.*s\n", line_length, source_line);

            // Column pointer
            fprintf(stderr, "%s     |%s ", COLOR_GRAY, COLOR_RESET);
            for (int i = 1; i < col; i++) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "%s^%s", COLOR_RED, COLOR_RESET);

            // Underline the token if it has length
            if (token && token->length > 1) {
                for (size_t i = 1; i < token->length && i < 20; i++) {
                    fprintf(stderr, "%s~%s", COLOR_RED, COLOR_RESET);
                }
            }
            fprintf(stderr, "\n");
        }
    }

    // Show token info
    if (token) {
        if (token->type == TOKEN_EOF) {
            fprintf(stderr, "%s     |%s at end of file\n", COLOR_GRAY, COLOR_RESET);
        } else if (token->type != TOKEN_ERROR && token->length > 0) {
            fprintf(stderr, "%s     |%s found: '%.*s' (type: %s)\n",
                    COLOR_GRAY, COLOR_RESET,
                    (int)token->length, token->start,
                    token_type_to_string(token->type));

            // Suggest fix for common typos
            if (token->type == TOKEN_IDENTIFIER) {
                const char* suggestion = suggest_fix(token->start, token->length);
                if (suggestion) {
                    fprintf(stderr, "%s     |%s\n", COLOR_GRAY, COLOR_RESET);
                    fprintf(stderr, "%s     = help:%s Did you mean '%s%s%s'?\n",
                            COLOR_GRAY, COLOR_RESET,
                            COLOR_YELLOW, suggestion, COLOR_RESET);
                }
            }
        }
    }

    fprintf(stderr, "\n");
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
ASTNode* parse_statement(Parser* parser);  // Public for REPL
static ASTNode* parse_declaration(Parser* parser);
static ASTNode* parse_comparison(Parser* parser);
static ASTNode* parse_block(Parser* parser);

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

    if (match(parser, TOKEN_NULL)) {
        return ast_create_null(parser->previous.line, parser->previous.column);
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        char* name = malloc(parser->previous.length + 1);
        memcpy(name, parser->previous.start, parser->previous.length);
        name[parser->previous.length] = '\0';
        ASTNode* node = ast_create_identifier(name, parser->previous.line, parser->previous.column);
        free(name);
        return node;
    }

    // Module names that are also type keywords (array, string, etc.)
    // These can be used as identifiers when followed by '.' for method calls
    if (match(parser, TOKEN_ARRAY) || match(parser, TOKEN_STR)) {
        const char* module_name = token_type_to_string(parser->previous.type);
        char* name = strdup(module_name);
        // Convert to lowercase for consistency (STR -> str, ARRAY -> array)
        for (char* p = name; *p; p++) *p = tolower(*p);
        ASTNode* node = ast_create_identifier(name, parser->previous.line, parser->previous.column);
        free(name);
        return node;
    }
    
    if (match(parser, TOKEN_LEFT_BRACKET)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        ASTList* elements = ast_list_create();
        if (!check(parser, TOKEN_RIGHT_BRACKET)) {
            do {
                if (match(parser, TOKEN_DOT_DOT_DOT)) {
                    int spread_line = parser->previous.line;
                    int spread_col = parser->previous.column;
                    ASTNode* expr = parse_expression(parser);
                    ASTNode* spread = ast_create_spread(expr, spread_line, spread_col);
                    if (spread) ast_list_append(elements, spread);
                } else {
                    ASTNode* elem = parse_expression(parser);
                    if (elem) ast_list_append(elements, elem);
                }
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after array literal");
        return ast_create_array_literal(elements, line, column);
    }
    
    if (match(parser, TOKEN_LEFT_PAREN)) {
        ASTNode* expr = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    // Anonymous function: blast(params) { body }
    if (match(parser, TOKEN_BLAST)) {
        int line = parser->previous.line;
        int column = parser->previous.column;

        consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'blast' for anonymous function");
        ASTList* params = ast_list_create();
        if (!check(parser, TOKEN_RIGHT_PAREN)) {
            do {
                consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
                char* param_name = malloc(parser->previous.length + 1);
                memcpy(param_name, parser->previous.start, parser->previous.length);
                param_name[parser->previous.length] = '\0';
                ASTNode* param = ast_create_identifier(param_name, parser->previous.line, parser->previous.column);
                ast_list_append(params, param);
                free(param_name);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

        consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before function body");
        ASTNode* body = parse_block(parser);

        return ast_create_function_decl(NULL, params, NULL, body, false, line, column);
    }
    
    error(parser, "Expected expression");
    return NULL;
}

// Parse call expressions
static ASTNode* parse_call(Parser* parser) {
    ASTNode* expr = parse_primary(parser);
    
    while (true) {
        if (match(parser, TOKEN_LEFT_BRACE)) { // Struct literal
            if (expr->type != AST_IDENTIFIER) {
                error(parser, "Expected struct name before '{'");
                return NULL;
            }
            char* struct_name = expr->identifier.name;
            int line = parser->previous.line;
            int column = parser->previous.column;

            ASTList* fields = ast_list_create();
            if (!check(parser, TOKEN_RIGHT_BRACE)) {
                do {
                    consume(parser, TOKEN_IDENTIFIER, "Expected field name in struct literal");
                    char* field_name = malloc(parser->previous.length + 1);
                    memcpy(field_name, parser->previous.start, parser->previous.length);
                    field_name[parser->previous.length] = '\0';
                    ASTNode* field_ident = ast_create_identifier(field_name, parser->previous.line, parser->previous.column);
                    free(field_name);

                    consume(parser, TOKEN_COLON, "Expected ':' after field name");

                    ASTNode* value = parse_expression(parser);
                    ASTNode* assign = ast_create_assign(field_ident, value, parser->previous.line, parser->previous.column);
                    ast_list_append(fields, assign);
                } while (match(parser, TOKEN_COMMA));
            }
            consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after struct literal");

            expr = ast_create_struct_literal(struct_name, fields, line, column);

        } else if (match(parser, TOKEN_LEFT_PAREN)) {
            ASTList* args = ast_list_create();
            
            if (!check(parser, TOKEN_RIGHT_PAREN)) {
                do {
                    ASTNode* arg = parse_expression(parser);
                    if (arg) ast_list_append(args, arg);
                } while (match(parser, TOKEN_COMMA));
            }
            
            consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
            expr = ast_create_call(expr, args, parser->previous.line, parser->previous.column);
        } else if (match(parser, TOKEN_QUESTION_DOT)) {
            int line = parser->previous.line;
            int column = parser->previous.column;
            if (match(parser, TOKEN_IDENTIFIER)) {
                char* name = malloc(parser->previous.length + 1);
                memcpy(name, parser->previous.start, parser->previous.length);
                name[parser->previous.length] = '\0';
                expr = ast_create_optional_chain_member(expr, name, line, column);
                free(name);
            } else if (match(parser, TOKEN_LEFT_BRACKET)) {
                ASTNode* index = parse_expression(parser);
                consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after optional index");
                expr = ast_create_optional_chain_index(expr, index, line, column);
            } else {
                error(parser, "Expected property name or index after '?.'");
            }
        } else if (match(parser, TOKEN_DOT)) {
            consume(parser, TOKEN_IDENTIFIER, "Expected property name after '.'");
            char* name = malloc(parser->previous.length + 1);
            memcpy(name, parser->previous.start, parser->previous.length);
            name[parser->previous.length] = '\0';
            expr = ast_create_member_expr(expr, name, parser->previous.line, parser->previous.column);
            free(name);
        } else if (match(parser, TOKEN_LEFT_BRACKET)) {
            ASTNode* index = parse_expression(parser);
            consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index expression");
            expr = ast_create_index(expr, index, parser->previous.line, parser->previous.column);
        } else {
            break;
        }
    }
    
    return expr;
}

// Parse unary operators (!, -, typeof)
static ASTNode* parse_unary(Parser* parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_MINUS)) {
        Token op = parser->previous;
        int line = op.line;
        int column = op.column;
        ASTNode* right = parse_unary(parser);  // Right-associative
        OperatorType op_type = (op.type == TOKEN_BANG) ? OP_NOT : OP_NEG;
        return ast_create_unary_op(op_type, right, line, column);
    }

    if (match(parser, TOKEN_TYPEOF)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        ASTNode* operand = parse_unary(parser);
        return ast_create_typeof(operand, line, column);
    }

    return parse_call(parser);
}

static ASTNode* parse_factor(Parser* parser) {
    ASTNode* expr = parse_unary(parser);

    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        Token op = parser->previous;
        ASTNode* right = parse_unary(parser);
        OperatorType op_type;
        if (op.type == TOKEN_STAR) op_type = OP_MUL;
        else if (op.type == TOKEN_SLASH) op_type = OP_DIV;
        else op_type = OP_MOD;
        expr = ast_create_binary_op(op_type, expr, right, op.line, op.column);
    }

    return expr;
}

static ASTNode* parse_term(Parser* parser) {
    ASTNode* expr = parse_factor(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token op = parser->previous;
        ASTNode* right = parse_factor(parser);
        OperatorType op_type = (op.type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        expr = ast_create_binary_op(op_type, expr, right, op.line, op.column);
    }
    
    return expr;
}

static ASTNode* parse_comparison(Parser* parser) {
    ASTNode* expr = parse_term(parser);
    
    while (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_BANG_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL) ||
           match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL)) {
        Token op = parser->previous;
        ASTNode* right = parse_term(parser);
        OperatorType op_type;
        if (op.type == TOKEN_EQUAL_EQUAL) op_type = OP_EQ;
        else if (op.type == TOKEN_BANG_EQUAL) op_type = OP_NEQ;
        else if (op.type == TOKEN_LESS) op_type = OP_LT;
        else if (op.type == TOKEN_LESS_EQUAL) op_type = OP_LTE;
        else if (op.type == TOKEN_GREATER) op_type = OP_GT;
        else op_type = OP_GTE;
        expr = ast_create_binary_op(op_type, expr, right, op.line, op.column);
    }
    
    return expr;
}

static ASTNode* parse_and(Parser* parser) {
    ASTNode* expr = parse_comparison(parser);

    while (match(parser, TOKEN_AND_AND)) {
        Token op = parser->previous;
        ASTNode* right = parse_comparison(parser);
        expr = ast_create_binary_op(OP_AND, expr, right, op.line, op.column);
    }

    return expr;
}

static ASTNode* parse_or(Parser* parser) {
    ASTNode* expr = parse_and(parser);

    while (match(parser, TOKEN_OR_OR)) {
        Token op = parser->previous;
        ASTNode* right = parse_and(parser);
        expr = ast_create_binary_op(OP_OR, expr, right, op.line, op.column);
    }

    return expr;
}

static ASTNode* parse_nullish_coalescing(Parser* parser) {
    ASTNode* expr = parse_or(parser);

    while (match(parser, TOKEN_QUESTION_QUESTION)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        ASTNode* right = parse_or(parser);
        expr = ast_create_nullish_coalescing(expr, right, line, column);
    }

    return expr;
}

// Range has low precedence: <add> .. <add>
static ASTNode* parse_range(Parser* parser) {
    ASTNode* expr = parse_nullish_coalescing(parser);
    if (match(parser, TOKEN_DOT_DOT)) {
        Token op = parser->previous;
        ASTNode* right = parse_nullish_coalescing(parser);
        expr = ast_create_binary_op(OP_RANGE, expr, right, op.line, op.column);
    }
    return expr;
}

// Assignment (right-associative): target = value
static ASTNode* parse_assignment(Parser* parser) {
    ASTNode* expr = parse_range(parser);
    
    if (match(parser, TOKEN_EQUAL)) {
        Token equals = parser->previous;
        ASTNode* value = parse_assignment(parser);
        
        if (expr->type == AST_IDENTIFIER || expr->type == AST_INDEX_EXPR || expr->type == AST_MEMBER_EXPR) {
            return ast_create_assign(expr, value, equals.line, equals.column);
        }
        
        error_at(parser, &equals, "Invalid assignment target");
    }
    
    return expr;
}

static ASTNode* parse_expression(Parser* parser) {
    return parse_assignment(parser);
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

// Parse return statement
static ASTNode* parse_return_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    ASTNode* value = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        value = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after return value");
    return ast_create_return(value, line, column);
}

// Parse await statement: await <expr>;
static ASTNode* parse_await_statement(Parser* parser) {
    (void)parser->previous; // unused for now
    ASTNode* value = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after await expression");
    // For now, await is a no-op; just evaluate the expression.
    // In the future, we can add async/await semantics.
    return value;
}

// Parse loop statement: loop (<condition>) <statement>
static ASTNode* parse_loop_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'loop'");
    ASTNode* condition = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after loop condition");
    ASTNode* body = parse_statement(parser);
    
    return ast_create_loop(condition, body, line, column);
}

// Parse break/continue statements
static ASTNode* parse_break_or_continue(Parser* parser, bool is_break) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after statement");
    return is_break ? ast_create_break(line, column) : ast_create_continue(line, column);
}

// Parse cruise (for-range or C-style for) statement
// cruise (id in iterable) <body> - for-range
// cruise (init; condition; update) <body> - C-style for
static ASTNode* parse_cruise_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'cruise'");
    
    // Check if this is for-range (id in iterable) or C-style for (init; cond; update)
    // Peek at what follows the identifier
    consume(parser, TOKEN_IDENTIFIER, "Expected variable name after '('");
    char* var_name = malloc(parser->previous.length + 1);
    memcpy(var_name, parser->previous.start, parser->previous.length);
    var_name[parser->previous.length] = '\0';
    
    if (match(parser, TOKEN_IN)) {
        // For-range: cruise (id in iterable)
        ASTNode* iterable = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after cruise iterable");
        ASTNode* body = parse_statement(parser);
        ASTNode* cruise = ast_create_cruise(var_name, iterable, body, line, column);
        free(var_name);
        return cruise;
    } else {
        // C-style for: cruise (init; condition; update)
        // var_name is actually the first part of init (variable name)
        // We've already consumed the identifier, now expect = for assignment
        ASTNode* init = NULL;
        
        if (match(parser, TOKEN_EQUAL)) {
            // Simple assignment: var_name = expr
            ASTNode* ident = ast_create_identifier(var_name, line, column);
            ASTNode* value = parse_expression(parser);
            init = ast_create_assign(ident, value, line, column);
        } else {
            // Just a variable reference (init is just expr;)
            free(var_name);
            // We need to back up - the identifier we consumed might be part of an expression
            // For simplicity, we require cruise (init; cond; update) where init is an assignment
            error(parser, "Expected '=' for cruise initializer or 'in' for for-range");
            return NULL;
        }
        free(var_name);
        
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after cruise initializer");
        
        // Parse condition
        ASTNode* condition = parse_expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after cruise condition");
        
        // Parse update - this is an expression statement (without semicolon at end)
        ASTNode* update = parse_expression(parser);
        
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after cruise clauses");
        ASTNode* body = parse_statement(parser);
        
        // Desugar into: { init; loop (condition) { body; update; } }
        // Create update statement
        ASTList* update_stmts = ast_list_create();
        ast_list_append(update_stmts, update);
        ASTNode* update_block = ast_create_block(update_stmts, line, column);
        
        // Create body with update: { body_stmts... update; }
        // body should be a block, we need to add update at the end
        if (body->type == AST_BLOCK) {
            ast_list_append(body->block.statements, update);
        } else {
            // Single statement body - create a new block
            ASTList* body_stmts = ast_list_create();
            ast_list_append(body_stmts, body);
            ast_list_append(body_stmts, update);
            body = ast_create_block(body_stmts, line, column);
        }
        
        // Create loop: loop (condition) { body_with_update }
        ASTNode* loop = ast_create_loop(condition, body, line, column);
        
        // Create block: { init; loop; }
        ASTList* block_stmts = ast_list_create();
        ast_list_append(block_stmts, init);
        ast_list_append(block_stmts, loop);
        
        return ast_create_block(block_stmts, line, column);
    }
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

static ASTNode* parse_destructure_element(Parser* parser) {
    int line = parser->current.line;
    int column = parser->current.column;
    
    if (match(parser, TOKEN_DOT_DOT_DOT)) {
        consume(parser, TOKEN_IDENTIFIER, "Expected identifier after '...' in destructuring");
        char* name = malloc(parser->previous.length + 1);
        memcpy(name, parser->previous.start, parser->previous.length);
        name[parser->previous.length] = '\0';
        return ast_create_destructure_rest(name, line, column);
    }
    
    if (match(parser, TOKEN_COMMA)) {
        return ast_create_destructure_skip(line, column);
    }
    
    consume(parser, TOKEN_IDENTIFIER, "Expected identifier in destructuring pattern");
    char* name = malloc(parser->previous.length + 1);
    memcpy(name, parser->previous.start, parser->previous.length);
    name[parser->previous.length] = '\0';
    ASTNode* ident = ast_create_identifier(name, parser->previous.line, parser->previous.column);
    free(name);
    return ident;
}

static ASTNode* parse_array_destructure_pattern(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    ASTList* elements = ast_list_create();
    
    if (!check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            if (check(parser, TOKEN_RIGHT_BRACKET)) break;
            ASTNode* elem = parse_destructure_element(parser);
            if (elem) {
                if (elem->type == AST_DESTRUCTURE_SKIP) {
                    continue;
                }
                ast_list_append(elements, elem);
            }
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after destructuring pattern");
    return ast_create_destructure_array(elements, line, column);
}

static ASTNode* parse_struct_destructure_pattern(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    ASTList* fields = ast_list_create();
    
    if (!check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            if (check(parser, TOKEN_RIGHT_BRACE)) break;
            consume(parser, TOKEN_IDENTIFIER, "Expected field name in struct destructuring");
            char* field_name = malloc(parser->previous.length + 1);
            memcpy(field_name, parser->previous.start, parser->previous.length);
            field_name[parser->previous.length] = '\0';
            
            char* var_name = strdup(field_name);
            
            if (match(parser, TOKEN_COLON)) {
                consume(parser, TOKEN_IDENTIFIER, "Expected variable name after ':'");
                free(var_name);
                var_name = malloc(parser->previous.length + 1);
                memcpy(var_name, parser->previous.start, parser->previous.length);
                var_name[parser->previous.length] = '\0';
            }
            
            ASTNode* field_assign = ast_create_assign(
                ast_create_identifier(field_name, parser->previous.line, parser->previous.column),
                ast_create_identifier(var_name, parser->previous.line, parser->previous.column),
                line, column
            );
            ast_list_append(fields, field_assign);
            free(field_name);
            free(var_name);
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after struct destructuring pattern");
    return ast_create_destructure_struct(fields, line, column);
}

// Parse variable declaration
static ASTNode* parse_variable(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    bool is_turbo = (parser->previous.type == TOKEN_TURBO);
    
    TypeInfo* type = NULL;
    char* name = NULL;
    ASTNode* destructure_pattern = NULL;

    if (is_turbo) {
        if (match(parser, TOKEN_LEFT_BRACKET)) {
            destructure_pattern = parse_array_destructure_pattern(parser);
        } else if (match(parser, TOKEN_LEFT_BRACE)) {
            destructure_pattern = parse_struct_destructure_pattern(parser);
        } else if (match(parser, TOKEN_I32) || match(parser, TOKEN_STR) || match(parser, TOKEN_BOOL)) {
             char* type_name = strdup(token_type_to_string(parser->previous.type));
             type = type_info_create(type_name, false, true);
             free(type_name);
             consume(parser, TOKEN_IDENTIFIER, "Expected variable name after type");
             name = malloc(parser->previous.length + 1);
             memcpy(name, parser->previous.start, parser->previous.length);
             name[parser->previous.length] = '\0';
        } else if (check(parser, TOKEN_IDENTIFIER)) {
             consume(parser, TOKEN_IDENTIFIER, "Expected type or variable name");
             Token first_ident = parser->previous;

             if (check(parser, TOKEN_IDENTIFIER)) {
                 char* type_name = malloc(first_ident.length + 1);
                 memcpy(type_name, first_ident.start, first_ident.length);
                 type_name[first_ident.length] = '\0';
                 type = type_info_create(type_name, false, true);
                 free(type_name);
                 consume(parser, TOKEN_IDENTIFIER, "Expected variable name after type");
                 name = malloc(parser->previous.length + 1);
                 memcpy(name, parser->previous.start, parser->previous.length);
                 name[parser->previous.length] = '\0';
             } else {
                 name = malloc(first_ident.length + 1);
                 memcpy(name, first_ident.start, first_ident.length);
                 name[first_ident.length] = '\0';
                 type = NULL;
             }
        } else {
             error(parser, "Expected type, variable name, or destructuring pattern after 'turbo'");
             return NULL;
        }
    } else {
         char* type_name = strdup(token_type_to_string(parser->previous.type));
         type = type_info_create(type_name, false, false);
         free(type_name);
         consume(parser, TOKEN_IDENTIFIER, "Expected variable name after type");
         name = malloc(parser->previous.length + 1);
         memcpy(name, parser->previous.start, parser->previous.length);
         name[parser->previous.length] = '\0';
    }
    
    ASTNode* initializer = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        initializer = parse_expression(parser);
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    
    ASTNode* node = ast_create_variable_decl(name, type, initializer, is_turbo, line, column);
    if (name) free(name);
    node->variable_decl.destructure_pattern = destructure_pattern;
    return node;
}

// Parse function declaration
static ASTNode* parse_function(Parser* parser, bool is_async) {
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
    
    // Robust parameter parsing: consume optional type tokens, then identifier, until ')'.
    while (!check(parser, TOKEN_RIGHT_PAREN) && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_STREAM) || match(parser, TOKEN_STR) ||
            match(parser, TOKEN_I32) || match(parser, TOKEN_BOOL) ||
            match(parser, TOKEN_PTR)) {
            // type token consumed and ignored
        }
        consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
        char* pname = malloc(parser->previous.length + 1);
        memcpy(pname, parser->previous.start, parser->previous.length);
        pname[parser->previous.length] = '\0';
        
        int param_line = parser->previous.line;
        int param_col = parser->previous.column;
        
        ASTNode* param;
        if (match(parser, TOKEN_EQUAL)) {
            ASTNode* default_val = parse_expression(parser);
            param = ast_create_variable_decl(pname, NULL, default_val, false, param_line, param_col);
        } else {
            param = ast_create_identifier(pname, param_line, param_col);
        }
        free(pname);
        ast_list_append(params, param);
        if (!match(parser, TOKEN_COMMA)) {
            break;
        }
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

    // Optional return type arrow: accept and ignore the type token for now.
    if (check(parser, TOKEN_ARROW)) {
        advance(parser); // consume ->
        // Consume a single type token if present (ignored for now).
        if (check(parser, TOKEN_STREAM) || check(parser, TOKEN_STR) ||
            check(parser, TOKEN_I32) || check(parser, TOKEN_BOOL) ||
            check(parser, TOKEN_PTR) || check(parser, TOKEN_ARRAY) ||
            check(parser, TOKEN_ARRAY)) {
            advance(parser);
        }
    }
    
    // Body
    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before function body");
    ASTNode* body = parse_block(parser);
    
    ASTNode* func = ast_create_function_decl(name, params, NULL, body, is_async, line, column);
    free(name);
    return func;
}

static ASTNode* parse_struct_declaration(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;

    // The struct name is the token *after* 'struct'
    consume(parser, TOKEN_IDENTIFIER, "Expected struct name");
    char* name = malloc(parser->previous.length + 1);
    memcpy(name, parser->previous.start, parser->previous.length);
    name[parser->previous.length] = '\0';

    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before struct body");

    ASTList* fields = ast_list_create();
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        // Parse field type
        Token type_token;
        if (check(parser, TOKEN_STR) || check(parser, TOKEN_I32) || check(parser, TOKEN_BOOL) || check(parser, TOKEN_IDENTIFIER)) {
            type_token = parser->current;
            advance(parser); // Consume the type token
        } else {
            error_at_current(parser, "Expected type for struct field");
            break; // Exit loop on error
        }

        char* type_name;
        if (type_token.type == TOKEN_IDENTIFIER) {
            type_name = malloc(type_token.length + 1);
            memcpy(type_name, type_token.start, type_token.length);
            type_name[type_token.length] = '\0';
        } else {
            // For keyword types (STR, I32, BOOL), use token_type_to_string
            type_name = strdup(token_type_to_string(type_token.type));
        }

        // Parse field name
        consume(parser, TOKEN_IDENTIFIER, "Expected field name");
        char* field_name = malloc(parser->previous.length + 1);
        memcpy(field_name, parser->previous.start, parser->previous.length);
        field_name[parser->previous.length] = '\0';
        
        TypeInfo* type = type_info_create(type_name, false, false); // Create TypeInfo
        ASTNode* field = ast_create_variable_decl(field_name, type, NULL, false, parser->previous.line, parser->previous.column);
        ast_list_append(fields, field);
        free(field_name);
        free(type_name); // Free the duplicated type_name
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after struct field");
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after struct body");
    
    // Struct declaration can optionally end with a semicolon
    if (match(parser, TOKEN_SEMICOLON)) {
        // consumed
    }

    ASTNode* struct_decl = ast_create_struct_decl(name, fields, line, column);
    free(name);
    return struct_decl;
}

static ASTNode* parse_enum_declaration(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;

    // The enum name is the token *after* 'enum'
    consume(parser, TOKEN_IDENTIFIER, "Expected enum name");
    char* name = malloc(parser->previous.length + 1);
    memcpy(name, parser->previous.start, parser->previous.length);
    name[parser->previous.length] = '\0';

    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before enum body");

    ASTList* values = ast_list_create();
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        // Parse enum value name
        consume(parser, TOKEN_IDENTIFIER, "Expected enum value name");
        char* value_name = malloc(parser->previous.length + 1);
        memcpy(value_name, parser->previous.start, parser->previous.length);
        value_name[parser->previous.length] = '\0';

        ASTNode* value = ast_create_identifier(value_name, parser->previous.line, parser->previous.column);
        ast_list_append(values, value);
        free(value_name);

        // Allow optional comma between values
        if (!check(parser, TOKEN_RIGHT_BRACE)) {
            if (match(parser, TOKEN_COMMA)) {
                // consumed
            } else {
                // No comma, that's ok if next is closing brace
                if (!check(parser, TOKEN_RIGHT_BRACE)) {
                    error_at_current(parser, "Expected ',' or '}' in enum");
                    break;
                }
            }
        }
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after enum body");

    // Enum declaration can optionally end with a semicolon
    if (match(parser, TOKEN_SEMICOLON)) {
        // consumed
    }

    ASTNode* enum_decl = ast_create_enum_decl(name, values, line, column);
    free(name);
    return enum_decl;
}

// Parse declaration
static ASTNode* parse_declaration(Parser* parser) {
    // Import statement: import "filename.rads";
    if (match(parser, TOKEN_IMPORT)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        consume(parser, TOKEN_STRING, "Expected filename string after 'import'");
        char* filename = malloc(parser->previous.length - 1); // -2 for quotes, +1 for null
        memcpy(filename, parser->previous.start + 1, parser->previous.length - 2);
        filename[parser->previous.length - 2] = '\0';
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after import");
        ASTNode* import_node = ast_create_import(filename, line, column);
        free(filename);
        return import_node;
    }

    if (match(parser, TOKEN_STRUCT)) { // Match and consume the STRUCT token
        return parse_struct_declaration(parser);
    }

    if (match(parser, TOKEN_ENUM)) { // Match and consume the ENUM token
        return parse_enum_declaration(parser);
    }

    // Function declarations: allow either 'async blast' or plain 'blast'.
    if (check(parser, TOKEN_ASYNC) || check(parser, TOKEN_BLAST)) {
        bool is_async = false;
        if (match(parser, TOKEN_ASYNC)) {
            is_async = true;
            consume(parser, TOKEN_BLAST, "Expected 'blast' after 'async'");
        }
        else {
            consume(parser, TOKEN_BLAST, "Expected 'blast' for function");
        }
        return parse_function(parser, is_async);
    }
    
    // Statements (including variable decls inside blocks, but here we are at top level?)
    // Actually RADS allows top level vars? Maybe.
    // For blocks, we use parse_statement.
    
    // Let's handle statements
    return parse_statement(parser);
}

// Parse statement - Public for REPL
ASTNode* parse_statement(Parser* parser) {
    if (match(parser, TOKEN_ECHO)) {
        return parse_echo_statement(parser);
    }
    
    if (match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }

    if (match(parser, TOKEN_AWAIT)) {
        return parse_await_statement(parser);
    }

    // Allow empty statements to recover from stray semicolons during panic-mode.
    if (match(parser, TOKEN_SEMICOLON)) {
        return NULL;
    }
    
    if (match(parser, TOKEN_LOOP)) {
        return parse_loop_statement(parser);
    }
    
    if (match(parser, TOKEN_CRUISE)) {
        return parse_cruise_statement(parser);
    }
    
    if (match(parser, TOKEN_BREAK)) {
        return parse_break_or_continue(parser, true);
    }
    
    if (match(parser, TOKEN_CONTINUE)) {
        return parse_break_or_continue(parser, false);
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

        // Handle elif chain - build nested if statements
        while (match(parser, TOKEN_ELIF)) {
            int elif_line = parser->previous.line;
            int elif_column = parser->previous.column;
            consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'elif'");
            ASTNode* elif_condition = parse_expression(parser);
            consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after elif condition");
            ASTNode* elif_then = parse_statement(parser);

            // Create nested if for this elif
            ASTNode* nested_if = ast_create_if(elif_condition, elif_then, NULL, elif_line, elif_column);

            if (else_branch == NULL) {
                // First elif becomes the else branch
                else_branch = nested_if;
            } else {
                // Find the deepest if node and attach this elif to its else branch
                ASTNode* current = else_branch;
                while (current->type == AST_IF_STMT && current->if_stmt.else_branch != NULL) {
                    current = current->if_stmt.else_branch;
                }
                current->if_stmt.else_branch = nested_if;
            }
        }

        // Handle final else
        if (match(parser, TOKEN_ELSE)) {
            ASTNode* final_else = parse_statement(parser);
            if (else_branch == NULL) {
                else_branch = final_else;
            } else {
                // Attach to the last elif's else branch
                ASTNode* current = else_branch;
                while (current->type == AST_IF_STMT && current->if_stmt.else_branch != NULL) {
                    current = current->if_stmt.else_branch;
                }
                current->if_stmt.else_branch = final_else;
            }
        }

        return ast_create_if(condition, then_branch, else_branch, line, column);
    }
    
    if (match(parser, TOKEN_TRY)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        
        consume(parser, TOKEN_LEFT_BRACE, "Expected '{' after 'try'");
        ASTNode* try_block = parse_block(parser);
        
        char* catch_var = NULL;
        ASTNode* catch_block = NULL;
        if (match(parser, TOKEN_CATCH)) {
            consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'catch'");
            consume(parser, TOKEN_IDENTIFIER, "Expected variable name in catch");
            catch_var = malloc(parser->previous.length + 1);
            memcpy(catch_var, parser->previous.start, parser->previous.length);
            catch_var[parser->previous.length] = '\0';
            consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after catch variable");
            consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before catch body");
            catch_block = parse_block(parser);
        }
        
        ASTNode* finally_block = NULL;
        if (match(parser, TOKEN_FINALLY)) {
            consume(parser, TOKEN_LEFT_BRACE, "Expected '{' after 'finally'");
            finally_block = parse_block(parser);
        }
        
        return ast_create_try(try_block, catch_var, catch_block, finally_block, line, column);
    }
    
    if (match(parser, TOKEN_THROW)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        ASTNode* expr = parse_expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after throw expression");
        return ast_create_throw(expr, line, column);
    }
    
    // Variable declaration checks
    if (match(parser, TOKEN_TURBO) || match(parser, TOKEN_STR) || match(parser, TOKEN_I32) || match(parser, TOKEN_BOOL)) {
        return parse_variable(parser);
    }

    // Nested function declarations: async blast or plain blast
    if (check(parser, TOKEN_ASYNC) || check(parser, TOKEN_BLAST)) {
        bool is_async = match(parser, TOKEN_ASYNC);
        if (is_async) {
            consume(parser, TOKEN_BLAST, "Expected 'blast' after 'async'");
        } else {
            consume(parser, TOKEN_BLAST, "Expected 'blast' for function");
        }
        return parse_function(parser, is_async);
    }

    // Expression statement
    ASTNode* expr = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
    return expr;
}

// Synchronize parser after error - skip to next statement boundary
static void synchronize(Parser* parser) {
    parser->panic_mode = false;

    while (parser->current.type != TOKEN_EOF) {
        // Semicolon marks end of statement
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        // Right brace at top level (end of block/struct) - advance past it
        if (parser->current.type == TOKEN_RIGHT_BRACE) {
            advance(parser);
            return;
        }

        // These tokens typically start new statements
        switch (parser->current.type) {
            case TOKEN_STRUCT:
            case TOKEN_ENUM:
            case TOKEN_BLAST:
            case TOKEN_ASYNC:
            case TOKEN_TURBO:
            case TOKEN_IF:
            case TOKEN_LOOP:
            case TOKEN_CRUISE:
            case TOKEN_RETURN:
            case TOKEN_ECHO:
            case TOKEN_IMPORT:
                return;
            default:
                break;
        }

        advance(parser);
    }
}

// Initialize parser
void parser_init(Parser* parser, Lexer* lexer) {
    // Defensive zeroing keeps current/previous deterministic before first advance.
    memset(parser, 0, sizeof(Parser));
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->current.type = TOKEN_ERROR;
    parser->previous.type = TOKEN_ERROR;
    advance(parser);
}

// Main parse function
ASTNode* parser_parse(Parser* parser) {
    ASTList* declarations = ast_list_create();

    while (!match(parser, TOKEN_EOF)) {
        ASTNode* decl = parse_declaration(parser);
        if (decl) {
            ast_list_append(declarations, decl);
        } else if (parser->panic_mode) {
            // Parser is in panic mode - synchronize to recover
            synchronize(parser);
        }
    }

    if (parser->had_error) {
        ast_list_free(declarations);
        return NULL;
    }

    return ast_create_program(declarations);
}
