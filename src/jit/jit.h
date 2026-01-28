#ifndef RADS_JIT_H
#define RADS_JIT_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// JIT COMPILATION ENGINE - LLVM Backend for Hot Path Optimization
// RADS v0.0.7 "DARK MOON"
// ============================================================================

// Forward declarations
typedef struct VM VM;
typedef struct Value Value;

// JIT compilation result
typedef enum {
    JIT_SUCCESS = 0,
    JIT_ERROR_COMPILE,
    JIT_ERROR_MEMORY,
    JIT_ERROR_UNSUPPORTED
} JITResult;

// JIT-compiled function handle
typedef struct {
    void* native_code;
    size_t code_size;
    char* function_name;
    bool is_hot;
    int call_count;
} JITCompiledFunction;

// Code cache entry
typedef struct {
    char* key;  // Function signature/name
    JITCompiledFunction* function;
    struct CodeCacheEntry* next;
} CodeCacheEntry;

// JIT compilation state
typedef struct {
    CodeCacheEntry* cache;
    size_t cache_size;
    size_t cache_hits;
    size_t cache_misses;
    bool hot_path_detection_enabled;
    int hot_path_threshold;  // Minimum calls before JIT compilation
} JITState;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize the JIT compilation engine
 *
 * @param vm Pointer to the virtual machine
 * @return JIT state pointer on success, NULL on failure
 */
JITState* jit_init(VM* vm);

/**
 * Compile a RADS function using LLVM JIT
 *
 * @param state JIT compilation state
 * @param function_name Name of the function to compile
 * @param bytecode Pointer to bytecode
 * @param bytecode_size Size of bytecode
 * @param is_hot Whether this function is a hot path
 * @return Compiled function handle on success, NULL on failure
 */
JITCompiledFunction* jit_compile_function(
    JITState* state,
    const char* function_name,
    const unsigned char* bytecode,
    size_t bytecode_size,
    bool is_hot
);

/**
 * Execute a JIT-compiled function
 *
 * @param function Compiled function handle
 * @param vm Virtual machine instance
 * @return Value result
 */
Value jit_execute_function(JITCompiledFunction* function, VM* vm);

/**
 * Check if a function is hot enough for JIT compilation
 *
 * @param state JIT compilation state
 * @param function_name Function to check
 * @param call_count Current call count
 * @return true if function should be JIT compiled
 */
bool jit_should_compile(JITState* state, const char* function_name, int call_count);

/**
 * Clear the JIT code cache
 *
 * @param state JIT compilation state
 */
void jit_clear_cache(JITState* state);

/**
 * Get JIT compilation statistics
 *
 * @param state JIT compilation state
 * @param[out] cache_hits Number of cache hits
 * @param[out] cache_misses Number of cache misses
 * @param[out] cache_size Total cache size
 */
void jit_get_statistics(
    JITState* state,
    size_t* cache_hits,
    size_t* cache_misses,
    size_t* cache_size
);

/**
 * Cleanup and free JIT compilation state
 *
 * @param state JIT compilation state to free
 */
void jit_cleanup(JITState* state);

// ============================================================================
// INTERNAL LLVM INTEGRATION
// ============================================================================

/**
 * Initialize LLVM backend
 * Must be called before any JIT operations
 *
 * @return true on success, false on failure
 */
bool jit_init_llvm(void);

/**
 * Shutdown LLVM backend
 */
void jit_shutdown_llvm(void);

#endif // RADS_JIT_H
