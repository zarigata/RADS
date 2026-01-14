/*
 * RADS Constellation - Serverless Functions (FaaS)
 * Phase 9 (Iguana): Nice to Have Features
 *
 * Function-as-a-Service platform for event-driven computing
 * Cold start optimization, auto-scaling, and pay-per-use model
 */

#ifndef CONSTELLATION_SERVERLESS_H
#define CONSTELLATION_SERVERLESS_H

#include "../constellation.h"
#include "../autoscale/metrics.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * SERVERLESS FUNCTIONS
 * ============================================================================
 */

#define MAX_FUNCTIONS 1000
#define MAX_FUNCTION_NAME 128
#define MAX_TRIGGERS 16
#define MAX_COLD_POOL_SIZE 100
#define MAX_WARM_POOL_SIZE 50

typedef enum {
    FUNCTION_RUNTIME_RADS = 0,
    FUNCTION_RUNTIME_WASM = 1,
    FUNCTION_RUNTIME_NATIVE = 2
} FunctionRuntime;

typedef enum {
    FUNCTION_STATE_COLD = 0,
    FUNCTION_STATE_WARMING = 1,
    FUNCTION_STATE_WARM = 2,
    FUNCTION_STATE_RUNNING = 3,
    FUNCTION_STATE_COOLING = 4,
    FUNCTION_STATE_ERROR = 5
} FunctionState;

typedef enum {
    TRIGGER_TYPE_HTTP = 0,
    TRIGGER_TYPE_TIMER = 1,
    TRIGGER_TYPE_QUEUE = 2,
    TRIGGER_TYPE_STREAM = 3,
    TRIGGER_TYPE_EVENT = 4
} TriggerType;

typedef enum {
    INVOCATION_TYPE_SYNC = 0,
    INVOCATION_TYPE_ASYNC = 1,
    INVOCATION_TYPE_STREAM = 2
} InvocationType;

typedef struct {
    char key[128];
    char value[512];
} FunctionEnvVar;

typedef struct {
    TriggerType type;
    char config[512];
    bool enabled;
} FunctionTrigger;

typedef struct {
    char name[MAX_FUNCTION_NAME];
    char id[64];
    
    FunctionRuntime runtime;
    char handler[256];
    char code_path[512];
    
    FunctionEnvVar *env_vars;
    int env_count;
    
    FunctionTrigger triggers[MAX_TRIGGERS];
    int trigger_count;
    
    uint64_t memory_mb;
    int timeout_seconds;
    int max_concurrency;
    
    bool enable_cold_start_optimization;
    int warm_pool_size;
    int cold_start_timeout_ms;
    
    time_t created_at;
    time_t updated_at;
} FunctionDefinition;

typedef struct {
    char function_id[64];
    char instance_id[64];
    
    FunctionState state;
    void *runtime_context;
    
    time_t created_at;
    time_t last_used;
    time_t expires_at;
    
    uint64_t invocation_count;
    uint64_t total_execution_time_ms;
} FunctionInstance;

typedef struct {
    char invocation_id[64];
    char function_id[64];
    char instance_id[64];
    
    InvocationType type;
    
    void *input_data;
    size_t input_size;
    
    void *output_data;
    size_t output_size;
    
    int status_code;
    char error_message[512];
    
    time_t started_at;
    time_t completed_at;
    uint64_t duration_ms;
    
    uint64_t memory_used_mb;
    uint64_t cpu_time_ms;
} FunctionInvocation;

typedef struct {
    int min_instances;
    int max_instances;
    int target_concurrency;
    
    int scale_up_threshold;
    int scale_down_threshold;
    int scale_down_delay_seconds;
    
    bool enable_predictive_scaling;
} FunctionScalingPolicy;

typedef struct {
    uint64_t total_invocations;
    uint64_t successful_invocations;
    uint64_t failed_invocations;
    uint64_t cold_starts;
    uint64_t warm_starts;
    
    double average_duration_ms;
    double average_cold_start_ms;
    double p50_duration_ms;
    double p95_duration_ms;
    double p99_duration_ms;
    
    uint64_t total_memory_used_mb;
    uint64_t total_cpu_time_ms;
} FunctionMetrics;

typedef struct {
    int total_functions;
    int active_functions;
    int total_instances;
    int warm_instances;
    int cold_instances;
    
    uint64_t total_invocations;
    uint64_t invocations_per_second;
    double average_cold_start_ms;
    
    uint64_t total_memory_allocated_mb;
    uint64_t total_cpu_time_ms;
} ServerlessStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int serverless_init(void);
int serverless_shutdown(void);
bool serverless_is_running(void);

/*
 * ============================================================================
 * FUNCTION MANAGEMENT
 * ============================================================================
 */

FunctionDefinition *serverless_create_function(
    const char *name,
    FunctionRuntime runtime,
    const char *handler,
    const char *code_path
);

int serverless_update_function(const char *function_id, FunctionDefinition *definition);
int serverless_delete_function(const char *function_id);
FunctionDefinition *serverless_get_function(const char *function_id);
FunctionDefinition **serverless_list_functions(int *count);

int serverless_deploy_function(const char *function_id);
int serverless_undeploy_function(const char *function_id);

/*
 * ============================================================================
 * FUNCTION INVOCATION
 * ============================================================================
 */

