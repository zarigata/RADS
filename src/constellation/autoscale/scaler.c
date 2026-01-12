/*
 * RADS Constellation - Auto-Scaler Implementation
 * Phase 6: Auto-Scaling
 */

#define _GNU_SOURCE

#include "scaler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static ScalingPolicy **g_policies = NULL;
static int g_policy_count = 0;
static ScalingEvent **g_events = NULL;
static int g_event_count = 0;
static int g_max_events = 1000;
static ScalerConfig g_config;
static pthread_mutex_t g_scaler_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_evaluation_thread;
static bool g_initialized = false;
static bool g_running = false;
static uint64_t g_total_evaluations = 0;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static bool evaluate_comparison(double value, ComparisonOperator op, double threshold) {
    switch (op) {
        case OPERATOR_GREATER:
            return value > threshold;
        case OPERATOR_LESS:
            return value < threshold;
        case OPERATOR_EQUAL:
            return fabs(value - threshold) < 0.001;
        case OPERATOR_GREATER_EQUAL:
            return value >= threshold;
        case OPERATOR_LESS_EQUAL:
            return value <= threshold;
        default:
            return false;
    }
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int scaler_init(ScalerConfig *config) {
    pthread_mutex_lock(&g_scaler_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_scaler_mutex);
        return 1;
    }

    // Set configuration
    if (config) {
        g_config = *config;
    } else {
        g_config.max_policies = 100;
        g_config.evaluation_interval_ms = 10000;  // 10 seconds
        g_config.default_cooldown_seconds = 300;  // 5 minutes
        g_config.enable_predictive = true;
        g_config.prediction_confidence_threshold = 0.8;
    }

    g_policies = NULL;
    g_policy_count = 0;
    g_events = NULL;
    g_event_count = 0;
    g_initialized = true;
    g_running = true;
    g_total_evaluations = 0;

    // Start evaluation thread
    pthread_create(&g_evaluation_thread, NULL, scaler_evaluation_thread, NULL);

    pthread_mutex_unlock(&g_scaler_mutex);
    return 0;
}

int scaler_shutdown(void) {
    pthread_mutex_lock(&g_scaler_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_scaler_mutex);
        return -1;
    }

    g_running = false;
    g_initialized = false;

    pthread_mutex_unlock(&g_scaler_mutex);

    // Wait for thread
    pthread_join(g_evaluation_thread, NULL);

    pthread_mutex_lock(&g_scaler_mutex);

    // Free all policies
    for (int i = 0; i < g_policy_count; i++) {
        free(g_policies[i]);
    }
    free(g_policies);

    // Free all events
    for (int i = 0; i < g_event_count; i++) {
        free(g_events[i]);
    }
    free(g_events);

    g_policies = NULL;
    g_policy_count = 0;
    g_events = NULL;
    g_event_count = 0;

    pthread_mutex_unlock(&g_scaler_mutex);
    return 0;
}

bool scaler_is_running(void) {
    pthread_mutex_lock(&g_scaler_mutex);
    bool running = g_initialized && g_running;
    pthread_mutex_unlock(&g_scaler_mutex);
    return running;
}

/*
 * ============================================================================
 * POLICY MANAGEMENT
 * ============================================================================
 */

ScalingPolicy *scaler_create_policy(
    const char *name,
    const char *target_service,
    int min_instances,
    int max_instances
) {
    if (!g_initialized || !name || !target_service) return NULL;

    pthread_mutex_lock(&g_scaler_mutex);

    // Check if already exists
    for (int i = 0; i < g_policy_count; i++) {
        if (strcmp(g_policies[i]->name, name) == 0) {
            pthread_mutex_unlock(&g_scaler_mutex);
            return g_policies[i];
        }
    }

    // Create new policy
    ScalingPolicy *policy = calloc(1, sizeof(ScalingPolicy));
    if (!policy) {
        pthread_mutex_unlock(&g_scaler_mutex);
        return NULL;
    }

    strncpy(policy->name, name, sizeof(policy->name) - 1);
    strncpy(policy->target_service, target_service, sizeof(policy->target_service) - 1);
    policy->min_instances = min_instances;
    policy->max_instances = max_instances;
    policy->current_instances = min_instances;
    policy->rule_count = 0;
    policy->scale_up_cooldown_seconds = g_config.default_cooldown_seconds;
    policy->scale_down_cooldown_seconds = g_config.default_cooldown_seconds;
    policy->last_scale_up = 0;
    policy->last_scale_down = 0;
    policy->scale_up_count = 0;
    policy->scale_down_count = 0;
    policy->total_evaluations = 0;
    policy->enabled = true;

    // Add to list
    g_policies = realloc(g_policies, sizeof(ScalingPolicy *) * (g_policy_count + 1));
    g_policies[g_policy_count] = policy;
    g_policy_count++;

    pthread_mutex_unlock(&g_scaler_mutex);
    return policy;
}

