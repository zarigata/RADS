/*
 * RADS Constellation - Alerting Engine Implementation
 * Phase 7: Monitoring & Observability
 */

#define _GNU_SOURCE

#include "alerts.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static AlertRule **g_rules = NULL;
static int g_rule_count = 0;
static NotificationChannel **g_channels = NULL;
static int g_channel_count = 0;
static AlertEvent **g_events = NULL;
static int g_event_count = 0;
static int g_max_events = 1000;
static AlertConfig g_config;
static pthread_mutex_t g_alerts_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_evaluation_thread;
static bool g_initialized = false;
static bool g_running = false;
static uint64_t g_total_fires = 0;
static uint64_t g_total_resolves = 0;
static uint64_t g_total_notifications = 0;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static const char *severity_to_string(AlertSeverity severity) {
    switch (severity) {
        case ALERT_SEVERITY_INFO: return "INFO";
        case ALERT_SEVERITY_WARNING: return "WARNING";
        case ALERT_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

static const char *state_to_string(AlertState state) {
    switch (state) {
        case ALERT_STATE_OK: return "OK";
        case ALERT_STATE_PENDING: return "PENDING";
        case ALERT_STATE_FIRING: return "FIRING";
        case ALERT_STATE_RESOLVED: return "RESOLVED";
        default: return "UNKNOWN";
    }
}

static void add_event(AlertRule *rule, AlertState state, double metric_value) {
    pthread_mutex_lock(&g_alerts_mutex);
    
    if (g_event_count >= g_max_events) {
        free(g_events[0]);
        memmove(&g_events[0], &g_events[1], (g_max_events - 1) * sizeof(AlertEvent *));
        g_event_count = g_max_events - 1;
    }
    
    AlertEvent *event = calloc(1, sizeof(AlertEvent));
    if (event) {
        strncpy(event->rule_name, rule->name, MAX_ALERT_NAME - 1);
        event->state = state;
        event->severity = rule->severity;
        strncpy(event->message, rule->message, MAX_ALERT_MESSAGE - 1);
        event->timestamp = time(NULL);
        event->metric_value = metric_value;
        
        if (!g_events) {
            g_events = malloc(g_max_events * sizeof(AlertEvent *));
        }
        g_events[g_event_count++] = event;
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int alerts_init(AlertConfig *config) {
    pthread_mutex_lock(&g_alerts_mutex);
    
    if (g_initialized) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return 1;
    }
    
    if (config) {
        g_config = *config;
    } else {
        g_config.max_rules = MAX_ALERT_RULES;
        g_config.evaluation_interval_ms = 10000;
        g_config.default_breach_threshold = 3;
        g_config.enable_notifications = true;
    }
    
    g_rules = NULL;
    g_rule_count = 0;
    g_channels = NULL;
    g_channel_count = 0;
    g_events = NULL;
    g_event_count = 0;
    g_initialized = true;
    g_running = true;
    g_total_fires = 0;
    g_total_resolves = 0;
    g_total_notifications = 0;
    
    pthread_create(&g_evaluation_thread, NULL, alerts_evaluation_thread, NULL);
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return 0;
}

int alerts_shutdown(void) {
    pthread_mutex_lock(&g_alerts_mutex);
    
    if (!g_initialized) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return -1;
    }
    
    g_running = false;
    g_initialized = false;
    
    pthread_mutex_unlock(&g_alerts_mutex);
    
    pthread_join(g_evaluation_thread, NULL);
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        free(g_rules[i]);
    }
    free(g_rules);
    
    for (int i = 0; i < g_channel_count; i++) {
        free(g_channels[i]);
    }
    free(g_channels);
    
    for (int i = 0; i < g_event_count; i++) {
        free(g_events[i]);
    }
    free(g_events);
    
    g_rules = NULL;
    g_rule_count = 0;
    g_channels = NULL;
    g_channel_count = 0;
    g_events = NULL;
    g_event_count = 0;
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return 0;
}

bool alerts_is_running(void) {
    return g_running;
}

/*
 * ============================================================================
 * RULE MANAGEMENT
 * ============================================================================
 */

AlertRule *alerts_create_rule(
    const char *name,
    const char *metric_name,
    const char *condition,
    double threshold,
    int duration_seconds,
    AlertSeverity severity
) {
    if (!g_initialized || !name || !metric_name || !condition) return NULL;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, name) == 0) {
            pthread_mutex_unlock(&g_alerts_mutex);
            return g_rules[i];
        }
    }
    
    AlertRule *rule = calloc(1, sizeof(AlertRule));
    if (!rule) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return NULL;
    }
    
    strncpy(rule->name, name, MAX_ALERT_NAME - 1);
    strncpy(rule->condition.metric_name, metric_name, MAX_ALERT_NAME - 1);
    strncpy(rule->condition.condition, condition, MAX_ALERT_CONDITION - 1);
    rule->condition.threshold = threshold;
    rule->condition.duration_seconds = duration_seconds;
    rule->severity = severity;
    rule->state = ALERT_STATE_OK;
    rule->enabled = true;
    rule->evaluation_interval_seconds = 10;
    rule->breach_threshold = g_config.default_breach_threshold;
    rule->breach_count = 0;
    rule->channel_count = 0;
    rule->fire_count = 0;
    rule->resolve_count = 0;
    
    snprintf(rule->message, MAX_ALERT_MESSAGE, "Alert: %s condition triggered", name);
    
    g_rules = realloc(g_rules, (g_rule_count + 1) * sizeof(AlertRule *));
    g_rules[g_rule_count++] = rule;
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return rule;
}

