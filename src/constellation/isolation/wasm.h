/*
 * RADS Constellation - WebAssembly Runtime
 * Phase 9 (Iguana): Nice to Have Features
 *
 * WebAssembly instance management and execution
 * Provides sandboxed, portable execution environment
 */

#ifndef CONSTELLATION_WASM_H
#define CONSTELLATION_WASM_H

#include "../constellation.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * WASM RUNTIME
 * ============================================================================
 */

#define MAX_WASM_INSTANCES 1000
#define MAX_WASM_MODULES 100
#define MAX_WASM_IMPORTS 64
#define MAX_WASM_EXPORTS 64
#define MAX_WASM_MEMORY_MB 512

typedef enum {
    WASM_RUNTIME_WASMTIME = 0,
    WASM_RUNTIME_WASMER = 1,
    WASM_RUNTIME_WASM3 = 2,
    WASM_RUNTIME_NATIVE = 3
} WASMRuntimeType;

typedef enum {
    WASM_STATE_CREATED = 0,
    WASM_STATE_INITIALIZED = 1,
    WASM_STATE_RUNNING = 2,
    WASM_STATE_PAUSED = 3,
    WASM_STATE_STOPPED = 4,
    WASM_STATE_ERROR = 5
} WASMState;

typedef enum {
    WASM_VALUE_I32 = 0,
    WASM_VALUE_I64 = 1,
    WASM_VALUE_F32 = 2,
    WASM_VALUE_F64 = 3,
    WASM_VALUE_V128 = 4,
    WASM_VALUE_FUNCREF = 5,
    WASM_VALUE_EXTERNREF = 6
} WASMValueType;

typedef union {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    void *ref;
} WASMValue;

typedef struct {
    WASMValueType type;
    WASMValue value;
} WASMTypedValue;

typedef struct {
    char name[128];
    WASMValueType *param_types;
    int param_count;
    WASMValueType *return_types;
    int return_count;
} WASMFunctionSignature;

typedef struct {
    char name[128];
    WASMFunctionSignature signature;
    void *function_ptr;
} WASMImport;

typedef struct {
    char name[128];
    WASMFunctionSignature signature;
    uint32_t function_index;
} WASMExport;

typedef struct {
    char id[64];
    char module_path[512];
    
    uint8_t *bytecode;
    size_t bytecode_size;
    
    WASMImport imports[MAX_WASM_IMPORTS];
    int import_count;
    
    WASMExport exports[MAX_WASM_EXPORTS];
    int export_count;
    
    uint32_t initial_memory_pages;
    uint32_t max_memory_pages;
    
    bool has_start_function;
    bool is_compiled;
    
    time_t loaded_at;
} WASMModule;

typedef struct {
    char id[64];
    char module_id[64];
    
    WASMRuntimeType runtime_type;
    WASMState state;
    
    void *runtime_context;
    void *module_instance;
    
    uint8_t *memory;
    size_t memory_size;
    size_t memory_capacity;
    
    uint64_t instructions_executed;
    uint64_t gas_used;
    uint64_t gas_limit;
    
    time_t created_at;
    time_t started_at;
    time_t stopped_at;
    
    char error_message[512];
} WASMInstance;

typedef struct {
    uint64_t max_memory_bytes;
    uint64_t max_table_elements;
    uint64_t max_instances;
    uint64_t gas_limit;
    uint32_t stack_size;
    
    bool enable_simd;
    bool enable_threads;
    bool enable_bulk_memory;
    bool enable_reference_types;
    
    int execution_timeout_ms;
} WASMConfig;

typedef struct {
    int total_modules;
    int total_instances;
    int running_instances;
    uint64_t total_memory_used;
    uint64_t total_instructions_executed;
    uint64_t total_gas_used;
} WASMStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int wasm_init(WASMConfig *config);
int wasm_shutdown(void);
bool wasm_is_available(void);

/*
 * ============================================================================
 * MODULE MANAGEMENT
 * ============================================================================
 */

WASMModule *wasm_load_module(const char *module_path);
WASMModule *wasm_load_module_from_bytes(const uint8_t *bytecode, size_t size);
int wasm_unload_module(const char *module_id);
WASMModule *wasm_get_module(const char *module_id);
WASMModule **wasm_list_modules(int *count);

int wasm_compile_module(WASMModule *module);
int wasm_validate_module(WASMModule *module);

/*
 * ============================================================================
 * INSTANCE MANAGEMENT
 * ============================================================================
 */

WASMInstance *wasm_create_instance(
    const char *module_id,
    WASMRuntimeType runtime_type
);

int wasm_destroy_instance(const char *instance_id);
WASMInstance *wasm_get_instance(const char *instance_id);
WASMInstance **wasm_list_instances(int *count);