int scaler_destroy_policy(const char *name) {
    if (!g_initialized || !name) return -1;

    pthread_mutex_lock(&g_scaler_mutex);

    for (int i = 0; i < g_policy_count; i++) {
        if (strcmp(g_policies[i]->name, name) == 0) {
            free(g_policies[i]);

            for (int j = i; j < g_policy_count - 1; j++) {
                g_policies[j] = g_policies[j + 1];
            }

            g_policy_count--;

            pthread_mutex_unlock(&g_scaler_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_scaler_mutex);
    return -1;
}

ScalingPolicy *scaler_get_policy(const char *name) {
    if (!g_initialized || !name) return NULL;

    pthread_mutex_lock(&g_scaler_mutex);

    for (int i = 0; i < g_policy_count; i++) {
        if (strcmp(g_policies[i]->name, name) == 0) {
            ScalingPolicy *policy = g_policies[i];
            pthread_mutex_unlock(&g_scaler_mutex);
            return policy;
        }
    }

    pthread_mutex_unlock(&g_scaler_mutex);
    return NULL;
}

int scaler_set_policy_enabled(const char *name, bool enabled) {
    ScalingPolicy *policy = scaler_get_policy(name);
    if (!policy) return -1;

    pthread_mutex_lock(&g_scaler_mutex);
    policy->enabled = enabled;
    pthread_mutex_unlock(&g_scaler_mutex);

    return 0;
}

/*
 * ============================================================================
 * RULE MANAGEMENT
 * ============================================================================
 */

int scaler_add_rule(const char *policy_name, ScalingRule *rule) {
    if (!policy_name || !rule) return -1;

    ScalingPolicy *policy = scaler_get_policy(policy_name);
    if (!policy) return -1;

    pthread_mutex_lock(&g_scaler_mutex);

    if (policy->rule_count >= MAX_SCALING_RULES) {
        pthread_mutex_unlock(&g_scaler_mutex);
        return -1;
    }

    policy->rules[policy->rule_count] = *rule;
    policy->rule_count++;

    pthread_mutex_unlock(&g_scaler_mutex);
    return 0;
}

int scaler_remove_rule(const char *policy_name, int rule_index) {
    ScalingPolicy *policy = scaler_get_policy(policy_name);
    if (!policy || rule_index < 0 || rule_index >= policy->rule_count) return -1;

    pthread_mutex_lock(&g_scaler_mutex);

    for (int i = rule_index; i < policy->rule_count - 1; i++) {
        policy->rules[i] = policy->rules[i + 1];
    }

    policy->rule_count--;

    pthread_mutex_unlock(&g_scaler_mutex);
    return 0;
}

ScalingRule scaler_create_threshold_rule(
    const char *metric_name,
    ComparisonOperator op,
    double threshold,
    ScaleDirection direction,
    int adjustment
) {
    ScalingRule rule = {0};

    strncpy(rule.metric_name, metric_name, sizeof(rule.metric_name) - 1);
    rule.trigger_type = TRIGGER_TYPE_THRESHOLD;
    rule.operator = op;
    rule.threshold = threshold;
    rule.evaluation_period_seconds = 60;  // 1 minute
    rule.breach_count_required = 1;
    rule.direction = direction;
    rule.adjustment = adjustment;
    rule.enabled = true;

    return rule;
}

/*
 * ============================================================================
 * SCALING OPERATIONS
 * ============================================================================
 */

bool scaler_is_rule_triggered(ScalingRule *rule, Metric *metric) {
    if (!rule || !metric || !rule->enabled) return false;

    double value;

    switch (rule->trigger_type) {
        case TRIGGER_TYPE_THRESHOLD:
            value = metrics_get_average(rule->metric_name, rule->evaluation_period_seconds);
            return evaluate_comparison(value, rule->operator, rule->threshold);

        case TRIGGER_TYPE_RATE:
            value = metrics_get_rate(rule->metric_name, rule->evaluation_period_seconds);
            return evaluate_comparison(value, rule->operator, rule->threshold);

        case TRIGGER_TYPE_PREDICTIVE:
            // Predictive logic would go here
            return false;

        case TRIGGER_TYPE_SCHEDULE:
            // Schedule logic would go here
            return false;

        default:
            return false;
    }
}

int scaler_scale(
    ScalingPolicy *policy,
    ScaleDirection direction,
    int adjustment,
    const char *reason
) {
    if (!policy || direction == SCALE_DIRECTION_NONE) return -1;

    pthread_mutex_lock(&g_scaler_mutex);

    // Calculate new instance count
    int new_count = policy->current_instances;
    if (direction == SCALE_DIRECTION_UP) {
        new_count += adjustment;
    } else {
        new_count -= adjustment;
    }

    // Apply limits
    if (new_count < policy->min_instances) new_count = policy->min_instances;
    if (new_count > policy->max_instances) new_count = policy->max_instances;

    // Check if actually changing
    if (new_count == policy->current_instances) {
        pthread_mutex_unlock(&g_scaler_mutex);
        return 0;
    }

    // Create scaling event
    ScalingEvent *event = calloc(1, sizeof(ScalingEvent));
    if (event) {
        strncpy(event->policy_name, policy->name, sizeof(event->policy_name) - 1);
        strncpy(event->service_name, policy->target_service, sizeof(event->service_name) - 1);
        event->direction = direction;
        event->adjustment = adjustment;
        event->instances_before = policy->current_instances;
        event->instances_after = new_count;
        if (reason) {
            strncpy(event->reason, reason, sizeof(event->reason) - 1);
        }
        event->timestamp = time(NULL);

        // Add to event list
        if (g_event_count >= g_max_events) {
            // Remove oldest event
            free(g_events[0]);
            for (int i = 0; i < g_event_count - 1; i++) {
                g_events[i] = g_events[i + 1];
            }
            g_event_count--;
        }

        g_events = realloc(g_events, sizeof(ScalingEvent *) * (g_event_count + 1));
        g_events[g_event_count] = event;
        g_event_count++;
    }

    // Update policy
    policy->current_instances = new_count;

    if (direction == SCALE_DIRECTION_UP) {
        policy->last_scale_up = time(NULL);
        policy->scale_up_count++;
    } else {
        policy->last_scale_down = time(NULL);
        policy->scale_down_count++;
    }

    pthread_mutex_unlock(&g_scaler_mutex);

    // In a real implementation, this would trigger instance creation/deletion
    // via the scheduler/controller

    return 0;
}

int scaler_set_current_instances(const char *policy_name, int count) {
    ScalingPolicy *policy = scaler_get_policy(policy_name);
    if (!policy) return -1;

    pthread_mutex_lock(&g_scaler_mutex);
    policy->current_instances = count;
    pthread_mutex_unlock(&g_scaler_mutex);

    return 0;
}

int scaler_evaluate_policy(const char *policy_name) {
    ScalingPolicy *policy = scaler_get_policy(policy_name);
    if (!policy || !policy->enabled) return -1;

    pthread_mutex_lock(&g_scaler_mutex);
    policy->total_evaluations++;
    g_total_evaluations++;
    pthread_mutex_unlock(&g_scaler_mutex);

    // Evaluate each rule
    for (int i = 0; i < policy->rule_count; i++) {
        ScalingRule *rule = &policy->rules[i];
        if (!rule->enabled) continue;

        // Check cooldown
        if (scaler_in_cooldown(policy, rule->direction)) {
            continue;
        }

        // Get metric
        Metric *metric = metrics_get(rule->metric_name);
        if (!metric) continue;

        // Check if triggered
        if (scaler_is_rule_triggered(rule, metric)) {
            char reason[256];
            snprintf(reason, sizeof(reason), "Rule triggered: %s %s %.2f",
                     rule->metric_name,
                     rule->operator == OPERATOR_GREATER ? ">" :
                     rule->operator == OPERATOR_LESS ? "<" : "=",
                     rule->threshold);

            scaler_scale(policy, rule->direction, rule->adjustment, reason);
        }
    }

    return 0;
}

int scaler_evaluate_all(void) {
    if (!g_initialized) return -1;

    pthread_mutex_lock(&g_scaler_mutex);
    int count = g_policy_count;
    pthread_mutex_unlock(&g_scaler_mutex);

    for (int i = 0; i < count; i++) {
        pthread_mutex_lock(&g_scaler_mutex);
        if (i >= g_policy_count) {
            pthread_mutex_unlock(&g_scaler_mutex);
            break;
        }
        ScalingPolicy *policy = g_policies[i];
        pthread_mutex_unlock(&g_scaler_mutex);

        scaler_evaluate_policy(policy->name);
    }

    return 0;
}

/*
 * ============================================================================
 * COOLDOWN MANAGEMENT
 * ============================================================================
 */

bool scaler_in_cooldown(ScalingPolicy *policy, ScaleDirection direction) {
    if (!policy) return false;

    time_t now = time(NULL);
    time_t last_action;
    int cooldown_seconds;

    if (direction == SCALE_DIRECTION_UP) {
        last_action = policy->last_scale_up;
        cooldown_seconds = policy->scale_up_cooldown_seconds;
    } else {
        last_action = policy->last_scale_down;
        cooldown_seconds = policy->scale_down_cooldown_seconds;
    }

    if (last_action == 0) return false;

    return (now - last_action) < cooldown_seconds;
}

int scaler_get_cooldown_remaining(ScalingPolicy *policy, ScaleDirection direction) {
    if (!policy) return 0;

    if (!scaler_in_cooldown(policy, direction)) return 0;

    time_t now = time(NULL);
    time_t last_action;
    int cooldown_seconds;

    if (direction == SCALE_DIRECTION_UP) {
        last_action = policy->last_scale_up;
        cooldown_seconds = policy->scale_up_cooldown_seconds;
    } else {
        last_action = policy->last_scale_down;
        cooldown_seconds = policy->scale_down_cooldown_seconds;
    }

    int remaining = cooldown_seconds - (now - last_action);
    return remaining > 0 ? remaining : 0;
}

/*
 * ============================================================================
 * PREDICTIVE SCALING
 * ============================================================================
 */

double scaler_predict_metric(const char *metric_name, int seconds_ahead) {
    if (!metric_name || seconds_ahead <= 0) return 0.0;

    // Simple linear regression prediction
    Metric *metric = metrics_get(metric_name);
    if (!metric || metric->history_count < 2) return 0.0;

    pthread_mutex_lock(&g_scaler_mutex);

    // Calculate average rate of change
    double rate = metrics_get_rate(metric_name, 300);  // Last 5 minutes

    // Predict future value
    double current = metric->current_value;
    double predicted = current + (rate * seconds_ahead);

    pthread_mutex_unlock(&g_scaler_mutex);

    return predicted;
}

int scaler_evaluate_predictive(ScalingPolicy *policy) {
    if (!policy || !g_config.enable_predictive) return -1;

    // Predictive evaluation would go here
    // This would analyze trends and predict future resource needs

    return 0;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

ScalerStats scaler_get_stats(void) {
    ScalerStats stats = {0};

    if (!g_initialized) return stats;

    pthread_mutex_lock(&g_scaler_mutex);

    stats.total_policies = g_policy_count;
    stats.total_evaluations = g_total_evaluations;
    stats.pending_actions = 0;

    for (int i = 0; i < g_policy_count; i++) {
        if (g_policies[i]->enabled) {
            stats.active_policies++;
        }
        stats.total_scale_ups += g_policies[i]->scale_up_count;
        stats.total_scale_downs += g_policies[i]->scale_down_count;
    }

    pthread_mutex_unlock(&g_scaler_mutex);
    return stats;
}

ScalingEvent **scaler_get_events(int *count, int limit) {
    if (!g_initialized || !count) return NULL;

    pthread_mutex_lock(&g_scaler_mutex);

    int result_count = g_event_count;
    if (limit > 0 && result_count > limit) {
        result_count = limit;
    }

    ScalingEvent **results = malloc(sizeof(ScalingEvent *) * result_count);
    if (results) {
        // Return most recent events
        int start = g_event_count - result_count;
        for (int i = 0; i < result_count; i++) {
            results[i] = g_events[start + i];
        }
        *count = result_count;
    }

    pthread_mutex_unlock(&g_scaler_mutex);
    return results;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *scaler_evaluation_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks for quick exit
        for (int i = 0; i < g_config.evaluation_interval_ms / 100 && g_running; i++) {
            usleep(100000);  // 100ms
        }

        if (!g_running) continue;

        // Evaluate all policies
        scaler_evaluate_all();
    }

    return NULL;
}
