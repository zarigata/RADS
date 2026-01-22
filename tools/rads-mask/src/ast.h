/**
 * RADS Masquerading - Language-Agnostic AST
 *
 * Universal Abstract Syntax Tree representation for all source languages.
 * This AST is designed to be language-independent and easily convertible to RADS.
 *
 * Version: 0.0.5 "CHAMELEON"
 */

#ifndef RADS_MASK_AST_H
#define RADS_MASK_AST_H

#include <stdlib.h>
#include <string.h>

// Forward declarations
typedef struct MaskASTNode MaskASTNode;
typedef struct MaskASTList MaskASTList;

// ============================================================================
// AST Node Types
// ============================================================================

typedef enum {
    // Literals
    MASK_NODE_INT_LITERAL,
    MASK_NODE_FLOAT_LITERAL,
    MASK_NODE_STRING_LITERAL,
    MASK_NODE_BOOL_LITERAL,
    MASK_NODE_NULL_LITERAL,

    // Identifiers
    MASK_NODE_IDENTIFIER,

    // Operators
    MASK_NODE_BINARY_OP,
    MASK_NODE_UNARY_OP,
    MASK_NODE_ASSIGN,

    // Control Flow
    MASK_NODE_IF_STMT,
    MASK_NODE_WHILE_LOOP,
    MASK_NODE_FOR_LOOP,
    MASK_NODE_BREAK,
    MASK_NODE_CONTINUE,
    MASK_NODE_RETURN,

    // Functions
    MASK_NODE_FUNCTION_DEF,
    MASK_NODE_FUNCTION_CALL,
    MASK_NODE_PARAM_LIST,
    MASK_NODE_ARG_LIST,

    // Data Structures
    MASK_NODE_ARRAY_LITERAL,
    MASK_NODE_DICT_LITERAL,
    MASK_NODE_STRUCT_DEF,
    MASK_NODE_CLASS_DEF,

    // Access
    MASK_NODE_MEMBER_ACCESS,
    MASK_NODE_INDEX_ACCESS,

    // Variables
    MASK_NODE_VAR_DECL,

    // Expressions
    MASK_NODE_EXPR_STMT,

    // Special
    MASK_NODE_IMPORT,
    MASK_NODE_BLOCK,
    MASK_NODE_PROGRAM,

} MaskNodeType;

// ============================================================================
// Operator Types
// ============================================================================

typedef enum {
    // Arithmetic
    MASK_OP_ADD,
    MASK_OP_SUB,
    MASK_OP_MUL,
    MASK_OP_DIV,
    MASK_OP_MOD,
    MASK_OP_POW,

    // Comparison
    MASK_OP_EQ,
    MASK_OP_NE,
    MASK_OP_LT,
    MASK_OP_LE,
    MASK_OP_GT,
    MASK_OP_GE,

    // Logical
    MASK_OP_AND,
    MASK_OP_OR,
    MASK_OP_NOT,

    // Unary
    MASK_OP_NEGATE,
    MASK_OP_POSITIVE,

} MaskOperator;

// ============================================================================
// AST Node Structure
// ============================================================================

struct MaskASTNode {
    MaskNodeType type;

    union {
        // Literals
        struct {
            long long int_value;
        } int_literal;

        struct {
            double float_value;
        } float_literal;

        struct {
            char* string_value;
        } string_literal;

        struct {
            int bool_value;
        } bool_literal;

        // Identifier
        struct {
            char* name;
        } identifier;

        // Binary operation
        struct {
            MaskOperator op;
            MaskASTNode* left;
            MaskASTNode* right;
        } binary_op;

        // Unary operation
        struct {
            MaskOperator op;
            MaskASTNode* operand;
        } unary_op;

        // Assignment
        struct {
            MaskASTNode* target;
            MaskASTNode* value;
        } assign;

        // If statement
        struct {
            MaskASTNode* condition;
            MaskASTNode* then_block;
            MaskASTNode* else_block;  // May be NULL
        } if_stmt;

        // While loop
        struct {
            MaskASTNode* condition;
            MaskASTNode* body;
        } while_loop;

        // For loop
        struct {
            MaskASTNode* init;
            MaskASTNode* condition;
            MaskASTNode* increment;
            MaskASTNode* body;
        } for_loop;