int alerts_delete_rule(const char *name) {
    if (!g_initialized || !name) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, name) == 0) {
            free(g_rules[i]);
            
            for (int j = i; j < g_rule_count - 1; j++) {
                g_rules[j] = g_rules[j + 1];
            }
            g_rule_count--;
            
            pthread_mutex_unlock(&g_alerts_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return -1;
}

AlertRule *alerts_get_rule(const char *name) {
    if (!g_initialized || !name) return NULL;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, name) == 0) {
            AlertRule *rule = g_rules[i];
            pthread_mutex_unlock(&g_alerts_mutex);
            return rule;
        }
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return NULL;
}

int alerts_set_rule_enabled(const char *name, bool enabled) {
    if (!g_initialized || !name) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, name) == 0) {
            g_rules[i]->enabled = enabled;
            pthread_mutex_unlock(&g_alerts_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return -1;
}

int alerts_set_rule_message(const char *name, const char *message) {
    if (!g_initialized || !name || !message) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, name) == 0) {
            strncpy(g_rules[i]->message, message, MAX_ALERT_MESSAGE - 1);
            pthread_mutex_unlock(&g_alerts_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return -1;
}

/*
 * ============================================================================
 * NOTIFICATION CHANNELS
 * ============================================================================
 */

NotificationChannel *alerts_create_channel(const char *name, ChannelType type, const char *endpoint) {
    if (!g_initialized || !name) return NULL;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_channel_count; i++) {
        if (strcmp(g_channels[i]->name, name) == 0) {
            pthread_mutex_unlock(&g_alerts_mutex);
            return g_channels[i];
        }
    }
    
    NotificationChannel *channel = calloc(1, sizeof(NotificationChannel));
    if (!channel) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return NULL;
    }
    
    strncpy(channel->name, name, MAX_ALERT_NAME - 1);
    channel->type = type;
    if (endpoint) {
        strncpy(channel->endpoint, endpoint, 255);
    }
    channel->enabled = true;
    
    g_channels = realloc(g_channels, (g_channel_count + 1) * sizeof(NotificationChannel *));
    g_channels[g_channel_count++] = channel;
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return channel;
}