int wasm_start_instance(WASMInstance *instance);
int wasm_stop_instance(WASMInstance *instance);
int wasm_pause_instance(WASMInstance *instance);
int wasm_resume_instance(WASMInstance *instance);

/*
 * ============================================================================
 * FUNCTION EXECUTION
 * ============================================================================
 */

int wasm_call_function(
    WASMInstance *instance,
    const char *function_name,
    WASMTypedValue *params,
    int param_count,
    WASMTypedValue *results,
    int *result_count
);

int wasm_call_exported_function(
    const char *instance_id,
    const char *function_name,
    WASMTypedValue *params,
    int param_count,
    WASMTypedValue *results,
    int *result_count
);

/*
 * ============================================================================
 * MEMORY MANAGEMENT
 * ============================================================================
 */

int wasm_memory_grow(WASMInstance *instance, uint32_t pages);
uint32_t wasm_memory_size(WASMInstance *instance);
uint8_t *wasm_memory_data(WASMInstance *instance);

int wasm_memory_read(
    WASMInstance *instance,
    uint32_t offset,
    void *buffer,
    size_t size
);

int wasm_memory_write(
    WASMInstance *instance,
    uint32_t offset,
    const void *data,
    size_t size
);

/*
 * ============================================================================
 * IMPORTS & EXPORTS
 * ============================================================================
 */

int wasm_register_import(
    WASMModule *module,
    const char *name,
    WASMFunctionSignature *signature,
    void *function_ptr
);

WASMExport *wasm_get_export(WASMModule *module, const char *name);
WASMExport **wasm_list_exports(WASMModule *module, int *count);

/*
 * ============================================================================
 * GAS METERING
 * ============================================================================
 */

int wasm_set_gas_limit(WASMInstance *instance, uint64_t limit);
uint64_t wasm_get_gas_used(WASMInstance *instance);
uint64_t wasm_get_gas_remaining(WASMInstance *instance);
int wasm_refill_gas(WASMInstance *instance, uint64_t amount);

/*
 * ============================================================================
 * SANDBOXING
 * ============================================================================
 */

int wasm_enable_sandbox(WASMInstance *instance);
int wasm_disable_sandbox(WASMInstance *instance);
bool wasm_is_sandboxed(WASMInstance *instance);

int wasm_set_resource_limits(
    WASMInstance *instance,
    uint64_t max_memory,
    uint64_t max_instructions
);

/*
 * ============================================================================
 * SNAPSHOTS
 * ============================================================================
 */

int wasm_snapshot_create(WASMInstance *instance, const char *snapshot_id);
int wasm_snapshot_restore(WASMInstance *instance, const char *snapshot_id);
int wasm_snapshot_delete(const char *snapshot_id);

/*
 * ============================================================================
 * WASI SUPPORT
 * ============================================================================
 */

int wasm_wasi_init(WASMInstance *instance);
int wasm_wasi_set_args(WASMInstance *instance, char **args, int arg_count);
int wasm_wasi_set_env(WASMInstance *instance, char **env, int env_count);
int wasm_wasi_set_preopened_dir(WASMInstance *instance, const char *path);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

WASMStats wasm_get_stats(void);
int wasm_get_instance_stats(const char *instance_id, WASMStats *stats);

/*
 * ============================================================================
 * RUNTIME-SPECIFIC FUNCTIONS
 * ============================================================================
 */

int wasm_wasmtime_init(void);
int wasm_wasmtime_create_instance(WASMModule *module, WASMInstance *instance);
int wasm_wasmtime_call(WASMInstance *instance, const char *func, 
                       WASMTypedValue *params, int param_count,
                       WASMTypedValue *results, int *result_count);

int wasm_wasmer_init(void);
int wasm_wasmer_create_instance(WASMModule *module, WASMInstance *instance);
int wasm_wasmer_call(WASMInstance *instance, const char *func,
                     WASMTypedValue *params, int param_count,
                     WASMTypedValue *results, int *result_count);

int wasm_wasm3_init(void);
int wasm_wasm3_create_instance(WASMModule *module, WASMInstance *instance);
int wasm_wasm3_call(WASMInstance *instance, const char *func,
                    WASMTypedValue *params, int param_count,
                    WASMTypedValue *results, int *result_count);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

void wasm_generate_instance_id(char *id, size_t size);
void wasm_generate_module_id(char *id, size_t size);
int wasm_parse_wasm_binary(const uint8_t *bytecode, size_t size, WASMModule *module);
int wasm_validate_signature(WASMFunctionSignature *signature);

#endif // CONSTELLATION_WASM_H
