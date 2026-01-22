/**
 * RADS Masquerading - Python Lexer
 *
 * Tokenizes Python source code for conversion to RADS.
 *
 * Version: 0.0.5 "CHAMELEON"
 */

#ifndef PYTHON_LEXER_H
#define PYTHON_LEXER_H

#include <stdio.h>

// ============================================================================
// Token Types
// ============================================================================

typedef enum {
    // Literals
    PY_TOKEN_INT,
    PY_TOKEN_FLOAT,
    PY_TOKEN_STRING,
    PY_TOKEN_TRUE,
    PY_TOKEN_FALSE,
    PY_TOKEN_NONE,

    // Identifiers and Keywords
    PY_TOKEN_IDENTIFIER,
    PY_TOKEN_DEF,
    PY_TOKEN_CLASS,
    PY_TOKEN_IF,
    PY_TOKEN_ELIF,
    PY_TOKEN_ELSE,
    PY_TOKEN_WHILE,
    PY_TOKEN_FOR,
    PY_TOKEN_IN,
    PY_TOKEN_BREAK,
    PY_TOKEN_CONTINUE,
    PY_TOKEN_RETURN,
    PY_TOKEN_IMPORT,
    PY_TOKEN_FROM,
    PY_TOKEN_AS,
    PY_TOKEN_PASS,

    // Operators
    PY_TOKEN_PLUS,
    PY_TOKEN_MINUS,
    PY_TOKEN_STAR,
    PY_TOKEN_SLASH,
    PY_TOKEN_PERCENT,
    PY_TOKEN_POWER,
    PY_TOKEN_EQ,
    PY_TOKEN_NE,
    PY_TOKEN_LT,
    PY_TOKEN_LE,
    PY_TOKEN_GT,
    PY_TOKEN_GE,
    PY_TOKEN_AND,
    PY_TOKEN_OR,
    PY_TOKEN_NOT,
    PY_TOKEN_ASSIGN,

    // Delimiters
    PY_TOKEN_LPAREN,
    PY_TOKEN_RPAREN,
    PY_TOKEN_LBRACKET,
    PY_TOKEN_RBRACKET,
    PY_TOKEN_LBRACE,
    PY_TOKEN_RBRACE,
    PY_TOKEN_COMMA,
    PY_TOKEN_COLON,
    PY_TOKEN_DOT,
    PY_TOKEN_ARROW,

    // Special
    PY_TOKEN_NEWLINE,
    PY_TOKEN_INDENT,
    PY_TOKEN_DEDENT,
    PY_TOKEN_EOF,
    PY_TOKEN_ERROR

} PyTokenType;

// ============================================================================
// Token Structure
// ============================================================================

typedef struct {
    PyTokenType type;
    char* value;
    int line;
    int column;
} PyToken;

// ============================================================================
// Lexer Structure
// ============================================================================

typedef struct {
    const char* source;
    size_t position;
    size_t length;
    int line;
    int column;
    int indent_level;
    int pending_dedents;
} PyLexer;

// ============================================================================
// API Functions
// ============================================================================

/**
 * Initialize a Python lexer with source code
 */
PyLexer* py_lexer_init(const char* source);

/**
 * Get the next token from the source
 */
PyToken* py_lexer_next_token(PyLexer* lexer);

/**
 * Peek at the next token without consuming it
 */
PyToken* py_lexer_peek_token(PyLexer* lexer);

/**
 * Free a token
 */
void py_token_free(PyToken* token);

/**
 * Free the lexer
 */
void py_lexer_free(PyLexer* lexer);

/**
 * Get token type name for debugging
 */
const char* py_token_type_name(PyTokenType type);

#endif // PYTHON_LEXER_H
