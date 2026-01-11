#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// Initialize lexer
void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
}

// Helper functions
static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    lexer->column++;
    return true;
}

/* make_token
 * Why: Centralizes token construction and guards column metadata from underflow so
 *      parser diagnostics stay human-friendly and consistent.
 * How to upgrade: If we add UTF-8 column tracking, adjust the column math here to
 *      account for rune width. Keep the clamp to avoid negative/overflow columns.
 */
static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (size_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    int col = lexer->column - (int)token.length;
    token.column = col > 0 ? col : 1;
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer->column = 0;
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    // Line comment
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    // Block comment
                    advance(lexer); // /
                    advance(lexer); // *
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer); // *
                            advance(lexer); // /
                            break;
                        }
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 0;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType check_keyword(Lexer* lexer, int start, int length, const char* rest, TokenType type) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer* lexer) {
    switch (lexer->start[0]) {
        case 'a':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'l': return check_keyword(lexer, 2, 3, "loc", TOKEN_ALLOC);
                    case 'r': return check_keyword(lexer, 2, 3, "ray", TOKEN_ARRAY);
                    case 's': return check_keyword(lexer, 2, 3, "ync", TOKEN_ASYNC);
                    case 'w': return check_keyword(lexer, 2, 3, "ait", TOKEN_AWAIT);
                }
            }
            break;
        case 'b':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'l': return check_keyword(lexer, 2, 3, "ast", TOKEN_BLAST);
                    case 'o': return check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
                    case 'r': return check_keyword(lexer, 2, 3, "eak", TOKEN_BREAK);
                }
            }
            break;
        case 'c':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'a': return check_keyword(lexer, 2, 2, "se", TOKEN_CASE);
                    case 'h': return check_keyword(lexer, 2, 2, "ar", TOKEN_CHAR_TYPE);
                    case 'o': return check_keyword(lexer, 2, 6, "ntinue", TOKEN_CONTINUE);
                    case 'r': return check_keyword(lexer, 2, 4, "uise", TOKEN_CRUISE);
                }
            }
            break;
        case 'd':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'e': return check_keyword(lexer, 2, 5, "fault", TOKEN_DEFAULT);
                    case 'y': return check_keyword(lexer, 2, 6, "narray", TOKEN_DYNARRAY);
                }
            }
            break;
        case 'e':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'c': return check_keyword(lexer, 2, 2, "ho", TOKEN_ECHO);
                    case 'l':
                        if (lexer->current - lexer->start == 4) {
                            // Both "elif" and "else" are 4 chars - check which one
                            if (lexer->start[2] == 'i') {
                                return check_keyword(lexer, 2, 2, "if", TOKEN_ELIF);
                            } else {
                                return check_keyword(lexer, 2, 2, "se", TOKEN_ELSE);
                            }
                        } else {
                            return check_keyword(lexer, 2, 2, "se", TOKEN_ELSE);
                        }
                    case 'n': return check_keyword(lexer, 2, 2, "um", TOKEN_ENUM);
                    case 'x': return check_keyword(lexer, 2, 4, "port", TOKEN_EXPORT);
                }
            }
            break;
        case 'f':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case '3': return check_keyword(lexer, 2, 1, "2", TOKEN_F32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOKEN_F64);
                    case 'a': return check_keyword(lexer, 2, 3, "lse", TOKEN_FALSE);
                    case 'r': return check_keyword(lexer, 2, 2, "ee", TOKEN_FREE);
                }
            }
            break;
        case 'i':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case '3': return check_keyword(lexer, 2, 1, "2", TOKEN_I32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOKEN_I64);
                    case 'f': return check_keyword(lexer, 2, 0, "", TOKEN_IF);
                    case 'm': return check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
                    case 'n': return check_keyword(lexer, 2, 0, "", TOKEN_IN);
                }
            }
            break;
        case 'l': return check_keyword(lexer, 1, 3, "oop", TOKEN_LOOP);
        case 'm': return check_keyword(lexer, 1, 3, "ove", TOKEN_MOVE);
        case 'n': return check_keyword(lexer, 1, 3, "ull", TOKEN_NULL);
        case 'p':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'l': return check_keyword(lexer, 2, 4, "ugin", TOKEN_PLUGIN);
                    case 't': return check_keyword(lexer, 2, 1, "r", TOKEN_PTR);
                }
            }
            break;
        case 'r': return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
        case 's':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'p': return check_keyword(lexer, 2, 3, "awn", TOKEN_SPAWN);
                    case 't':
                        if (lexer->current - lexer->start > 2) {
                            switch (lexer->start[2]) {
                                case 'r':
                                    if (lexer->current - lexer->start == 3) return TOKEN_STR;
                                    if (lexer->current - lexer->start > 3) {
                                        if (lexer->start[3] == 'e') return check_keyword(lexer, 4, 2, "am", TOKEN_STREAM);
                                        if (lexer->start[3] == 'u') return check_keyword(lexer, 4, 2, "ct", TOKEN_STRUCT);
                                    }
                                    break;
                            }
                        }
                        break;
                    case 'w': return check_keyword(lexer, 2, 4, "itch", TOKEN_SWITCH);
                }
            }
            break;
        case 't':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'r': return check_keyword(lexer, 2, 2, "ue", TOKEN_TRUE);
                    case 'u': return check_keyword(lexer, 2, 3, "rbo", TOKEN_TURBO);
                }
            }
            break;
        case 'x': return check_keyword(lexer, 1, 5, "treme", TOKEN_XTREME);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    return make_token(lexer, identifier_type(lexer));
}

