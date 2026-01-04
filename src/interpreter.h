#ifndef RADS_INTERPRETER_H
#define RADS_INTERPRETER_H

#include "ast.h"

// Value types for runtime
typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool bool_val;
        long long int_val;
        double float_val;
        char* string_val;
    };
} Value;

// Interpreter functions
int interpret(ASTNode* program);
void value_print(Value* value);
void value_free(Value* value);

#endif // RADS_INTERPRETER_H
