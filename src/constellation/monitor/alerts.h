/*
 * RADS Constellation - Alerting Engine
 * Phase 7: Monitoring & Observability
 */

#ifndef CONSTELLATION_ALERTS_H
#define CONSTELLATION_ALERTS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * ALERTING ENGINE
 * ============================================================================
 */

#define MAX_ALERT_NAME 128
#define MAX_ALERT_MESSAGE 512
#define MAX_ALERT_CONDITION 256
#define MAX_ALERT_RULES 100
#define MAX_NOTIFICATION_CHANNELS 10

typedef enum {
    ALERT_SEVERITY_INFO = 0,
    ALERT_SEVERITY_WARNING = 1,
    ALERT_SEVERITY_CRITICAL = 2
} AlertSeverity;

typedef enum {
    ALERT_STATE_OK = 0,
    ALERT_STATE_PENDING = 1,
    ALERT_STATE_FIRING = 2,
    ALERT_STATE_RESOLVED = 3
} AlertState;

typedef enum {
    CHANNEL_TYPE_LOG = 0,
    CHANNEL_TYPE_EMAIL = 1,
    CHANNEL_TYPE_SLACK = 2,
    CHANNEL_TYPE_WEBHOOK = 3,
    CHANNEL_TYPE_PAGERDUTY = 4
} ChannelType;

typedef struct {
    char name[MAX_ALERT_NAME];
    ChannelType type;
    char endpoint[256];
    bool enabled;
} NotificationChannel;

typedef struct {
    char metric_name[MAX_ALERT_NAME];
    char condition[MAX_ALERT_CONDITION];
    double threshold;
    int duration_seconds;
} AlertCondition;

typedef struct {
    char name[MAX_ALERT_NAME];
    AlertCondition condition;
    AlertSeverity severity;
    AlertState state;
    
    char message[MAX_ALERT_MESSAGE];
    
    int evaluation_interval_seconds;
    int breach_count;
    int breach_threshold;
    
    time_t first_breach_time;
    time_t last_evaluation_time;
    time_t fired_at;
    time_t resolved_at;
    
    NotificationChannel *channels[MAX_NOTIFICATION_CHANNELS];
    int channel_count;
    
    bool enabled;
    uint64_t fire_count;
    uint64_t resolve_count;
} AlertRule;

typedef struct {
    char rule_name[MAX_ALERT_NAME];
    AlertState state;
    AlertSeverity severity;
    char message[MAX_ALERT_MESSAGE];
    time_t timestamp;
    double metric_value;
} AlertEvent;

typedef struct {
    int max_rules;
    int evaluation_interval_ms;
    int default_breach_threshold;
    bool enable_notifications;
} AlertConfig;

typedef struct {
    int total_rules;
    int active_rules;
    int firing_alerts;
    uint64_t total_fires;
    uint64_t total_resolves;
    uint64_t total_notifications;
} AlertStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int alerts_init(AlertConfig *config);
int alerts_shutdown(void);
bool alerts_is_running(void);

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
);

int alerts_delete_rule(const char *name);
AlertRule *alerts_get_rule(const char *name);
int alerts_set_rule_enabled(const char *name, bool enabled);
int alerts_set_rule_message(const char *name, const char *message);

/*
 * ============================================================================
 * NOTIFICATION CHANNELS
 * ============================================================================
 */

NotificationChannel *alerts_create_channel(const char *name, ChannelType type, const char *endpoint);
int alerts_delete_channel(const char *name);
int alerts_add_channel_to_rule(const char *rule_name, const char *channel_name);
int alerts_remove_channel_from_rule(const char *rule_name, const char *channel_name);

/*
 * ============================================================================
 * EVALUATION
 * ============================================================================
 */

int alerts_evaluate_all(void);
int alerts_evaluate_rule(const char *rule_name);
bool alerts_check_condition(AlertRule *rule, double metric_value);

/*
 * ============================================================================
 * NOTIFICATIONS
 * ============================================================================
 */

int alerts_send_notification(AlertRule *rule, AlertEvent *event);
int alerts_notify_fire(AlertRule *rule);
int alerts_notify_resolve(AlertRule *rule);

/*
 * ============================================================================
 * EVENT HISTORY
 * ============================================================================
 */

AlertEvent **alerts_get_events(int *count, int limit);
AlertEvent **alerts_get_firing_alerts(int *count);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

AlertStats alerts_get_stats(void);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *alerts_evaluation_thread(void *arg);

#endif // CONSTELLATION_ALERTS_H
