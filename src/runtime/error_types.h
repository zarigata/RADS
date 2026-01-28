#include "stdlib.h"
#include <stdio.h>

// ============================================================================
// ERROR TYPES - Custom Error Types with Stack Traces
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// Error categories
typedef enum {
    ERROR_CATEGORY_IO = 0,
    ERROR_CATEGORY_NETWORK = 1,
    ERROR_CATEGORY_DATABASE = 2,
    ERROR_CATEGORY_TYPE = 3,
    ERROR_CATEGORY_MEMORY = 4,
    ERROR_CATEGORY_RUNTIME = 5
} ErrorCategory;

// Error type
typedef enum {
    ERROR_KIND_CRITICAL = 0,
    ERROR_KIND_ERROR = 1,
    ERROR_KIND_WARNING = 2,
    ERROR_KIND_INFO = 3
} ErrorKind;

// Error structure
typedef struct {
    ErrorCategory category;
    ErrorKind kind;
    char* message;
    char* file;
    int line_number;
    int column;
    char* function_name;
    int error_code;
    char* suggestion;
    struct Error* next;
    struct Error* cause;
} Error;

// Error type
typedef struct {
    ErrorType type;
    char* type_name;
    char* message;
} ErrorType;

// Error trace entry
typedef struct {
    char* function_name;
    char* file;
    int line_number;
    int column;
} ErrorTrace;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize error types system
 */
ErrorType* error_init(void);

/**
 * Create a new error
 *
 * @param category Error category
 * @param kind Error kind
 * @param message Error message
 * @return Error handle
 */
Error* error_create(
    ErrorCategory category,
    ErrorKind kind,
    const char* message
);

/**
 * Throw a typed error
 *
 * @param type Error type information
 * @param message Error message
 */
Error* error_throw_type(ErrorType* type, const char* message);

/**
 * Get current error (for catch blocks)
 *
 * @return Current error or NULL if none
 */
Error* error_get_current(void);

/**
 * Get error trace
 *
 * @return Trace of stack frames or NULL
 */
ErrorTrace* error_get_trace(void);

/**
 * Cleanup and free error system
 */
void error_cleanup(void);