int alerts_delete_channel(const char *name) {
    if (!g_initialized || !name) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_channel_count; i++) {
        if (strcmp(g_channels[i]->name, name) == 0) {
            free(g_channels[i]);
            
            for (int j = i; j < g_channel_count - 1; j++) {
                g_channels[j] = g_channels[j + 1];
            }
            g_channel_count--;
            
            pthread_mutex_unlock(&g_alerts_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return -1;
}

int alerts_add_channel_to_rule(const char *rule_name, const char *channel_name) {
    if (!g_initialized || !rule_name || !channel_name) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    AlertRule *rule = NULL;
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, rule_name) == 0) {
            rule = g_rules[i];
            break;
        }
    }
    
    NotificationChannel *channel = NULL;
    for (int i = 0; i < g_channel_count; i++) {
        if (strcmp(g_channels[i]->name, channel_name) == 0) {
            channel = g_channels[i];
            break;
        }
    }
    
    if (!rule || !channel) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return -1;
    }
    
    if (rule->channel_count >= MAX_NOTIFICATION_CHANNELS) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return -1;
    }
    
    rule->channels[rule->channel_count++] = channel;
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return 0;
}

/*
 * ============================================================================
 * EVALUATION
 * ============================================================================
 */

bool alerts_check_condition(AlertRule *rule, double metric_value) {
    const char *cond = rule->condition.condition;
    double threshold = rule->condition.threshold;
    
    if (strstr(cond, ">")) {
        return metric_value > threshold;
    } else if (strstr(cond, "<")) {
        return metric_value < threshold;
    } else if (strstr(cond, "==")) {
        return metric_value == threshold;
    } else if (strstr(cond, ">=")) {
        return metric_value >= threshold;
    } else if (strstr(cond, "<=")) {
        return metric_value <= threshold;
    }
    
    return false;
}

int alerts_evaluate_rule(const char *rule_name) {
    if (!g_initialized || !rule_name) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    AlertRule *rule = NULL;
    for (int i = 0; i < g_rule_count; i++) {
        if (strcmp(g_rules[i]->name, rule_name) == 0) {
            rule = g_rules[i];
            break;
        }
    }
    
    if (!rule || !rule->enabled) {
        pthread_mutex_unlock(&g_alerts_mutex);
        return -1;
    }
    
    rule->last_evaluation_time = time(NULL);
    
    double metric_value = 75.0;
    
    bool condition_met = alerts_check_condition(rule, metric_value);
    
    if (condition_met) {
        rule->breach_count++;
        if (rule->breach_count == 1) {
            rule->first_breach_time = time(NULL);
        }
        
        if (rule->breach_count >= rule->breach_threshold && rule->state != ALERT_STATE_FIRING) {
            rule->state = ALERT_STATE_FIRING;
            rule->fired_at = time(NULL);
            rule->fire_count++;
            g_total_fires++;
            
            add_event(rule, ALERT_STATE_FIRING, metric_value);
            alerts_notify_fire(rule);
        }
    } else {
        if (rule->state == ALERT_STATE_FIRING) {
            rule->state = ALERT_STATE_RESOLVED;
            rule->resolved_at = time(NULL);
            rule->resolve_count++;
            g_total_resolves++;
            
            add_event(rule, ALERT_STATE_RESOLVED, metric_value);
            alerts_notify_resolve(rule);
        }
        
        rule->breach_count = 0;
        rule->first_breach_time = 0;
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return 0;
}

int alerts_evaluate_all(void) {
    if (!g_initialized) return -1;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i]->enabled) {
            pthread_mutex_unlock(&g_alerts_mutex);
            alerts_evaluate_rule(g_rules[i]->name);
            pthread_mutex_lock(&g_alerts_mutex);
        }
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return 0;
}

/*
 * ============================================================================
 * NOTIFICATIONS
 * ============================================================================
 */

