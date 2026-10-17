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
    bool no_struct_literal; // true while parsing a cruise iterable: `Name {`
                            // there is ALWAYS the loop body, never a struct literal
} Parser;

// Parser functions
void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parser_parse(Parser* parser);
ASTNode* parse_statement(Parser* parser);  // For REPL mode
ASTNode* parse_repl_line(Parser* parser);  // Run 27: REPL top-level entry (struct/enum/import/blast + statements)

#endif // RADS_PARSER_H
