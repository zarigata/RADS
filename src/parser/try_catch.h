#ifndef RADS_PARSER_TRYCATCH_H
#define RADS_PARSER_TRYCATCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// Forward declarations
typedef struct VM VM;
typedef struct Value Value;
typedef struct ASTNode ASTNode;
typedef struct Token Token;

// Try-catch node types
typedef enum {
    TRY_NODE = 0,
    CATCH_NODE = 1,
    FINALLY_NODE = 2
} TryCatchNodeType;

// Try-catch statement
typedef struct {
    TryCatchNodeType type;
    ASTNode* try_block;
    ASTNode* catch_block;
    ASTNode* finally_block;
    ASTNode* error_expr;
} TryCatchStatement;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Parse try-catch statement
 *
 * @param tokens Token list
 * @return TryCatchStatement or NULL
 */
TryCatchStatement* parser_parse_try_catch(Token* tokens);

/**
 * Generate try-catch AST from parsed statement
 *
 * @param stmt Try-catch statement
 * @return AST node representing the statement
 */
ASTNode* ast_generate_try_catch(TryCatchStatement* stmt);