int alerts_send_notification(AlertRule *rule, AlertEvent *event) {
    if (!g_config.enable_notifications) return 0;
    
    for (int i = 0; i < rule->channel_count; i++) {
        NotificationChannel *channel = rule->channels[i];
        if (!channel->enabled) continue;
        
        g_total_notifications++;
    }
    
    return 0;
}

int alerts_notify_fire(AlertRule *rule) {
    if (!rule) return -1;
    
    AlertEvent event = {0};
    strncpy(event.rule_name, rule->name, MAX_ALERT_NAME - 1);
    event.state = ALERT_STATE_FIRING;
    event.severity = rule->severity;
    strncpy(event.message, rule->message, MAX_ALERT_MESSAGE - 1);
    event.timestamp = time(NULL);
    
    return alerts_send_notification(rule, &event);
}

int alerts_notify_resolve(AlertRule *rule) {
    if (!rule) return -1;
    
    AlertEvent event = {0};
    strncpy(event.rule_name, rule->name, MAX_ALERT_NAME - 1);
    event.state = ALERT_STATE_RESOLVED;
    event.severity = rule->severity;
    snprintf(event.message, MAX_ALERT_MESSAGE, "Alert resolved: %s", rule->name);
    event.timestamp = time(NULL);
    
    return alerts_send_notification(rule, &event);
}

/*
 * ============================================================================
 * EVENT HISTORY
 * ============================================================================
 */

AlertEvent **alerts_get_events(int *count, int limit) {
    if (!g_initialized || !count) return NULL;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    int result_count = g_event_count;
    if (limit > 0 && result_count > limit) {
        result_count = limit;
    }
    
    AlertEvent **results = malloc(sizeof(AlertEvent *) * result_count);
    if (results) {
        int start = g_event_count - result_count;
        for (int i = 0; i < result_count; i++) {
            results[i] = g_events[start + i];
        }
        *count = result_count;
    }
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return results;
}

AlertEvent **alerts_get_firing_alerts(int *count) {
    if (!g_initialized || !count) return NULL;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    int firing_count = 0;
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i]->state == ALERT_STATE_FIRING) {
            firing_count++;
        }
    }
    
    if (firing_count == 0) {
        *count = 0;
        pthread_mutex_unlock(&g_alerts_mutex);
        return NULL;
    }
    
    AlertEvent **results = malloc(sizeof(AlertEvent *) * firing_count);
    int idx = 0;
    
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i]->state == ALERT_STATE_FIRING) {
            AlertEvent *event = calloc(1, sizeof(AlertEvent));
            strncpy(event->rule_name, g_rules[i]->name, MAX_ALERT_NAME - 1);
            event->state = g_rules[i]->state;
            event->severity = g_rules[i]->severity;
            strncpy(event->message, g_rules[i]->message, MAX_ALERT_MESSAGE - 1);
            event->timestamp = g_rules[i]->fired_at;
            results[idx++] = event;
        }
    }
    
    *count = firing_count;
    pthread_mutex_unlock(&g_alerts_mutex);
    return results;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

AlertStats alerts_get_stats(void) {
    AlertStats stats = {0};
    
    if (!g_initialized) return stats;
    
    pthread_mutex_lock(&g_alerts_mutex);
    
    stats.total_rules = g_rule_count;
    
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i]->enabled) {
            stats.active_rules++;
        }
        if (g_rules[i]->state == ALERT_STATE_FIRING) {
            stats.firing_alerts++;
        }
    }
    
    stats.total_fires = g_total_fires;
    stats.total_resolves = g_total_resolves;
    stats.total_notifications = g_total_notifications;
    
    pthread_mutex_unlock(&g_alerts_mutex);
    return stats;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *alerts_evaluation_thread(void *arg) {
    (void)arg;
    
    while (g_running) {
        for (int i = 0; i < g_config.evaluation_interval_ms / 100 && g_running; i++) {
            usleep(100000);
        }
        
        if (!g_running) continue;
        
        alerts_evaluate_all();
    }
    
    return NULL;
}