static Token number(Lexer* lexer) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    // Look for decimal point
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer); // consume '.'
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
        return make_token(lexer, TOKEN_FLOAT);
    }
    
    return make_token(lexer, TOKEN_INTEGER);
}

static Token string(Lexer* lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        if (peek(lexer) == '\\') {
            advance(lexer); // escape character
            if (!is_at_end(lexer)) advance(lexer);
        } else {
            advance(lexer);
        }
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }
    
    advance(lexer); // closing "
    return make_token(lexer, TOKEN_STRING);
}

static Token char_literal(Lexer* lexer) {
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated character literal");
    }
    
    if (peek(lexer) == '\\') {
        advance(lexer); // escape
        if (!is_at_end(lexer)) advance(lexer);
    } else {
        advance(lexer);
    }
    
    if (peek(lexer) != '\'') {
        return error_token(lexer, "Unterminated character literal");
    }
    
    advance(lexer); // closing '
    return make_token(lexer, TOKEN_CHAR);
}

// Main tokenization function
Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance(lexer);
    
    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        return identifier(lexer);
    }
    
    // Numbers
    if (isdigit(c)) {
        return number(lexer);
    }
    
    switch (c) {
        // Single character tokens
        case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
        case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
        case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
        case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
        case '[': return make_token(lexer, TOKEN_LEFT_BRACKET);
        case ']': return make_token(lexer, TOKEN_RIGHT_BRACKET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '%': return make_token(lexer, TOKEN_PERCENT);
        case '~': return make_token(lexer, TOKEN_TILDE);
        case '^': return make_token(lexer, TOKEN_CARET);
        
        // Two character tokens
        case '-':
            return make_token(lexer, match(lexer, '>') ? TOKEN_ARROW : TOKEN_MINUS);
        case '!':
            return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_LESS_EQUAL);
            if (match(lexer, '<')) return make_token(lexer, TOKEN_LEFT_SHIFT);
            return make_token(lexer, TOKEN_LESS);
        case '>':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_GREATER_EQUAL);
            if (match(lexer, '>')) return make_token(lexer, TOKEN_RIGHT_SHIFT);
            return make_token(lexer, TOKEN_GREATER);
        case '&':
            return make_token(lexer, match(lexer, '&') ? TOKEN_AND_AND : TOKEN_AMPERSAND);
        case '|':
            return make_token(lexer, match(lexer, '|') ? TOKEN_OR_OR : TOKEN_PIPE);
        case '.':
            return make_token(lexer, match(lexer, '.') ? TOKEN_DOT_DOT : TOKEN_DOT);
        case ':':
            return make_token(lexer, match(lexer, ':') ? TOKEN_DOUBLE_COLON : TOKEN_COLON);
        case '/':
            return make_token(lexer, TOKEN_SLASH);
        
        // Literals
        case '"': return string(lexer);
        case '\'': return char_literal(lexer);
    }
    
    return error_token(lexer, "Unexpected character");
}