FunctionInvocation *serverless_invoke(
    const char *function_id,
    InvocationType type,
    const void *input_data,
    size_t input_size
);

FunctionInvocation *serverless_invoke_async(
    const char *function_id,
    const void *input_data,
    size_t input_size
);

FunctionInvocation *serverless_invoke_sync(
    const char *function_id,
    const void *input_data,
    size_t input_size,
    int timeout_ms
);

FunctionInvocation *serverless_get_invocation(const char *invocation_id);
int serverless_cancel_invocation(const char *invocation_id);

/*
 * ============================================================================
 * INSTANCE MANAGEMENT
 * ============================================================================
 */

FunctionInstance *serverless_get_warm_instance(const char *function_id);
FunctionInstance *serverless_create_instance(const char *function_id);
int serverless_destroy_instance(const char *instance_id);

int serverless_warm_pool_maintain(const char *function_id);
int serverless_warm_pool_scale(const char *function_id, int target_size);
int serverless_cold_start_optimize(const char *function_id);

/*
 * ============================================================================
 * TRIGGERS
 * ============================================================================
 */

int serverless_add_trigger(
    const char *function_id,
    TriggerType type,
    const char *config
);

int serverless_remove_trigger(const char *function_id, int trigger_index);
int serverless_enable_trigger(const char *function_id, int trigger_index);
int serverless_disable_trigger(const char *function_id, int trigger_index);

int serverless_trigger_http(const char *function_id, const char *path, const char *method);
int serverless_trigger_timer(const char *function_id, const char *cron_expression);
int serverless_trigger_queue(const char *function_id, const char *queue_name);
int serverless_trigger_event(const char *function_id, const char *event_type);

/*
 * ============================================================================
 * SCALING
 * ============================================================================
 */

int serverless_set_scaling_policy(
    const char *function_id,
    FunctionScalingPolicy *policy
);

FunctionScalingPolicy *serverless_get_scaling_policy(const char *function_id);

int serverless_scale_function(const char *function_id, int target_instances);
int serverless_auto_scale_function(const char *function_id);

/*
 * ============================================================================
 * COLD START OPTIMIZATION
 * ============================================================================
 */

int serverless_enable_cold_start_optimization(const char *function_id);
int serverless_disable_cold_start_optimization(const char *function_id);

int serverless_prewarm_function(const char *function_id, int instance_count);
int serverless_snapshot_function(const char *function_id);
int serverless_restore_from_snapshot(const char *function_id);

double serverless_get_cold_start_time(const char *function_id);
double serverless_get_warm_start_time(const char *function_id);

/*
 * ============================================================================
 * METRICS & MONITORING
 * ============================================================================
 */

FunctionMetrics serverless_get_function_metrics(const char *function_id);
ServerlessStats serverless_get_stats(void);

int serverless_record_invocation(FunctionInvocation *invocation);
int serverless_record_cold_start(const char *function_id, uint64_t duration_ms);

/*
 * ============================================================================
 * BILLING & COST
 * ============================================================================
 */

typedef struct {
    uint64_t invocation_count;
    uint64_t compute_time_ms;
    uint64_t memory_gb_seconds;
    double estimated_cost_usd;
} FunctionBilling;

FunctionBilling serverless_get_billing(const char *function_id, time_t start, time_t end);
double serverless_calculate_cost(uint64_t invocations, uint64_t compute_ms, uint64_t memory_mb);

/*
 * ============================================================================
 * ENVIRONMENT & CONFIGURATION
 * ============================================================================
 */

int serverless_set_env_var(const char *function_id, const char *key, const char *value);
int serverless_remove_env_var(const char *function_id, const char *key);
const char *serverless_get_env_var(const char *function_id, const char *key);

int serverless_set_memory(const char *function_id, uint64_t memory_mb);
int serverless_set_timeout(const char *function_id, int timeout_seconds);
int serverless_set_concurrency(const char *function_id, int max_concurrency);

/*
 * ============================================================================
 * LOGGING & DEBUGGING
 * ============================================================================
 */

int serverless_get_logs(
    const char *function_id,
    time_t start_time,
    time_t end_time,
    char **logs,
    int *log_count
);

int serverless_stream_logs(const char *function_id, void (*callback)(const char *log));

int serverless_enable_debug_mode(const char *function_id);
int serverless_disable_debug_mode(const char *function_id);

/*
 * ============================================================================
 * VERSIONING
 * ============================================================================
 */

int serverless_create_version(const char *function_id, const char *version_name);
int serverless_delete_version(const char *function_id, const char *version_name);
int serverless_rollback_version(const char *function_id, const char *version_name);

int serverless_create_alias(const char *function_id, const char *alias_name, const char *version);
int serverless_update_alias(const char *function_id, const char *alias_name, const char *version);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

void serverless_generate_invocation_id(char *id, size_t size);
void serverless_generate_function_id(char *id, size_t size);
void serverless_generate_instance_id(char *id, size_t size);

int serverless_execute_function(
    FunctionInstance *instance,
    const void *input,
    size_t input_size,
    void **output,
    size_t *output_size
);

int serverless_cleanup_expired_instances(void);
int serverless_reap_cold_instances(void);

#endif // CONSTELLATION_SERVERLESS_H