        // Return statement
        struct {
            MaskASTNode* value;  // May be NULL
        } return_stmt;

        // Function definition
        struct {
            char* name;
            MaskASTNode* params;
            MaskASTNode* body;
        } function_def;

        // Function call
        struct {
            MaskASTNode* function;
            MaskASTNode* args;
        } function_call;

        // Array literal
        struct {
            MaskASTList* elements;
        } array_literal;

        // Dictionary literal
        struct {
            MaskASTList* keys;
            MaskASTList* values;
        } dict_literal;

        // Member access (obj.member)
        struct {
            MaskASTNode* object;
            char* member;
        } member_access;

        // Index access (arr[index])
        struct {
            MaskASTNode* object;
            MaskASTNode* index;
        } index_access;

        // Variable declaration
        struct {
            char* name;
            MaskASTNode* init_value;  // May be NULL
            char* type_hint;          // May be NULL
        } var_decl;

        // Expression statement
        struct {
            MaskASTNode* expr;
        } expr_stmt;

        // Import
        struct {
            char* module_name;
            MaskASTList* imported_names;  // May be NULL for "import module"
        } import;

        // Block
        struct {
            MaskASTList* statements;
        } block;

        // Program (root)
        struct {
            MaskASTList* statements;
        } program;

    } data;
};

// ============================================================================
// AST List (for multiple children)
// ============================================================================

struct MaskASTList {
    MaskASTNode* node;
    MaskASTList* next;
};

// ============================================================================
// Constructor Functions
// ============================================================================

MaskASTNode* mask_ast_create_int(long long value);
MaskASTNode* mask_ast_create_float(double value);
MaskASTNode* mask_ast_create_string(const char* value);
MaskASTNode* mask_ast_create_bool(int value);
MaskASTNode* mask_ast_create_null(void);
MaskASTNode* mask_ast_create_identifier(const char* name);

MaskASTNode* mask_ast_create_binary_op(MaskOperator op, MaskASTNode* left, MaskASTNode* right);
MaskASTNode* mask_ast_create_unary_op(MaskOperator op, MaskASTNode* operand);
MaskASTNode* mask_ast_create_assign(MaskASTNode* target, MaskASTNode* value);

MaskASTNode* mask_ast_create_if(MaskASTNode* condition, MaskASTNode* then_block, MaskASTNode* else_block);
MaskASTNode* mask_ast_create_while(MaskASTNode* condition, MaskASTNode* body);
MaskASTNode* mask_ast_create_for(MaskASTNode* init, MaskASTNode* condition, MaskASTNode* increment, MaskASTNode* body);
MaskASTNode* mask_ast_create_return(MaskASTNode* value);
MaskASTNode* mask_ast_create_break(void);
MaskASTNode* mask_ast_create_continue(void);

MaskASTNode* mask_ast_create_function_def(const char* name, MaskASTNode* params, MaskASTNode* body);
MaskASTNode* mask_ast_create_function_call(MaskASTNode* function, MaskASTNode* args);

MaskASTNode* mask_ast_create_array(MaskASTList* elements);
MaskASTNode* mask_ast_create_dict(MaskASTList* keys, MaskASTList* values);

MaskASTNode* mask_ast_create_member_access(MaskASTNode* object, const char* member);
MaskASTNode* mask_ast_create_index_access(MaskASTNode* object, MaskASTNode* index);

MaskASTNode* mask_ast_create_var_decl(const char* name, MaskASTNode* init_value, const char* type_hint);
MaskASTNode* mask_ast_create_expr_stmt(MaskASTNode* expr);

MaskASTNode* mask_ast_create_import(const char* module_name, MaskASTList* imported_names);
MaskASTNode* mask_ast_create_block(MaskASTList* statements);
MaskASTNode* mask_ast_create_program(MaskASTList* statements);

// List functions
MaskASTList* mask_ast_list_create(MaskASTNode* node);
MaskASTList* mask_ast_list_append(MaskASTList* list, MaskASTNode* node);
int mask_ast_list_length(MaskASTList* list);

// Cleanup
void mask_ast_free(MaskASTNode* node);
void mask_ast_list_free(MaskASTList* list);

#endif // RADS_MASK_AST_H
