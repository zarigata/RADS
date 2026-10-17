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

// Non-destructive two-token lookahead past `current`: snapshot the lexer
// (POD: source/start/current/line/column), scan two tokens, restore. Used to
// disambiguate `Name {` struct literals from a statement block (run 19): the
// struct-literal parse is only entered when the tokens after `{` match the
// field shape `identifier ':'` or immediately close the literal (`}`).
// One token was not enough: `items { n = 1 }` also starts with an identifier.
static void peek2_next_types(Parser* parser, TokenType* t1, TokenType* t2) {
    Lexer save = *parser->lexer;
    Token a = lexer_next_token(parser->lexer);
    Token b = lexer_next_token(parser->lexer);
    *parser->lexer = save;
    *t1 = a.type;
    *t2 = b.type;
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

// Consume the terminator of a statement: ';', or — pythonic style — a
// newline, '}' or EOF when ';' is absent. Tokens carry line numbers, so a
// line break between the last consumed token and the current token ends
// the statement with zero lexer changes. Note the expression parser is
// greedy across newlines (like ASI): "let x = 1\n-5" parses as 1-5.
static void consume_statement_end(Parser* parser, const char* message) {
    if (parser->current.type == TOKEN_SEMICOLON) {
        advance(parser);
        return;
    }
    if (parser->current.type == TOKEN_EOF ||
        parser->current.type == TOKEN_RIGHT_BRACE ||
        parser->current.line > parser->previous.line) {
        return;  // implicit terminator
    }
    error_at_current(parser, message);
}

// Forward declarations
static ASTNode* parse_expression(Parser* parser);
ASTNode* parse_statement(Parser* parser);  // Public for REPL
static ASTNode* parse_declaration(Parser* parser);
static ASTNode* parse_comparison(Parser* parser);
static ASTNode* parse_block(Parser* parser);

// Run 27: REPL entry point. The REPL previously called parse_statement
// directly, which has no struct/enum route — a `struct Pt {...}` line died
// with "Expected ';' after expression". parse_declaration handles all
// top-level forms (import/struct/enum/async/blast) and falls through to
// parse_statement for everything else, exactly the REPL contract.

// Decode C-style escapes in a string literal body (between the quotes).
// The RADS lexer keeps escapes raw ("a\nb" lexes as 4 chars a,\,n,b) — this
// turns them into real control characters at parse time, like C/Python/JS.
// Writes the decoded bytes in-place conceptually; returns a malloc'd string.
static char* decode_string_escapes(const char* src, size_t len) {
    char* out = malloc(len + 1);
    if (!out) return NULL;
    size_t o = 0;
    for (size_t i = 0; i < len; i++) {
        char c = src[i];
        if (c == '\\' && i + 1 < len) {
            char e = src[++i];
            switch (e) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '0': c = '\0'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '\'': c = '\''; break;
                case 'e': c = '\033'; break;  // RADS ESC, for ANSI in echo
                default:  c = e;   break;     // unknown: keep the char itself
            }
        }
        out[o++] = c;
    }
    out[o] = '\0';
    return out;
}

