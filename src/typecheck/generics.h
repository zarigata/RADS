#ifndef RADS_GENERICS_H
#define RADS_GENERICS_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// GENERICS SUPPORT FOR RADS
// Generic types, type parameters, and instantiation
// ============================================================================

// Forward declarations
typedef struct Type Type;
typedef struct TypeContext TypeContext;

// Generic type structure
typedef struct Type {
    char* name;                    // Type name (e.g., "List", "Array")
    char* base_type;               // Base type (e.g., "unknown", "int", "string")
    char** type_params;            // Type parameters (e.g., ["T"])
    int param_count;               // Number of type parameters
    int is_polymorphic;            // Whether this is a generic type
} Type;

// Type context for managing all generic types and instances
typedef struct TypeContext {
    Type** types;                  // Array of generic type definitions
    int type_count;                // Number of generic type definitions
    Type** instances;              // Array of instantiated types
    int instance_count;           // Number of instantiated types
} TypeContext;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize the generics system
 * 
 * Must be called before any other generics functions
 */
void typecheck_generics_init(void);

/**
 * Create a generic type definition
 * 
 * @param name Name of the generic type (e.g., "List")
 * @param base_type Base type (e.g., "unknown", "int")
 * @param params Array of type parameter names (e.g., ["T"])
 * @param param_count Number of type parameters
 * @return Pointer to created Type, or NULL on failure
 */
Type* typecheck_create_generic(const char* name, const char* base_type, char** params, int param_count);

/**
 * Instantiate a generic type with concrete type parameters
 * 
 * @param generic_type The generic type to instantiate
 * @param concrete_params Array of concrete type names
 * @param param_count Number of concrete parameters
 * @return Pointer to instantiated Type, or NULL on failure
 */
Type* typecheck_instantiate_generic(Type* generic_type, char** concrete_params, int param_count);

/**
 * Check type compatibility between two types
 * 
 * @param type1 First type to compare
 * @param type2 Second type to compare
 * @return 1 if compatible, 0 if not compatible
 */
int typecheck_check_type_compatibility(Type* type1, Type* type2);

/**
 * Find a type by name (searches both definitions and instances)
 * 
 * @param name Name of the type to find
 * @return Pointer to Type, or NULL if not found
 */
Type* typecheck_find_type(const char* name);

/**
 * Register generic function types
 * 
 * @param name Name of the function to register generics for
 */
void typecheck_generic_register_function(const char* name);

/**
 * Run built-in generics test suite
 * 
 * @return 1 if all tests pass, 0 if any test fails
 */
int typecheck_generics_test(void);

/**
 * Cleanup and free all generics system resources
 * 
 * Frees all memory allocated by the generics system
 */
void typecheck_generics_cleanup(void);

/**
 * Print information about the current generics system state
 */
void typecheck_generics_info(void);

#endif // RADS_GENERICS_H