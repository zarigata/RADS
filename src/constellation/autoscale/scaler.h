/*
 * RADS Constellation - Auto-Scaler
 * Phase 6: Auto-Scaling
 */

#ifndef CONSTELLATION_SCALER_H
#define CONSTELLATION_SCALER_H

#include "metrics.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * AUTO-SCALING
 * ============================================================================
 */

#define MAX_SCALER_NAME 128
#define MAX_SCALING_RULES 16

// Scaling direction
typedef enum {
    SCALE_DIRECTION_UP = 1,
    SCALE_DIRECTION_DOWN = -1,
    SCALE_DIRECTION_NONE = 0
} ScaleDirection;

// Scaling trigger type
typedef enum {
    TRIGGER_TYPE_THRESHOLD = 0,      // Simple threshold (CPU > 80%)
    TRIGGER_TYPE_RATE = 1,           // Rate of change
    TRIGGER_TYPE_PREDICTIVE = 2,     // Predictive based on trends
    TRIGGER_TYPE_SCHEDULE = 3        // Time-based schedule
} TriggerType;

// Comparison operator
typedef enum {
    OPERATOR_GREATER = 0,
    OPERATOR_LESS = 1,
    OPERATOR_EQUAL = 2,
    OPERATOR_GREATER_EQUAL = 3,
    OPERATOR_LESS_EQUAL = 4
} ComparisonOperator;

// Scaling rule
typedef struct {
    char metric_name[MAX_METRIC_NAME];
    TriggerType trigger_type;
    ComparisonOperator operator;
    double threshold;
    int evaluation_period_seconds;
    int breach_count_required;
    ScaleDirection direction;
    int adjustment;  // Number of instances to add/remove
    bool enabled;
} ScalingRule;

// Scaling policy
typedef struct {
    char name[MAX_SCALER_NAME];
    char target_service[MAX_SCALER_NAME];

    // Limits
    int min_instances;
    int max_instances;
    int current_instances;

    // Rules
    ScalingRule rules[MAX_SCALING_RULES];
    int rule_count;

    // Cooldown
    int scale_up_cooldown_seconds;
    int scale_down_cooldown_seconds;
    time_t last_scale_up;
    time_t last_scale_down;

    // Statistics
    uint64_t scale_up_count;
    uint64_t scale_down_count;
    uint64_t total_evaluations;

    bool enabled;
} ScalingPolicy;

// Scaler configuration
typedef struct {
    int max_policies;
    int evaluation_interval_ms;
    int default_cooldown_seconds;
    bool enable_predictive;
    double prediction_confidence_threshold;
} ScalerConfig;

// Scaling event
typedef struct {
    char policy_name[MAX_SCALER_NAME];
    char service_name[MAX_SCALER_NAME];
    ScaleDirection direction;
    int adjustment;
    int instances_before;
    int instances_after;
    char reason[256];
    time_t timestamp;
} ScalingEvent;

// Scaler statistics
typedef struct {
    int total_policies;
    int active_policies;
    uint64_t total_scale_ups;
    uint64_t total_scale_downs;
    uint64_t total_evaluations;
    int pending_actions;
} ScalerStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

// Initialize auto-scaler
int scaler_init(ScalerConfig *config);

// Shutdown auto-scaler
int scaler_shutdown(void);

// Check if scaler is running
bool scaler_is_running(void);

/*
 * ============================================================================
 * POLICY MANAGEMENT
 * ============================================================================
 */

// Create scaling policy
ScalingPolicy *scaler_create_policy(
    const char *name,
    const char *target_service,
    int min_instances,
    int max_instances
);

// Destroy scaling policy
int scaler_destroy_policy(const char *name);

// Get policy by name
ScalingPolicy *scaler_get_policy(const char *name);

// Enable/disable policy
int scaler_set_policy_enabled(const char *name, bool enabled);

/*
 * ============================================================================
 * RULE MANAGEMENT
 * ============================================================================
 */

// Add scaling rule to policy
int scaler_add_rule(
    const char *policy_name,
    ScalingRule *rule
);

// Remove scaling rule
int scaler_remove_rule(
    const char *policy_name,
    int rule_index
);

// Create simple threshold rule
ScalingRule scaler_create_threshold_rule(
    const char *metric_name,
    ComparisonOperator op,
    double threshold,
    ScaleDirection direction,
    int adjustment
);

/*
 * ============================================================================
 * SCALING OPERATIONS
 * ============================================================================
 */

// Evaluate all policies
int scaler_evaluate_all(void);

// Evaluate specific policy
int scaler_evaluate_policy(const char *policy_name);

// Check if rule is triggered
bool scaler_is_rule_triggered(ScalingRule *rule, Metric *metric);

// Execute scaling action
int scaler_scale(
    ScalingPolicy *policy,
    ScaleDirection direction,
    int adjustment,
    const char *reason
);

// Set current instance count (for tracking)
int scaler_set_current_instances(const char *policy_name, int count);

/*
 * ============================================================================
 * COOLDOWN MANAGEMENT
 * ============================================================================
 */

// Check if in cooldown period
bool scaler_in_cooldown(ScalingPolicy *policy, ScaleDirection direction);

// Get remaining cooldown time
int scaler_get_cooldown_remaining(ScalingPolicy *policy, ScaleDirection direction);

/*
 * ============================================================================
 * PREDICTIVE SCALING
 * ============================================================================
 */

// Predict future metric value
double scaler_predict_metric(const char *metric_name, int seconds_ahead);

// Evaluate predictive scaling
int scaler_evaluate_predictive(ScalingPolicy *policy);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

// Get scaler statistics
ScalerStats scaler_get_stats(void);

// Get scaling events
ScalingEvent **scaler_get_events(int *count, int limit);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

// Evaluation thread
void *scaler_evaluation_thread(void *arg);

#endif // CONSTELLATION_SCALER_H
