#ifndef RADS_LEXER_H
#define RADS_LEXER_H

#include <stddef.h>
#include <stdbool.h>

// Token types
typedef enum {
    // Literals
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    
    // Identifiers
    TOKEN_IDENTIFIER,
    
    // Keywords
    TOKEN_BLAST,        // function
    TOKEN_TURBO,        // optimized variable
    TOKEN_STR,          // string type
    TOKEN_ARRAY,        // array type
    TOKEN_DYNARRAY,     // dynamic array
    TOKEN_STRUCT,       // struct
    TOKEN_ENUM,         // enum
    TOKEN_IF,           // if
    TOKEN_ELSE,         // else
    TOKEN_ELIF,         // elif
    TOKEN_LOOP,         // while loop
    TOKEN_CRUISE,       // for loop
    TOKEN_BREAK,        // break
    TOKEN_CONTINUE,     // continue
    TOKEN_RETURN,       // return
    TOKEN_IN,           // in
    TOKEN_SWITCH,       // switch
    TOKEN_CASE,         // case
    TOKEN_DEFAULT,      // default
    TOKEN_ASYNC,        // async
    TOKEN_AWAIT,        // await
    TOKEN_SPAWN,        // spawn thread
    TOKEN_STREAM,       // stream type
    TOKEN_IMPORT,       // import
    TOKEN_EXPORT,       // export
    TOKEN_PLUGIN,       // plugin
    TOKEN_ECHO,         // print
    TOKEN_TYPEOF,       // typeof operator
    TOKEN_XTREME,       // unsafe/fast mode
    TOKEN_ALLOC,        // allocate
    TOKEN_FREE,         // free memory
    TOKEN_MOVE,         // move semantics
    
    // Types
    TOKEN_I32,
    TOKEN_I64,
    TOKEN_F32,
    TOKEN_F64,
    TOKEN_BOOL,
    TOKEN_CHAR_TYPE,
    TOKEN_PTR,
    
    // Operators
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    TOKEN_PERCENT,      // %
    TOKEN_EQUAL,        // =
    TOKEN_EQUAL_EQUAL,  // ==
    TOKEN_BANG,         // !
    TOKEN_BANG_EQUAL,   // !=
    TOKEN_LESS,         // <
    TOKEN_LESS_EQUAL,   // <=
    TOKEN_GREATER,      // >
    TOKEN_GREATER_EQUAL,// >=
    TOKEN_AND_AND,      // &&
    TOKEN_OR_OR,        // ||
    TOKEN_AMPERSAND,    // &
    TOKEN_PIPE,         // |
    TOKEN_CARET,        // ^
    TOKEN_TILDE,        // ~
    TOKEN_LEFT_SHIFT,   // <<
    TOKEN_RIGHT_SHIFT,  // >>
    TOKEN_ARROW,        // ->
    TOKEN_DOT,          // .
    TOKEN_DOT_DOT,      // ..
    TOKEN_COLON,        // :
    TOKEN_DOUBLE_COLON, // ::
    
    // Delimiters
    TOKEN_LEFT_PAREN,   // (
    TOKEN_RIGHT_PAREN,  // )
    TOKEN_LEFT_BRACE,   // {
    TOKEN_RIGHT_BRACE,  // }
    TOKEN_LEFT_BRACKET, // [
    TOKEN_RIGHT_BRACKET,// ]
    TOKEN_COMMA,        // ,
    TOKEN_SEMICOLON,    // ;
    
    // Special
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    int line;
    int column;
} Token;

// Lexer structure
typedef struct {
    const char* source;
    const char* start;
    const char* current;
    int line;
    int column;
} Lexer;

// Lexer functions
void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);
const char* token_type_to_string(TokenType type);
void token_print(Token* token);

#endif // RADS_LEXER_H