// Parse primary expressions
// Run 24: the clause tail of a comprehension —
//   ('for'/'cruise') IDENT 'in' expr ('if' expr)* , repeated for nesting:
//   [y for x in xss for y in x if y > 2]
// 'for' arrives as TOKEN_CRUISE (pythonic lexer alias), 'if' as TOKEN_IF.
// No ternary exists in RADS, so an 'if' in clause position can only be a
// filter clause — expressions never continue with TOKEN_IF/TOKEN_CRUISE.
static ASTList* parse_comprehension_clauses(Parser* parser) {
    ASTList* clauses = ast_list_create();
    while (check(parser, TOKEN_CRUISE) || check(parser, TOKEN_IF)) {
        if (match(parser, TOKEN_CRUISE)) {
            int line = parser->previous.line;
            int column = parser->previous.column;
            consume(parser, TOKEN_IDENTIFIER, "Expected loop variable after 'for' in comprehension");
            char* var = malloc(parser->previous.length + 1);
            if (!var) { ast_list_free(clauses); return NULL; }
            memcpy(var, parser->previous.start, parser->previous.length);
            var[parser->previous.length] = '\0';
            consume(parser, TOKEN_IN, "Expected 'in' after comprehension loop variable");
            ASTNode* iterable = parse_expression(parser);
            if (!iterable) { free(var); ast_list_free(clauses); return NULL; }
            ASTNode* cf = ast_create_comp_for(var, iterable, line, column);
            free(var);
            if (!cf) { ast_list_free(clauses); return NULL; }
            ast_list_append(clauses, cf);
        } else {
            match(parser, TOKEN_IF);
            int line = parser->previous.line;
            int column = parser->previous.column;
            ASTNode* cond = parse_expression(parser);
            if (!cond) { ast_list_free(clauses); return NULL; }
            ASTNode* ci = ast_create_comp_if(cond, line, column);
            if (!ci) { ast_list_free(clauses); return NULL; }
            ast_list_append(clauses, ci);
        }
    }
    return clauses;
}

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
        // Strip quotes and decode escapes (\n, \t, \" ...) into real bytes
        size_t len = parser->previous.length - 2;
        char* str = decode_string_escapes(parser->previous.start + 1, len);
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

    // Module names that are also type keywords (array, str, struct) — usable
    // as module identifiers for dotted native calls (struct.calcsize, etc.)
    if (match(parser, TOKEN_ARRAY) || match(parser, TOKEN_STR) || match(parser, TOKEN_STRUCT)) {
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
            // Run 24: list comprehension [e for x in xs if c]. The element
            // expression is parsed first; a following 'for'/'cruise' turns
            // the whole thing into a comprehension node instead of a literal.
            // ('for' lexes as TOKEN_CRUISE via the pythonic alias — an
            // expression can never continue with it, so one token of
            // lookahead after the first element is unambiguous.)
            bool first = true;
            do {
                if (match(parser, TOKEN_DOT_DOT_DOT)) {
                    int spread_line = parser->previous.line;
                    int spread_col = parser->previous.column;
                    ASTNode* expr = parse_expression(parser);
                    ASTNode* spread = ast_create_spread(expr, spread_line, spread_col);
                    if (spread) ast_list_append(elements, spread);
                    first = false;
                } else {
                    ASTNode* elem = parse_expression(parser);
                    if (!elem) return NULL;
                    if (first && check(parser, TOKEN_CRUISE)) {
                        // [expr for ...] — comprehension, not a literal.
                        ASTList* clauses = parse_comprehension_clauses(parser);
                        ast_list_free(elements); // empty — the expr moved out
                        if (!clauses) { ast_free(elem); return NULL; }
                        consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after comprehension");
                        return ast_create_comprehension(elem, NULL, clauses, false, line, column);
                    }
                    ast_list_append(elements, elem);
                    first = false;
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

    // Anonymous map literal: {"key": value, other: value} — desugared to a
    // flat [k, v, k, v, ...] array literal, the same object representation
    // json.parse produces, so every json.* helper accepts it directly.
    // (A '{' reaching parse_primary is never a block: statement-level '{'
    // is consumed by parse_statement before any expression is parsed.)
    if (match(parser, TOKEN_LEFT_BRACE)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        ASTList* elements = ast_list_create();
        if (!check(parser, TOKEN_RIGHT_BRACE)) {
            do {
                // Run 24: keys parse as full expressions now, so computed
                // keys {str(i) + "k": v} and comprehensions
                // {k_expr: v_expr for x in xs} share one path. The two
                // legacy shapes — string keys and bare-identifier keys
                // (evaluated to their NAME, matching pre-run-24 literals)
                // — are recognized by 2-token lookahead before committing.
                ASTNode* key_node = NULL;
                bool key_was_ident = false;
                const char* key_ident_name = NULL;
                TokenType t1, t2;
                peek2_next_types(parser, &t1, &t2);
                // NOTE: peek2_next_types returns the two tokens AFTER
                // parser->current — pair it with check() for the current
                // token. Legacy string key: STRING ':' …
                if (check(parser, TOKEN_STRING) && t1 == TOKEN_COLON) {
                    match(parser, TOKEN_STRING);
                    size_t len = parser->previous.length - 2;
                    char* key = decode_string_escapes(parser->previous.start + 1, len);
                    key_node = ast_create_string(key, parser->previous.line, parser->previous.column);
                    free(key);
                } else if (check(parser, TOKEN_IDENTIFIER) && t1 == TOKEN_COLON) {
                    match(parser, TOKEN_IDENTIFIER);
                    char* key = malloc(parser->previous.length + 1);
                    memcpy(key, parser->previous.start, parser->previous.length);
                    key[parser->previous.length] = '\0';
                    key_node = ast_create_string(key, parser->previous.line, parser->previous.column);
                    // If this turns out to be a comprehension ({w: w.len for
                    // w in words}), the key must evaluate the VARIABLE w per
                    // iteration, not the literal name. Deferred decision —
                    // the 'for' only appears after the value expression, so
                    // ownership of `key` stays here until the branch resolves.
                    key_was_ident = true;
                    key_ident_name = key;
                } else {
                    key_node = parse_expression(parser);
                }
                if (!key_node) { if (key_was_ident) free((void*)key_ident_name); return NULL; }
                consume(parser, TOKEN_COLON, "Expected ':' after map key");
                ASTNode* val = parse_expression(parser);
                if (!val) {
                    ast_free(key_node);
                    if (key_was_ident) free((void*)key_ident_name);
                    return NULL;
                }
                if (elements->count == 0 && check(parser, TOKEN_CRUISE)) {
                    // {k: v for ...} — dict comprehension.
                    if (key_was_ident) {
                        // re-key: bare identifier means the loop variable's
                        // VALUE, matching Python ({w: ... for w in ...}).
                        ast_free(key_node);
                        key_node = ast_create_identifier(key_ident_name, line, column);
                        free((void*)key_ident_name);
                        key_was_ident = false;
                    }
                    ASTList* clauses = parse_comprehension_clauses(parser);
                    ast_list_free(elements); // empty — k/v moved out
                    if (!clauses) { ast_free(key_node); ast_free(val); return NULL; }
                    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after dict comprehension");
                    return ast_create_comprehension(val, key_node, clauses, true, line, column);
                }
                ast_list_append(elements, key_node);
                ast_list_append(elements, val);
                if (key_was_ident) free((void*)key_ident_name);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after map literal");
        return ast_create_dict_literal(elements, line, column);
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
    if (!expr) return NULL; /* propagate parse error instead of dereferencing NULL */
    
    while (true) {
        if (check(parser, TOKEN_LEFT_BRACE) && expr->type == AST_IDENTIFIER
            && !parser->no_struct_literal) {
            // `Name {` is a struct literal ONLY when the tokens after `{`
            // match the field shape `identifier ':'` or close the literal
            // (`}`). Otherwise it is a statement block — e.g.
            // `cruise i in items { n = n + 1; }` (run 19). One token of
            // lookahead was not enough: `items { n = 1 }` also starts with
            // an identifier, so we must also see the colon.
            TokenType after_brace, after_field;
            peek2_next_types(parser, &after_brace, &after_field);
            if ((after_brace != TOKEN_IDENTIFIER || after_field != TOKEN_COLON)
                && after_brace != TOKEN_RIGHT_BRACE) break;
            match(parser, TOKEN_LEFT_BRACE); // consume the brace we checked for
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

            // Run 28: the identifier node's work is done (its name was copied
            // into the literal by the strdup above). Free it — the old code
            // just overwrote `expr`, orphaning one AST node per literal site,
            // every parse, forever. NOTE: create BEFORE freeing — struct_name
            // aliases expr->identifier.name, which ast_free releases.
            ASTNode* lit = ast_create_struct_literal(struct_name, fields, line, column);
            ast_free(expr);
            expr = lit;

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

    // Run 33: parse_range can return NULL in panic mode (e.g. leftover '='
    // after a failed enum body). The old code dereferenced expr->type right
    // here and segfaulted (rc=139) after printing the parse error.
    if (!expr) return NULL;

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

// Parse echo statement: echo(expr); or pythonic echo expr
static ASTNode* parse_echo_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    
    bool has_paren = match(parser, TOKEN_LEFT_PAREN);
    ASTNode* expr = parse_expression(parser);
    if (has_paren) {
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
    }
    consume_statement_end(parser, "Expected ';' after statement");
    
    return ast_create_echo(expr, line, column);
}

// Parse return statement
static ASTNode* parse_return_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    ASTNode* value = NULL;
    if (!check(parser, TOKEN_SEMICOLON) &&
        !check(parser, TOKEN_RIGHT_BRACE) &&
        !check(parser, TOKEN_EOF) &&
        parser->current.line <= parser->previous.line) {
        value = parse_expression(parser);
    }
    consume_statement_end(parser, "Expected ';' after return value");
    return ast_create_return(value, line, column);
}

// Parse await statement: await <expr>;
static ASTNode* parse_await_statement(Parser* parser) {
    (void)parser->previous; // unused for now
    ASTNode* value = parse_expression(parser);
    consume_statement_end(parser, "Expected ';' after await expression");
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
    consume_statement_end(parser, "Expected ';' after statement");
    return is_break ? ast_create_break(line, column) : ast_create_continue(line, column);
}

// Parse cruise (for-range or C-style for) statement
// cruise (id in iterable) <body> - for-range
// cruise (init; condition; update) <body> - C-style for
static ASTNode* parse_cruise_statement(Parser* parser) {
    int line = parser->previous.line;
    int column = parser->previous.column;
    // pythonic: parens optional — `for i in range(...)` or `cruise (i in 0..3)`
    bool has_paren = match(parser, TOKEN_LEFT_PAREN);
    
    // Check if this is for-range (id in iterable) or C-style for (init; cond; update)
    // Peek at what follows the identifier
    consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'cruise'");
    char* var_name = malloc(parser->previous.length + 1);
    memcpy(var_name, parser->previous.start, parser->previous.length);
    var_name[parser->previous.length] = '\0';
    
    if (match(parser, TOKEN_IN)) {
        // For-range: cruise (id in iterable)
        // The `{` that may follow the iterable is ALWAYS the loop body —
        // suppress struct-literal parsing for this one expression.
        parser->no_struct_literal = true;
        ASTNode* iterable = parse_expression(parser);
        parser->no_struct_literal = false;
        if (!iterable) return NULL; /* propagate parse error */
        if (has_paren) {
            consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after cruise iterable");
        }
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
        else if (parser->panic_mode) {
            /* Error recovery: skip to a statement boundary (like synchronize,
             * but stop at the '}' that ends this block). Without this, a NULL
             * statement that consumed no token loops forever. */
            parser->panic_mode = false;
            while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
                if (parser->previous.type == TOKEN_SEMICOLON) break;
                switch (parser->current.type) {
                    case TOKEN_STRUCT: case TOKEN_ENUM: case TOKEN_BLAST:
                    case TOKEN_ASYNC: case TOKEN_TURBO: case TOKEN_IF:
                    case TOKEN_LOOP: case TOKEN_CRUISE: case TOKEN_RETURN:
                    case TOKEN_ECHO: case TOKEN_IMPORT:
                        goto done_recover;
                    default:
                        advance(parser);
                }
            }
            done_recover:;
        }
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
    
    consume_statement_end(parser, "Expected ';' after variable declaration");
    
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
    size_t value_count = 0;
    long long auto_next = 0;      // auto-counter for bare members
    bool has_explicit = false;    // any `= value` in this body?
    // Grown on first explicit value only; NULL = pure auto-numbering.
    long long* int_values = NULL;
    size_t int_cap = 0;
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        // Parse enum value name
        consume(parser, TOKEN_IDENTIFIER, "Expected enum value name");
        if (parser->previous.type != TOKEN_IDENTIFIER) break; // error path: bail out
        char* value_name = malloc(parser->previous.length + 1);
        memcpy(value_name, parser->previous.start, parser->previous.length);
        value_name[parser->previous.length] = '\0';

        ASTNode* value = ast_create_identifier(value_name, parser->previous.line, parser->previous.column);
        ast_list_append(values, value);
        free(value_name);

        // Run 33: optional explicit value — `RED = 1` / `BLUE = RED + 3` /
        // `COLD = -1`. Constant-fold: identifier refs resolve against members
        // already defined in THIS enum; otherwise fall back to parse_integer
        // so `RED + 3` still errors cleanly instead of segfaulting.
        long long eff = auto_next;
        if (match(parser, TOKEN_EQUAL)) {
            has_explicit = true;
            bool neg = match(parser, TOKEN_MINUS);
            if (match(parser, TOKEN_INTEGER)) {
                char* end;
                long long iv = strtoll(parser->previous.start, &end, 10);
                eff = neg ? -iv : iv;
            } else if (match(parser, TOKEN_IDENTIFIER)) {
                // Reference to an earlier member of the same enum.
                char* ref = malloc(parser->previous.length + 1);
                memcpy(ref, parser->previous.start, parser->previous.length);
                ref[parser->previous.length] = '\0';
                long long ref_val = 0;
                bool found = false;
                for (size_t i = 0; i < value_count; i++) {
                    if (strcmp(values->nodes[i]->identifier.name, ref) == 0) {
                        ref_val = int_values ? int_values[i] : (long long)i;
                        found = true;
                        break;
                    }
                }
                free(ref);
                if (!found) {
                    error_at(parser, &parser->previous, "Unknown enum member in explicit value");
                    break;
                }
                // optional "+ N" / "- N" offset
                if (match(parser, TOKEN_PLUS)) {
                    if (match(parser, TOKEN_INTEGER)) {
                        char* end;
                        long long off = strtoll(parser->previous.start, &end, 10);
                        ref_val += off;
                    }
                } else if (match(parser, TOKEN_MINUS)) {
                    if (match(parser, TOKEN_INTEGER)) {
                        char* end;
                        long long off = strtoll(parser->previous.start, &end, 10);
                        ref_val -= off;
                    }
                }
                eff = ref_val;
            } else {
                error_at(parser, &parser->previous, "Expected integer or member name after '=' in enum");
                break;
            }
        }
        if (has_explicit || int_values) {
            if (!int_values) {
                // First explicit value: materialize the parallel array,
                // backfilling auto values for any earlier bare members.
                int_cap = value_count + 8;
                int_values = calloc(int_cap, sizeof(long long));
                if (!int_values) { error_at_current(parser, "Out of memory"); break; }
                for (size_t i = 0; i < value_count; i++) {
                    int_values[i] = (long long)i; // bare prefix: index numbering
                }
            }
            if (value_count >= int_cap) {
                int_cap *= 2;
                long long* grown = realloc(int_values, int_cap * sizeof(long long));
                if (!grown) {
                    free(int_values);
                    int_values = NULL; // avoid dangling store into the node
                    error_at_current(parser, "Out of memory");
                    break;
                }
                int_values = grown;
            }
            int_values[value_count] = eff;
        }

        value_count++;
        auto_next = eff + 1; // next bare member continues from here

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

    ASTNode* enum_decl = ast_create_enum_decl(name, values, int_values, line, column);
    free(name);
    return enum_decl;
}

// Parse declaration
static ASTNode* parse_declaration(Parser* parser) {
    // Import statement: import "filename.rads"; or pythonic import bisect;
    if (match(parser, TOKEN_IMPORT)) {
        int line = parser->previous.line;
        int column = parser->previous.column;
        char* filename;
        if (match(parser, TOKEN_IDENTIFIER)) {
            // bare module name, e.g. `import bisect;`
            filename = strndup(parser->previous.start, parser->previous.length);
        } else {
            consume(parser, TOKEN_STRING, "Expected filename string after 'import'");
            filename = malloc(parser->previous.length - 1); // -2 for quotes, +1 for null
            memcpy(filename, parser->previous.start + 1, parser->previous.length - 2);
            filename[parser->previous.length - 2] = '\0';
        }
        consume_statement_end(parser, "Expected ';' after import");
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

// Parse declaration — public REPL entry (run 27): top-level forms that
// parse_statement cannot express (struct/enum/import/blast) parse here;
// everything else delegates down to parse_statement unchanged.
ASTNode* parse_repl_line(Parser* parser) {
    return parse_declaration(parser);
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
        consume_statement_end(parser, "Expected ';' after throw expression");
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
    consume_statement_end(parser, "Expected ';' after expression");
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

// True when the parser's current token is unchanged since `before` — used
// to detect a parse iteration that consumed no input (a guaranteed infinite
// loop otherwise: panic mode suppresses errors and nothing advances).
static bool parser_stalled(Parser* parser, Token before) {
    return parser->current.type == before.type
        && parser->current.start == before.start
        && parser->current.length == before.length;
}

// Main parse function
ASTNode* parser_parse(Parser* parser) {
    ASTList* declarations = ast_list_create();

    while (!match(parser, TOKEN_EOF)) {
        Token before = parser->current;
        ASTNode* decl = parse_declaration(parser);
        if (decl) {
            ast_list_append(declarations, decl);
        } else if (parser->panic_mode) {
            // Parser is in panic mode - synchronize to recover
            synchronize(parser);
        }
        // Forward-progress guard: never let a top-level iteration spin
        // without consuming a token (unbounded stderr = disk-fill risk).
        if (parser->current.type != TOKEN_EOF && parser_stalled(parser, before)) {
            advance(parser);
        }
    }

    if (parser->had_error) {
        ast_list_free(declarations);
        return NULL;
    }

    return ast_create_program(declarations);
}