/* token_type_to_string
 * Why: Keeps lexer/debug output readable so parser and language errors remain
 *      conversational per the "Mechanic" rule.
 * How to upgrade: When adding new tokens, add their string names here; consider
 *      generating this from the enum to avoid drift.
 */
const char* token_type_to_string(TokenType type) {
    switch (type) {
        // Literals
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_NULL: return "NULL";

        // Identifiers
        case TOKEN_IDENTIFIER: return "IDENTIFIER";

        // Keywords
        case TOKEN_BLAST: return "BLAST";
        case TOKEN_TURBO: return "TURBO";
        case TOKEN_STR: return "STR";
        case TOKEN_ARRAY: return "ARRAY";
        case TOKEN_DYNARRAY: return "DYNARRAY";
        case TOKEN_STRUCT: return "STRUCT";
        case TOKEN_ENUM: return "ENUM";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_ELIF: return "ELIF";
        case TOKEN_LOOP: return "LOOP";
        case TOKEN_CRUISE: return "CRUISE";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_IN: return "IN";
        case TOKEN_SWITCH: return "SWITCH";
        case TOKEN_CASE: return "CASE";
        case TOKEN_DEFAULT: return "DEFAULT";
        case TOKEN_ASYNC: return "ASYNC";
        case TOKEN_AWAIT: return "AWAIT";
        case TOKEN_SPAWN: return "SPAWN";
        case TOKEN_STREAM: return "STREAM";
        case TOKEN_IMPORT: return "IMPORT";
        case TOKEN_EXPORT: return "EXPORT";
        case TOKEN_PLUGIN: return "PLUGIN";
        case TOKEN_ECHO: return "ECHO";
        case TOKEN_XTREME: return "XTREME";
        case TOKEN_ALLOC: return "ALLOC";
        case TOKEN_FREE: return "FREE";
        case TOKEN_MOVE: return "MOVE";

        // Types
        case TOKEN_I32: return "I32";
        case TOKEN_I64: return "I64";
        case TOKEN_F32: return "F32";
        case TOKEN_F64: return "F64";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_CHAR_TYPE: return "CHAR_TYPE";
        case TOKEN_PTR: return "PTR";

        // Operators
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TOKEN_BANG: return "BANG";
        case TOKEN_BANG_EQUAL: return "BANG_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_AND_AND: return "AND_AND";
        case TOKEN_OR_OR: return "OR_OR";
        case TOKEN_AMPERSAND: return "AMPERSAND";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_CARET: return "CARET";
        case TOKEN_TILDE: return "TILDE";
        case TOKEN_LEFT_SHIFT: return "LEFT_SHIFT";
        case TOKEN_RIGHT_SHIFT: return "RIGHT_SHIFT";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_DOT: return "DOT";
        case TOKEN_DOT_DOT: return "DOT_DOT";
        case TOKEN_COLON: return "COLON";
        case TOKEN_DOUBLE_COLON: return "DOUBLE_COLON";

        // Delimiters
        case TOKEN_LEFT_PAREN: return "LEFT_PAREN";
        case TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
        case TOKEN_LEFT_BRACE: return "LEFT_BRACE";
        case TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
        case TOKEN_LEFT_BRACKET: return "LEFT_BRACKET";
        case TOKEN_RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";

        // Special
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void token_print(Token* token) {
    printf("[%s] '%.*s' (line %d, col %d)\n",
           token_type_to_string(token->type),
           (int)token->length,
           token->start,
           token->line,
           token->column);
}
