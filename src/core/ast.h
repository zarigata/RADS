#ifndef RADS_AST_H
#define RADS_AST_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct ASTList ASTList;

// AST Node Types
typedef enum {
    // Literals
    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_BOOL_LITERAL,
    AST_NULL_LITERAL,
    
    // Identifiers
    AST_IDENTIFIER,
    
    // Binary operations
    AST_BINARY_OP,
    
    // Unary operations
    AST_UNARY_OP,
    
    // Statements
    AST_FUNCTION_DECL,
    AST_VARIABLE_DECL,
    AST_STRUCT_DECL,
    AST_ENUM_DECL,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_LOOP_STMT,
    AST_CRUISE_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_ECHO_STMT,
    AST_IMPORT_STMT,
    AST_BLOCK,
    
    // Expressions
    AST_CALL_EXPR,
    AST_ASSIGN_EXPR,
    AST_ARRAY_LITERAL,
    AST_INDEX_EXPR,
    AST_MEMBER_EXPR,
    AST_TYPEOF_EXPR,
    AST_STRUCT_LITERAL,
    
    // Types
    AST_TYPE,
    
    // Program
    AST_PROGRAM
} ASTNodeType;

// Operator types
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_NEG,
    OP_RANGE
} OperatorType;

// Type information
typedef struct {
    char* name;
    bool is_array;
    bool is_turbo;
} TypeInfo;

// AST Node List (for function parameters, statements, etc.)
struct ASTList {
    ASTNode** nodes;
    size_t count;
    size_t capacity;
};

// AST Node structure
struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        // Literals
        struct {
            long long value;
        } integer_literal;
        
        struct {
            double value;
        } float_literal;
        
        struct {
            char* value;
        } string_literal;
        
        struct {
            char value;
        } char_literal;
        
        struct {
            bool value;
        } bool_literal;
        
        // Identifier
        struct {
            char* name;
        } identifier;
        
        // Binary operation
        struct {
            OperatorType op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        
        // Unary operation
        struct {
            OperatorType op;
            ASTNode* operand;
        } unary_op;
        
        // Typeof expression
        struct {
            ASTNode* operand;
        } typeof_expr;
        
        // Function declaration
        struct {
            char* name;
            ASTList* parameters;
            TypeInfo* return_type;
            ASTNode* body;
            bool is_async;
        } function_decl;
        
        // Variable declaration
        struct {
            char* name;
            TypeInfo* var_type;
            ASTNode* initializer;
            bool is_turbo;
        } variable_decl;
        
        // Struct declaration
        struct {
            char* name;
            ASTList* fields;
        } struct_decl;

        // Enum declaration
        struct {
            char* name;
            ASTList* values;  // List of identifier nodes
        } enum_decl;

        // Return statement
        struct {
            ASTNode* value;
        } return_stmt;
        
        // If statement
        struct {
            ASTNode* condition;
            ASTNode* then_branch;
            ASTNode* else_branch;
        } if_stmt;
        
        // Loop statement
        struct {
            ASTNode* condition;
            ASTNode* body;
        } loop_stmt;
        
        // Cruise (for) statement
        struct {
            char* iterator;
            ASTNode* iterable;
            ASTNode* body;
        } cruise_stmt;
        
        // Echo statement
        struct {
            ASTNode* expression;
        } echo_stmt;

        // Import statement
        struct {
            char* filename;
        } import_stmt;

        // Block
        struct {
            ASTList* statements;
        } block;
        
        // Function call
        struct {
            ASTNode* callee;
            ASTList* arguments;
        } call_expr;
        
        // Assignment
        struct {
            ASTNode* target;
            ASTNode* value;
        } assign_expr;
        
        // Array literal
        struct {
            ASTList* elements;
        } array_literal;
        
        // Index expression
        struct {
            ASTNode* array;
            ASTNode* index;
        } index_expr;
        
        // Member expression
        struct {
            ASTNode* object;
            char* member;
        } member_expr;

        // Struct literal
        struct {
            char* name;
            ASTList* fields;
        } struct_literal;
        
        // Program
        struct {
            ASTList* declarations;
        } program;
    };
};

// AST creation functions
ASTNode* ast_create_integer(long long value, int line, int column);
ASTNode* ast_create_float(double value, int line, int column);
ASTNode* ast_create_string(const char* value, int line, int column);
ASTNode* ast_create_bool(bool value, int line, int column);
ASTNode* ast_create_null(int line, int column);
ASTNode* ast_create_identifier(const char* name, int line, int column);
ASTNode* ast_create_binary_op(OperatorType op, ASTNode* left, ASTNode* right, int line, int column);
ASTNode* ast_create_unary_op(OperatorType op, ASTNode* operand, int line, int column);
ASTNode* ast_create_typeof(ASTNode* operand, int line, int column);
ASTNode* ast_create_function_decl(const char* name, ASTList* params, TypeInfo* return_type, ASTNode* body, bool is_async, int line, int column);
ASTNode* ast_create_variable_decl(const char* name, TypeInfo* type, ASTNode* initializer, bool is_turbo, int line, int column);
ASTNode* ast_create_struct_decl(const char* name, ASTList* fields, int line, int column);
ASTNode* ast_create_enum_decl(const char* name, ASTList* values, int line, int column);
ASTNode* ast_create_return(ASTNode* value, int line, int column);
ASTNode* ast_create_if(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int line, int column);
ASTNode* ast_create_loop(ASTNode* condition, ASTNode* body, int line, int column);
ASTNode* ast_create_echo(ASTNode* expression, int line, int column);
ASTNode* ast_create_import(const char* filename, int line, int column);
ASTNode* ast_create_block(ASTList* statements, int line, int column);
ASTNode* ast_create_call(ASTNode* callee, ASTList* arguments, int line, int column);
ASTNode* ast_create_member_expr(ASTNode* object, const char* member, int line, int column);
ASTNode* ast_create_struct_literal(const char* name, ASTList* fields, int line, int column);
ASTNode* ast_create_array_literal(ASTList* elements, int line, int column);
ASTNode* ast_create_index(ASTNode* array, ASTNode* index, int line, int column);
ASTNode* ast_create_assign(ASTNode* target, ASTNode* value, int line, int column);
ASTNode* ast_create_cruise(const char* iterator, ASTNode* iterable, ASTNode* body, int line, int column);
ASTNode* ast_create_break(int line, int column);
ASTNode* ast_create_continue(int line, int column);
ASTNode* ast_create_program(ASTList* declarations);

// AST list functions
ASTList* ast_list_create();
void ast_list_append(ASTList* list, ASTNode* node);
void ast_list_free(ASTList* list);

// Type info functions
TypeInfo* type_info_create(const char* name, bool is_array, bool is_turbo);
void type_info_free(TypeInfo* type);

// AST cleanup
void ast_free(ASTNode* node);

// AST printing (for debugging)
void ast_print(ASTNode* node, int indent);

#endif // RADS_AST_H
