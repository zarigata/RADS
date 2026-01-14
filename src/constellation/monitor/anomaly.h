/*
 * RADS Constellation - Anomaly Detection
 * Phase 9 (Iguana): Nice to Have Features
 *
 * Statistical analysis and machine learning for detecting system anomalies
 * Identifies unusual patterns, outliers, and potential issues
 */

#ifndef CONSTELLATION_ANOMALY_H
#define CONSTELLATION_ANOMALY_H

#include "../autoscale/metrics.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * ANOMALY DETECTION
 * ============================================================================
 */

#define MAX_ANOMALY_DETECTORS 100
#define MAX_ANOMALY_HISTORY 1000
#define MAX_ANOMALY_RULES 50

typedef enum {
    ANOMALY_SEVERITY_INFO = 0,
    ANOMALY_SEVERITY_WARNING = 1,
    ANOMALY_SEVERITY_ERROR = 2,
    ANOMALY_SEVERITY_CRITICAL = 3
} AnomalySeverity;

typedef enum {
    ANOMALY_TYPE_STATISTICAL = 0,
    ANOMALY_TYPE_THRESHOLD = 1,
    ANOMALY_TYPE_RATE_CHANGE = 2,
    ANOMALY_TYPE_PATTERN = 3,
    ANOMALY_TYPE_CORRELATION = 4
} AnomalyType;

typedef enum {
    DETECT_METHOD_ZSCORE = 0,
    DETECT_METHOD_IQR = 1,
    DETECT_METHOD_MAD = 2,
    DETECT_METHOD_ISOLATION_FOREST = 3,
    DETECT_METHOD_MOVING_AVERAGE = 4
} DetectionMethod;

typedef struct {
    char id[64];
    char metric_name[MAX_METRIC_NAME];
    AnomalyType type;
    AnomalySeverity severity;
    
    double detected_value;
    double expected_value;
    double deviation;
    double confidence;
    
    time_t detected_at;
    time_t resolved_at;
    bool is_resolved;
    
    char description[512];
    char recommendation[512];
} Anomaly;

typedef struct {
    char metric_name[MAX_METRIC_NAME];
    DetectionMethod method;
    
    double sensitivity;
    int window_size;
    double threshold_multiplier;
    
    double baseline_mean;
    double baseline_std_dev;
    double baseline_median;
    double baseline_iqr;
    
    time_t last_updated;
    int sample_count;
    
    bool enabled;
} AnomalyDetector;

typedef struct {
    char name[128];
    char metric_pattern[MAX_METRIC_NAME];
    
    DetectionMethod method;
    double threshold;
    int evaluation_window;
    
    AnomalySeverity severity;
    bool auto_resolve;
    int auto_resolve_after_seconds;
    
    bool enabled;
} AnomalyRule;

typedef struct {
    int total_detectors;
    int active_detectors;
    uint64_t total_anomalies;
    uint64_t active_anomalies;
    uint64_t resolved_anomalies;
    uint64_t false_positives;
    double detection_rate;
} AnomalyStats;

typedef struct {
    char metric1[MAX_METRIC_NAME];
    char metric2[MAX_METRIC_NAME];
    double correlation;
    time_t last_calculated;
} CorrelationPair;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int anomaly_init(void);
int anomaly_shutdown(void);
bool anomaly_is_running(void);

/*
 * ============================================================================
 * DETECTOR MANAGEMENT
 * ============================================================================
 */

AnomalyDetector *anomaly_create_detector(
    const char *metric_name,
    DetectionMethod method,
    double sensitivity
);

int anomaly_destroy_detector(const char *metric_name);
AnomalyDetector *anomaly_get_detector(const char *metric_name);
AnomalyDetector **anomaly_list_detectors(int *count);

int anomaly_set_detector_enabled(const char *metric_name, bool enabled);

/*
 * ============================================================================
 * RULE MANAGEMENT
 * ============================================================================
 */

AnomalyRule *anomaly_create_rule(
    const char *name,
    const char *metric_pattern,
    DetectionMethod method,
    double threshold,
    AnomalySeverity severity
);

int anomaly_destroy_rule(const char *name);
AnomalyRule *anomaly_get_rule(const char *name);
int anomaly_set_rule_enabled(const char *name, bool enabled);

/*
 * ============================================================================
 * DETECTION
 * ============================================================================
 */

int anomaly_detect(const char *metric_name, double value, time_t timestamp);
int anomaly_detect_all(void);

bool anomaly_is_anomalous(
    const char *metric_name,
    double value,
    DetectionMethod method,
    double *confidence
);

int anomaly_update_baseline(const char *metric_name);
int anomaly_update_all_baselines(void);

/*
 * ============================================================================
 * ANOMALY MANAGEMENT
 * ============================================================================
 */

Anomaly **anomaly_get_active(int *count);
Anomaly **anomaly_get_history(int limit, int *count);
Anomaly *anomaly_get_by_id(const char *id);

int anomaly_resolve(const char *id);
int anomaly_mark_false_positive(const char *id);
int anomaly_auto_resolve_old(int max_age_seconds);

/*
 * ============================================================================
 * STATISTICAL METHODS
 * ============================================================================
 */

bool anomaly_detect_zscore(
    double value,
    double mean,
    double std_dev,
    double threshold
);

bool anomaly_detect_iqr(
    double value,
    double q1,
    double q3,
    double multiplier
);

bool anomaly_detect_mad(
    double value,
    double median,
    double mad,
    double threshold
);

bool anomaly_detect_moving_average(
    double value,
    double *history,
    int history_count,
    int window_size,
    double threshold
);

/*
 * ============================================================================
 * CORRELATION ANALYSIS
 * ============================================================================
 */

double anomaly_calculate_correlation(
    const char *metric1,
    const char *metric2,
    int window_seconds
);

CorrelationPair **anomaly_find_correlations(
    double min_correlation,
    int *count
);

bool anomaly_detect_correlation_break(
    const char *metric1,
    const char *metric2,
    double expected_correlation,
    double tolerance
);

/*
 * ============================================================================
 * PATTERN ANALYSIS
 * ============================================================================
 */

bool anomaly_detect_spike(
    const char *metric_name,
    double value,
    double spike_threshold
);

bool anomaly_detect_drop(
    const char *metric_name,
    double value,
    double drop_threshold
);

bool anomaly_detect_flatline(
    const char *metric_name,
    int duration_seconds
);

bool anomaly_detect_oscillation(
    const char *metric_name,
    int period_seconds,
    double amplitude_threshold
);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

AnomalyStats anomaly_get_stats(void);
double anomaly_get_false_positive_rate(void);
double anomaly_get_detection_accuracy(void);

/*
 * ============================================================================
 * REPORTING
 * ============================================================================
 */

char *anomaly_generate_report(time_t start_time, time_t end_time);
int anomaly_export_to_json(const char *filepath);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

void calculate_baseline_statistics(AnomalyDetector *detector, double *values, int count);
double calculate_median(double *values, int count);
double calculate_iqr(double *values, int count, double *q1, double *q3);
double calculate_mad(double *values, int count, double median);
void generate_anomaly_id(char *id, size_t size);

#endif // CONSTELLATION_ANOMALY_H
