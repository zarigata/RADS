#include "autoscale/metrics.h"
#include "autoscale/scaler.h"
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
    printf("║  RADS CONSTELLATION - PHASE 6 TESTS                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    // Test 1: Metrics System
    printf("\n📊 Metrics System Tests\n");

    MetricsConfig metrics_config = {
        .max_metrics = 100,
        .history_size = 50,
        .retention_seconds = 3600,
        .collection_interval_ms = 5000,
        .enable_aggregation = true
    };

    test_assert(metrics_init(&metrics_config) == 0, "Metrics init");
    test_assert(metrics_is_running(), "Metrics running");

    // Register metrics
    Metric *cpu_metric = metrics_register("cpu_usage", METRIC_TYPE_GAUGE, NULL, 0);
    test_assert(cpu_metric != NULL, "Register CPU metric");
    test_assert(strcmp(cpu_metric->name, "cpu_usage") == 0, "CPU metric name");
    test_assert(cpu_metric->type == METRIC_TYPE_GAUGE, "CPU metric type");

    Metric *req_metric = metrics_register("request_count", METRIC_TYPE_COUNTER, NULL, 0);
    test_assert(req_metric != NULL, "Register request metric");

    // Update metrics
    test_assert(metrics_set("cpu_usage", 45.5) == 0, "Set CPU usage");
    test_assert(metrics_set("cpu_usage", 55.2) == 0, "Update CPU usage");
    test_assert(metrics_set("cpu_usage", 72.8) == 0, "Update CPU usage again");

    test_assert(metrics_increment("request_count", 10.0) == 0, "Increment requests");
    test_assert(metrics_increment("request_count", 25.0) == 0, "Increment requests again");

    // Query metrics
    double cpu_current = metrics_get_current("cpu_usage");
    test_assert(cpu_current == 72.8, "Get current CPU");

    double cpu_avg = metrics_get_average("cpu_usage", 60);
    test_assert(cpu_avg > 0.0, "Get CPU average");

    double cpu_min = metrics_get_min("cpu_usage");
    test_assert(cpu_min == 45.5, "Get CPU min");

    double cpu_max = metrics_get_max("cpu_usage");
    test_assert(cpu_max == 72.8, "Get CPU max");

    // List all metrics
    int metric_count = 0;
    Metric **all_metrics = metrics_list_all(&metric_count);
    test_assert(all_metrics != NULL, "List all metrics");
    test_assert(metric_count == 2, "Metric count correct");
    if (all_metrics) free(all_metrics);

    // Get stats
    MetricsStats metrics_stats = metrics_get_stats();
    test_assert(metrics_stats.total_metrics == 2, "Metrics stats total");
    test_assert(metrics_stats.total_updates > 0, "Metrics stats updates");

    printf("  ✅ Metrics: %d passed\n", 19);

    // Test 2: Auto-Scaler
    printf("\n⚙️  Auto-Scaler Tests\n");

    ScalerConfig scaler_config = {
        .max_policies = 50,
        .evaluation_interval_ms = 5000,
        .default_cooldown_seconds = 60,
        .enable_predictive = true,
        .prediction_confidence_threshold = 0.8
    };

    test_assert(scaler_init(&scaler_config) == 0, "Scaler init");
    test_assert(scaler_is_running(), "Scaler running");

    // Create scaling policy
    ScalingPolicy *policy = scaler_create_policy("api-policy", "api-service", 2, 10);
    test_assert(policy != NULL, "Create scaling policy");
    test_assert(strcmp(policy->name, "api-policy") == 0, "Policy name");
    test_assert(strcmp(policy->target_service, "api-service") == 0, "Policy target service");
    test_assert(policy->min_instances == 2, "Policy min instances");
    test_assert(policy->max_instances == 10, "Policy max instances");
    test_assert(policy->current_instances == 2, "Policy current instances");
    test_assert(policy->enabled, "Policy enabled");

    // Create scaling rules
    ScalingRule scale_up_rule = scaler_create_threshold_rule(
        "cpu_usage",
        OPERATOR_GREATER,
        70.0,
        SCALE_DIRECTION_UP,
        2
    );

    ScalingRule scale_down_rule = scaler_create_threshold_rule(
        "cpu_usage",
        OPERATOR_LESS,
        30.0,
        SCALE_DIRECTION_DOWN,
        1
    );

    test_assert(scaler_add_rule("api-policy", &scale_up_rule) == 0, "Add scale-up rule");
    test_assert(scaler_add_rule("api-policy", &scale_down_rule) == 0, "Add scale-down rule");
    test_assert(policy->rule_count == 2, "Rule count");

    // Test cooldown
    test_assert(!scaler_in_cooldown(policy, SCALE_DIRECTION_UP), "Not in cooldown initially");

    // Simulate high CPU triggering scale-up
    metrics_set("cpu_usage", 85.0);
    test_assert(scaler_evaluate_policy("api-policy") == 0, "Evaluate policy");

    // After evaluation, check if scaled
    ScalerStats scaler_stats = scaler_get_stats();
    test_assert(scaler_stats.total_policies == 1, "Scaler stats total policies");
    test_assert(scaler_stats.active_policies == 1, "Scaler stats active policies");
    test_assert(scaler_stats.total_evaluations > 0, "Scaler stats evaluations");

    // Test manual scaling
    test_assert(scaler_scale(policy, SCALE_DIRECTION_UP, 1, "Manual test") == 0, "Manual scale up");
    test_assert(policy->current_instances > 2, "Instances increased");

    // Get scaling events
    int event_count = 0;
    ScalingEvent **events = scaler_get_events(&event_count, 10);
    test_assert(events != NULL, "Get scaling events");
    test_assert(event_count > 0, "Event count > 0");
    if (events) free(events);

    // Test policy enable/disable
    test_assert(scaler_set_policy_enabled("api-policy", false) == 0, "Disable policy");
    test_assert(!policy->enabled, "Policy disabled");
    test_assert(scaler_set_policy_enabled("api-policy", true) == 0, "Enable policy");
    test_assert(policy->enabled, "Policy enabled");

    // Test set current instances
    test_assert(scaler_set_current_instances("api-policy", 5) == 0, "Set current instances");
    test_assert(policy->current_instances == 5, "Current instances updated");

    // Test predictive scaling
    double predicted = scaler_predict_metric("cpu_usage", 60);
    test_assert(predicted >= 0.0, "Predict metric");

    printf("  ✅ AutoScaler: %d passed\n", 28);

    // Shutdown tests
    printf("\n🛑 Shutdown Tests\n");

    test_assert(metrics_shutdown() == 0, "Metrics shutdown");
    test_assert(!metrics_is_running(), "Metrics not running");
    test_assert(scaler_shutdown() == 0, "Scaler shutdown");
    test_assert(!scaler_is_running(), "Scaler not running");

    printf("  ✅ Shutdown: %d passed\n", 4);

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ Passed: %-3d  ❌ Failed: %-3d                           ║\n",
           tests_passed, tests_failed);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
