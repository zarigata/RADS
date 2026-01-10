#ifndef RADS_PARSER_H
#define RADS_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

// Parser functions
void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parser_parse(Parser* parser);
ASTNode* parse_statement(Parser* parser);  // For REPL mode

#endif // RADS_PARSER_H
