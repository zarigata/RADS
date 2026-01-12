/*
 * RADS Constellation - Metrics Collection
 * Phase 6: Auto-Scaling
 */

#ifndef CONSTELLATION_METRICS_H
#define CONSTELLATION_METRICS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * METRICS COLLECTION
 * ============================================================================
 */

#define MAX_METRIC_NAME 128
#define MAX_METRIC_LABELS 8
#define MAX_LABEL_KEY 64
#define MAX_LABEL_VALUE 128
#define METRIC_HISTORY_SIZE 100

// Metric types
typedef enum {
    METRIC_TYPE_GAUGE = 0,      // Current value (CPU%, memory)
    METRIC_TYPE_COUNTER = 1,    // Cumulative value (requests)
    METRIC_TYPE_HISTOGRAM = 2,  // Distribution (response times)
    METRIC_TYPE_SUMMARY = 3     // Statistical summary
} MetricType;

// Metric label
typedef struct {
    char key[MAX_LABEL_KEY];
    char value[MAX_LABEL_VALUE];
} MetricLabel;

// Metric data point
typedef struct {
    double value;
    time_t timestamp;
} MetricDataPoint;

// Metric definition
typedef struct {
    char name[MAX_METRIC_NAME];
    MetricType type;

    // Labels (service, instance, etc.)
    MetricLabel labels[MAX_METRIC_LABELS];
    int label_count;

    // Current value
    double current_value;
    time_t last_updated;

    // Historical data
    MetricDataPoint history[METRIC_HISTORY_SIZE];
    int history_count;
    int history_index;

    // Statistics
    double min_value;
    double max_value;
    double avg_value;
    uint64_t sample_count;
} Metric;

// Metrics configuration
typedef struct {
    int max_metrics;
    int history_size;
    int retention_seconds;
    int collection_interval_ms;
    bool enable_aggregation;
} MetricsConfig;

// Metrics statistics
typedef struct {
    int total_metrics;
    int active_metrics;
    uint64_t total_samples;
    uint64_t total_updates;
} MetricsStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

// Initialize metrics system
int metrics_init(MetricsConfig *config);

// Shutdown metrics system
int metrics_shutdown(void);

// Check if metrics system is running
bool metrics_is_running(void);

/*
 * ============================================================================
 * METRIC MANAGEMENT
 * ============================================================================
 */

// Register a metric
Metric *metrics_register(
    const char *name,
    MetricType type,
    MetricLabel *labels,
    int label_count
);

// Unregister a metric
int metrics_unregister(const char *name);

// Get metric by name
Metric *metrics_get(const char *name);

// List all metrics
Metric **metrics_list_all(int *count);

/*
 * ============================================================================
 * METRIC UPDATES
 * ============================================================================
 */

// Set metric value (for gauges)
int metrics_set(const char *name, double value);

// Increment metric (for counters)
int metrics_increment(const char *name, double delta);

// Record value (for histograms/summaries)
int metrics_record(const char *name, double value);

// Update metric with timestamp
int metrics_update(const char *name, double value, time_t timestamp);

/*
 * ============================================================================
 * METRIC QUERIES
 * ============================================================================
 */

// Get current value
double metrics_get_current(const char *name);

// Get average over time period
double metrics_get_average(const char *name, int seconds);

// Get minimum value
double metrics_get_min(const char *name);

// Get maximum value
double metrics_get_max(const char *name);

// Get rate of change (per second)
double metrics_get_rate(const char *name, int seconds);

// Get percentile (50th, 90th, 95th, 99th)
double metrics_get_percentile(const char *name, double percentile);

/*
 * ============================================================================
 * AGGREGATION
 * ============================================================================
 */

// Aggregate metrics by label
Metric *metrics_aggregate_by_label(
    const char *name_pattern,
    const char *label_key,
    const char *label_value,
    int *count
);

// Get sum of metrics matching pattern
double metrics_sum(const char *name_pattern);

// Get count of metrics matching pattern
int metrics_count(const char *name_pattern);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

// Get metrics statistics
MetricsStats metrics_get_stats(void);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

// Collection thread
void *metrics_collection_thread(void *arg);

// Cleanup thread
void *metrics_cleanup_thread(void *arg);

#endif // CONSTELLATION_METRICS_H
