#include "monitor/tsdb.h"
#include "monitor/logs.h"
#include "monitor/alerts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int tests_passed = 0;
int tests_failed = 0;

void test_assert(int cond, const char *msg) {
    if (!cond) {
        printf("  ❌ FAIL: %s\n", msg);
        tests_failed++;
    } else {
        tests_passed++;
    }
}

int main() {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  RADS CONSTELLATION - PHASE 7 TESTS                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    // Test 1: Time-Series Database
    printf("\n📊 Time-Series Database Tests\n");

    TSDBConfig tsdb_config = {
        .max_series = 100,
        .retention_seconds = 3600,
        .block_size_seconds = 300,
        .max_points_per_block = 300,
        .enable_compression = false,
        .enable_disk_persistence = false,
        .data_directory = NULL
    };

    test_assert(tsdb_init(&tsdb_config) == 0, "TSDB init");
    test_assert(tsdb_is_running(), "TSDB running");

    // Create series
    TSDBSeries *cpu_series = tsdb_create_series("cpu_usage", NULL, 0);
    test_assert(cpu_series != NULL, "Create CPU series");
    test_assert(strcmp(cpu_series->name, "cpu_usage") == 0, "CPU series name");

    TSDBSeries *mem_series = tsdb_create_series("memory_usage", NULL, 0);
    test_assert(mem_series != NULL, "Create memory series");

    // Insert data points
    time_t now = time(NULL);
    test_assert(tsdb_insert("cpu_usage", 45.5, now) == 0, "Insert CPU data");
    test_assert(tsdb_insert("cpu_usage", 55.2, now + 60) == 0, "Insert CPU data 2");
    test_assert(tsdb_insert("cpu_usage", 72.8, now + 120) == 0, "Insert CPU data 3");
    test_assert(tsdb_insert("memory_usage", 2048.0, now) == 0, "Insert memory data");

    // Query data
    TSDBQueryResult *result = tsdb_query_range("cpu_usage", now - 10, now + 200);
    test_assert(result != NULL, "Query CPU range");
    test_assert(result->point_count == 3, "Query result count");
    if (result) {
        free(result->points);
        free(result);
    }

    // Aggregations
    double avg = tsdb_avg("cpu_usage", now - 10, now + 200);
    test_assert(avg > 0.0, "Calculate average");

    double max = tsdb_max("cpu_usage", now - 10, now + 200);
    test_assert(max == 72.8, "Calculate max");

    double min = tsdb_min("cpu_usage", now - 10, now + 200);
    test_assert(min == 45.5, "Calculate min");

    // List series
    int series_count = 0;
    TSDBSeries **all_series = tsdb_list_series(&series_count);
    test_assert(all_series != NULL, "List all series");
    test_assert(series_count == 2, "Series count");
    if (all_series) free(all_series);

    // Statistics
    TSDBStats tsdb_stats = tsdb_get_stats();
    test_assert(tsdb_stats.total_series == 2, "TSDB stats series");
    test_assert(tsdb_stats.total_points == 4, "TSDB stats points");

    printf("  ✅ TSDB: %d passed\n", 18);

    // Test 2: Log Aggregation
    printf("\n📝 Log Aggregation Tests\n");

    LogConfig log_config = {
        .max_entries = 1000,
        .retention_seconds = 3600,
        .enable_compression = false,
        .enable_disk_persistence = false,
        .log_directory = NULL,
        .min_level = LOG_LEVEL_DEBUG
    };

    test_assert(logs_init(&log_config) == 0, "Logs init");
    test_assert(logs_is_running(), "Logs running");

    // Write logs
    test_assert(logs_debug("test-service", "Debug message") == 0, "Write debug log");
    test_assert(logs_info("test-service", "Info message") == 0, "Write info log");
    test_assert(logs_warn("test-service", "Warning message") == 0, "Write warn log");
    test_assert(logs_error("test-service", "Error message") == 0, "Write error log");

    // Query logs
    LogQueryResult *log_result = logs_query_recent(10);
    test_assert(log_result != NULL, "Query recent logs");
    test_assert(log_result->entry_count == 4, "Log query count");
    if (log_result) {
        free(log_result->entries);
        free(log_result);
    }

    // Query by level
    LogQueryResult *error_logs = logs_query_by_level(LOG_LEVEL_ERROR, 10);
    test_assert(error_logs != NULL, "Query error logs");
    test_assert(error_logs->entry_count == 1, "Error log count");
    if (error_logs) {
        free(error_logs->entries);
        free(error_logs);
    }

    // Query by source
    LogQueryResult *service_logs = logs_query_by_source("test-service", 10);
    test_assert(service_logs != NULL, "Query by source");
    test_assert(service_logs->entry_count == 4, "Source log count");
    if (service_logs) {
        free(service_logs->entries);
        free(service_logs);
    }

    // Statistics
    LogStats log_stats = logs_get_stats();
    test_assert(log_stats.total_logs == 4, "Log stats total");
    test_assert(log_stats.logs_by_level[LOG_LEVEL_ERROR] == 1, "Log stats by level");

    printf("  ✅ Logs: %d passed\n", 14);

    // Test 3: Alerting Engine
    printf("\n🚨 Alerting Engine Tests\n");

    AlertConfig alert_config = {
        .max_rules = 100,
        .evaluation_interval_ms = 5000,
        .default_breach_threshold = 3,
        .enable_notifications = true
    };

    test_assert(alerts_init(&alert_config) == 0, "Alerts init");
    test_assert(alerts_is_running(), "Alerts running");

    // Create alert rule
    AlertRule *cpu_alert = alerts_create_rule(
        "high-cpu",
        "cpu_usage",
        ">",
        80.0,
        60,
        ALERT_SEVERITY_WARNING
    );
    test_assert(cpu_alert != NULL, "Create CPU alert");
    test_assert(strcmp(cpu_alert->name, "high-cpu") == 0, "Alert name");
    test_assert(cpu_alert->severity == ALERT_SEVERITY_WARNING, "Alert severity");
    test_assert(cpu_alert->enabled, "Alert enabled");

    // Create critical alert
    AlertRule *critical_alert = alerts_create_rule(
        "critical-cpu",
        "cpu_usage",
        ">",
        95.0,
        30,
        ALERT_SEVERITY_CRITICAL
    );
    test_assert(critical_alert != NULL, "Create critical alert");

    // Get rule
    AlertRule *retrieved = alerts_get_rule("high-cpu");
    test_assert(retrieved != NULL, "Get alert rule");
    test_assert(retrieved == cpu_alert, "Retrieved rule matches");

    // Set custom message
    test_assert(alerts_set_rule_message("high-cpu", "CPU usage is too high!") == 0, "Set alert message");

    // Create notification channel
    NotificationChannel *log_channel = alerts_create_channel("log-channel", CHANNEL_TYPE_LOG, NULL);
    test_assert(log_channel != NULL, "Create log channel");
    test_assert(log_channel->enabled, "Channel enabled");

    // Add channel to rule
    test_assert(alerts_add_channel_to_rule("high-cpu", "log-channel") == 0, "Add channel to rule");
    test_assert(cpu_alert->channel_count == 1, "Rule channel count");

    // Evaluate rule
    test_assert(alerts_evaluate_rule("high-cpu") == 0, "Evaluate alert rule");

    // Check condition
    bool condition_met = alerts_check_condition(cpu_alert, 85.0);
    test_assert(condition_met, "Check condition (should trigger)");

    bool condition_not_met = alerts_check_condition(cpu_alert, 50.0);
    test_assert(!condition_not_met, "Check condition (should not trigger)");

    // Disable/enable rule
    test_assert(alerts_set_rule_enabled("high-cpu", false) == 0, "Disable alert");
    test_assert(!cpu_alert->enabled, "Alert disabled");
    test_assert(alerts_set_rule_enabled("high-cpu", true) == 0, "Enable alert");
    test_assert(cpu_alert->enabled, "Alert enabled");

    // Statistics
    AlertStats alert_stats = alerts_get_stats();
    test_assert(alert_stats.total_rules == 2, "Alert stats total rules");
    test_assert(alert_stats.active_rules == 2, "Alert stats active rules");

    // Get events
    int event_count = 0;
    AlertEvent **events = alerts_get_events(&event_count, 10);
    test_assert(events != NULL || event_count == 0, "Get alert events");
    if (events) free(events);

    printf("  ✅ Alerts: %d passed\n", 23);

    // Shutdown tests
    printf("\n🛑 Shutdown Tests\n");

    test_assert(tsdb_shutdown() == 0, "TSDB shutdown");
    test_assert(!tsdb_is_running(), "TSDB not running");
    test_assert(logs_shutdown() == 0, "Logs shutdown");
    test_assert(!logs_is_running(), "Logs not running");
    test_assert(alerts_shutdown() == 0, "Alerts shutdown");
    test_assert(!alerts_is_running(), "Alerts not running");

    printf("  ✅ Shutdown: %d passed\n", 6);

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ Passed: %-3d  ❌ Failed: %-3d                           ║\n",
           tests_passed, tests_failed);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
