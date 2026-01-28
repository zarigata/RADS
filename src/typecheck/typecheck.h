#ifndef RADS_TYPECHECK_H
#define RADS_TYPECHECK_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// TYPE CHECKING AND INFERENCE - Optional Types, Generics, Union Types
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// Forward declarations
typedef struct VM VM;
typedef struct Value Value;

// Type categories
typedef enum {
    TYPE_KIND_NONE = 0,
    TYPE_KIND_OPTIONAL = 1,
    TYPE_KIND_GENERIC = 2,
    TYPE_KIND_UNION = 3
} TypeKind;

// Generic type parameters
typedef struct {
    char* name;
    int num_params;
    TypeKind kind;
    struct GenericParam* params;
    struct GenericType* next;
} GenericType;

// Type information
typedef struct {
    TypeKind kind;
    char* name;
    bool is_optional;
    bool is_validated;
    struct TypeInfo* next;
} TypeInfo;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize type checker
 *
 * @param vm Pointer to the virtual machine
 * @return Type checker state pointer on success, NULL on failure
 */
TypeChecker* typecheck_init(VM* vm);

/**
 * Add optional type annotation
 *
 * @param state Type checker state
 * @param var_name Variable name
 * @param type_name Type name
 * @return true on success
 */
bool typecheck_add_annotation(TypeChecker* state, const char* var_name, const char* type_name);

/**
 * Validate type annotation
 *
 * @param state Type checker state
 * @param type_info Type information to validate
 * @return true if valid
 */
bool typecheck_validate(TypeChecker* state, TypeInfo* type_info);

/**
 * Perform type inference
 *
 * @param state Type checker state
 * @param expression AST node to infer type from
 * @return Inferred type, or NULL if can't infer
 */
TypeInfo* typecheck_infer(TypeChecker* state, void* expression);

/**
 * Get type of variable
 *
 * @param state Type checker state
 * @param var_name Variable name
 * @return Type information for variable
 */
TypeInfo* typecheck_get_type(TypeChecker* state, const char* var_name);

/**
 * Cleanup and free type checker state
 *
 * @param state Type checker state to free
 */
void typecheck_cleanup(TypeChecker* state);

#endif // RADS_TYPECHECK_H
