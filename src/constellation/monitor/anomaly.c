/*
 * RADS Constellation - Anomaly Detection Implementation
 * Phase 9 (Iguana): Nice to Have Features
 */

#include "anomaly.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

typedef struct {
    AnomalyDetector *detectors[MAX_ANOMALY_DETECTORS];
    int detector_count;
    
    AnomalyRule *rules[MAX_ANOMALY_RULES];
    int rule_count;
    
    Anomaly *anomalies[MAX_ANOMALY_HISTORY];
    int anomaly_count;
    
    pthread_mutex_t lock;
    
    uint64_t total_anomalies;
    uint64_t false_positives;
    
    bool is_initialized;
} AnomalyState;

static AnomalyState g_state = {0};

int anomaly_init(void) {
    if (g_state.is_initialized) {
        return 0;
    }
    
    memset(&g_state, 0, sizeof(AnomalyState));
    pthread_mutex_init(&g_state.lock, NULL);
    g_state.is_initialized = true;
    
    return 0;
}

int anomaly_shutdown(void) {
    if (!g_state.is_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.detector_count; i++) {
        free(g_state.detectors[i]);
    }
    
    for (int i = 0; i < g_state.rule_count; i++) {
        free(g_state.rules[i]);
    }
    
    for (int i = 0; i < g_state.anomaly_count; i++) {
        free(g_state.anomalies[i]);
    }
    
    g_state.is_initialized = false;
    
    pthread_mutex_unlock(&g_state.lock);
    pthread_mutex_destroy(&g_state.lock);
    
    return 0;
}

bool anomaly_is_running(void) {
    return g_state.is_initialized;
}

void generate_anomaly_id(char *id, size_t size) {
    static uint64_t counter = 0;
    snprintf(id, size, "anomaly-%lu-%ld", counter++, time(NULL));
}

AnomalyDetector *anomaly_create_detector(const char *metric_name, DetectionMethod method, double sensitivity) {
    if (!g_state.is_initialized || !metric_name) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    if (g_state.detector_count >= MAX_ANOMALY_DETECTORS) {
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    AnomalyDetector *detector = calloc(1, sizeof(AnomalyDetector));
    if (!detector) {
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    strncpy(detector->metric_name, metric_name, MAX_METRIC_NAME - 1);
    detector->method = method;
    detector->sensitivity = sensitivity;
    detector->window_size = 100;
    detector->threshold_multiplier = 3.0;
    detector->enabled = true;
    detector->last_updated = time(NULL);
    
    g_state.detectors[g_state.detector_count++] = detector;
    
    pthread_mutex_unlock(&g_state.lock);
    return detector;
}

int anomaly_destroy_detector(const char *metric_name) {
    if (!g_state.is_initialized || !metric_name) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.detector_count; i++) {
        if (strcmp(g_state.detectors[i]->metric_name, metric_name) == 0) {
            free(g_state.detectors[i]);
            
            for (int j = i; j < g_state.detector_count - 1; j++) {
                g_state.detectors[j] = g_state.detectors[j + 1];
            }
            g_state.detector_count--;
            
            pthread_mutex_unlock(&g_state.lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return -1;
}

AnomalyDetector *anomaly_get_detector(const char *metric_name) {
    if (!g_state.is_initialized || !metric_name) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.detector_count; i++) {
        if (strcmp(g_state.detectors[i]->metric_name, metric_name) == 0) {
            AnomalyDetector *detector = g_state.detectors[i];
            pthread_mutex_unlock(&g_state.lock);
            return detector;
        }
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return NULL;
}

AnomalyRule *anomaly_create_rule(const char *name, const char *metric_pattern,
                                  DetectionMethod method, double threshold,
                                  AnomalySeverity severity) {
    if (!g_state.is_initialized || !name || !metric_pattern) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    if (g_state.rule_count >= MAX_ANOMALY_RULES) {
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    AnomalyRule *rule = calloc(1, sizeof(AnomalyRule));
    if (!rule) {
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    strncpy(rule->name, name, sizeof(rule->name) - 1);
    strncpy(rule->metric_pattern, metric_pattern, MAX_METRIC_NAME - 1);
    rule->method = method;
    rule->threshold = threshold;
    rule->severity = severity;
    rule->evaluation_window = 300;
    rule->auto_resolve = true;
    rule->auto_resolve_after_seconds = 600;
    rule->enabled = true;
    
    g_state.rules[g_state.rule_count++] = rule;
    
    pthread_mutex_unlock(&g_state.lock);
    return rule;
}

bool anomaly_detect_zscore(double value, double mean, double std_dev, double threshold) {
    if (std_dev == 0.0) {
        return false;
    }
    
    double zscore = fabs((value - mean) / std_dev);
    return zscore > threshold;
}

bool anomaly_detect_iqr(double value, double q1, double q3, double multiplier) {
    double iqr = q3 - q1;
    double lower_bound = q1 - multiplier * iqr;
    double upper_bound = q3 + multiplier * iqr;
    
    return value < lower_bound || value > upper_bound;
}

double calculate_median(double *values, int count) {
    if (count == 0) {
        return 0.0;
    }
    
    double *sorted = malloc(count * sizeof(double));
    if (!sorted) {
        return 0.0;
    }
    
    memcpy(sorted, values, count * sizeof(double));
    
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                double temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    double median;
    if (count % 2 == 0) {
        median = (sorted[count / 2 - 1] + sorted[count / 2]) / 2.0;
    } else {
        median = sorted[count / 2];
    }
    
    free(sorted);
    return median;
}

double calculate_iqr(double *values, int count, double *q1, double *q3) {
    if (count < 4) {
        *q1 = 0.0;
        *q3 = 0.0;
        return 0.0;
    }
    
    double *sorted = malloc(count * sizeof(double));
    if (!sorted) {
        return 0.0;
    }
    
    memcpy(sorted, values, count * sizeof(double));
    
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                double temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    int q1_idx = count / 4;
    int q3_idx = (3 * count) / 4;
    
    *q1 = sorted[q1_idx];
    *q3 = sorted[q3_idx];
    
    double iqr = *q3 - *q1;
    
    free(sorted);
    return iqr;
}

double calculate_mad(double *values, int count, double median) {
    if (count == 0) {
        return 0.0;
    }
    
    double *deviations = malloc(count * sizeof(double));
    if (!deviations) {
        return 0.0;
    }
    
    for (int i = 0; i < count; i++) {
        deviations[i] = fabs(values[i] - median);
    }
    
    double mad = calculate_median(deviations, count);
    free(deviations);
    
    return mad;
}

bool anomaly_detect_mad(double value, double median, double mad, double threshold) {
    if (mad == 0.0) {
        return false;
    }
    
    double modified_zscore = 0.6745 * fabs(value - median) / mad;
    return modified_zscore > threshold;
}

bool anomaly_detect_moving_average(double value, double *history, int history_count,
                                   int window_size, double threshold) {
    if (history_count < window_size) {
        return false;
    }
    
    double sum = 0.0;
    for (int i = history_count - window_size; i < history_count; i++) {
        sum += history[i];
    }
    double moving_avg = sum / window_size;
    
    double deviation = fabs(value - moving_avg) / moving_avg;
    return deviation > threshold;
}

void calculate_baseline_statistics(AnomalyDetector *detector, double *values, int count) {
    if (!detector || !values || count == 0) {
        return;
    }
    
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += values[i];
    }
    detector->baseline_mean = sum / count;
    
    double variance = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = values[i] - detector->baseline_mean;
        variance += diff * diff;
    }
    detector->baseline_std_dev = sqrt(variance / count);
    
    detector->baseline_median = calculate_median(values, count);
    
    double q1, q3;
    detector->baseline_iqr = calculate_iqr(values, count, &q1, &q3);
    
    detector->sample_count = count;
    detector->last_updated = time(NULL);
}

bool anomaly_is_anomalous(const char *metric_name, double value,
                          DetectionMethod method, double *confidence) {
    AnomalyDetector *detector = anomaly_get_detector(metric_name);
    if (!detector || !detector->enabled) {
        if (confidence) *confidence = 0.0;
        return false;
    }
    
    bool is_anomaly = false;
    double conf = 0.0;
    
    switch (method) {
        case DETECT_METHOD_ZSCORE:
            is_anomaly = anomaly_detect_zscore(value, detector->baseline_mean,
                                              detector->baseline_std_dev,
                                              detector->threshold_multiplier);
            if (is_anomaly) {
                conf = fabs((value - detector->baseline_mean) / detector->baseline_std_dev) / 
                       detector->threshold_multiplier;
            }
            break;
            
        case DETECT_METHOD_IQR: {
            double q1 = detector->baseline_median - detector->baseline_iqr / 2;
            double q3 = detector->baseline_median + detector->baseline_iqr / 2;
            is_anomaly = anomaly_detect_iqr(value, q1, q3, 1.5);
            conf = is_anomaly ? 0.85 : 0.0;
            break;
        }
            
        case DETECT_METHOD_MAD: {
            double mad = detector->baseline_iqr * 0.6745;
            is_anomaly = anomaly_detect_mad(value, detector->baseline_median, mad, 3.5);
            conf = is_anomaly ? 0.90 : 0.0;
            break;
        }
            
        default:
            break;
    }
    
    if (confidence) {
        *confidence = conf;
    }
    
    return is_anomaly;
}

int anomaly_detect(const char *metric_name, double value, time_t timestamp) {
    if (!g_state.is_initialized || !metric_name) {
        return -1;
    }
    
    double confidence = 0.0;
    bool is_anomaly = anomaly_is_anomalous(metric_name, 
                                           anomaly_get_detector(metric_name)->method,
                                           value, &confidence);
    
    if (!is_anomaly) {
        return 0;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    if (g_state.anomaly_count >= MAX_ANOMALY_HISTORY) {
        free(g_state.anomalies[0]);
        memmove(&g_state.anomalies[0], &g_state.anomalies[1],
                (MAX_ANOMALY_HISTORY - 1) * sizeof(Anomaly*));
        g_state.anomaly_count--;
    }
    
    Anomaly *anomaly = calloc(1, sizeof(Anomaly));
    if (!anomaly) {
        pthread_mutex_unlock(&g_state.lock);
        return -1;
    }
    
    generate_anomaly_id(anomaly->id, sizeof(anomaly->id));
    strncpy(anomaly->metric_name, metric_name, MAX_METRIC_NAME - 1);
    anomaly->type = ANOMALY_TYPE_STATISTICAL;
    anomaly->severity = ANOMALY_SEVERITY_WARNING;
    anomaly->detected_value = value;
    
    AnomalyDetector *detector = anomaly_get_detector(metric_name);
    if (detector) {
        anomaly->expected_value = detector->baseline_mean;
        anomaly->deviation = fabs(value - detector->baseline_mean);
    }
    
    anomaly->confidence = confidence;
    anomaly->detected_at = timestamp;
    anomaly->is_resolved = false;
    
    snprintf(anomaly->description, sizeof(anomaly->description),
             "Anomaly detected in %s: value %.2f deviates from expected %.2f by %.2f (confidence: %.2f%%)",
             metric_name, value, anomaly->expected_value, anomaly->deviation, confidence * 100);
    
    snprintf(anomaly->recommendation, sizeof(anomaly->recommendation),
             "Investigate %s for unusual activity. Check related metrics and system logs.",
             metric_name);
    
    g_state.anomalies[g_state.anomaly_count++] = anomaly;
    g_state.total_anomalies++;
    
    pthread_mutex_unlock(&g_state.lock);
    return 0;
}

Anomaly **anomaly_get_active(int *count) {
    if (!g_state.is_initialized || !count) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    int active_count = 0;
    for (int i = 0; i < g_state.anomaly_count; i++) {
        if (!g_state.anomalies[i]->is_resolved) {
            active_count++;
        }
    }
    
    if (active_count == 0) {
        *count = 0;
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    Anomaly **active = malloc(active_count * sizeof(Anomaly*));
    if (!active) {
        *count = 0;
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    int idx = 0;
    for (int i = 0; i < g_state.anomaly_count; i++) {
        if (!g_state.anomalies[i]->is_resolved) {
            active[idx++] = g_state.anomalies[i];
        }
    }
    
    *count = active_count;
    pthread_mutex_unlock(&g_state.lock);
    return active;
}

int anomaly_resolve(const char *id) {
    if (!g_state.is_initialized || !id) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.anomaly_count; i++) {
        if (strcmp(g_state.anomalies[i]->id, id) == 0) {
            g_state.anomalies[i]->is_resolved = true;
            g_state.anomalies[i]->resolved_at = time(NULL);
            pthread_mutex_unlock(&g_state.lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return -1;
}

int anomaly_mark_false_positive(const char *id) {
    if (anomaly_resolve(id) == 0) {
        g_state.false_positives++;
        return 0;
    }
    return -1;
}

bool anomaly_detect_spike(const char *metric_name, double value, double spike_threshold) {
    AnomalyDetector *detector = anomaly_get_detector(metric_name);
    if (!detector) {
        return false;
    }
    
    double ratio = value / detector->baseline_mean;
    return ratio > (1.0 + spike_threshold);
}

bool anomaly_detect_drop(const char *metric_name, double value, double drop_threshold) {
    AnomalyDetector *detector = anomaly_get_detector(metric_name);
    if (!detector) {
        return false;
    }
    
    double ratio = value / detector->baseline_mean;
    return ratio < (1.0 - drop_threshold);
}

AnomalyStats anomaly_get_stats(void) {
    AnomalyStats stats = {0};
    
    if (!g_state.is_initialized) {
        return stats;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    stats.total_detectors = g_state.detector_count;
    stats.total_anomalies = g_state.total_anomalies;
    stats.false_positives = g_state.false_positives;
    
    for (int i = 0; i < g_state.detector_count; i++) {
        if (g_state.detectors[i]->enabled) {
            stats.active_detectors++;
        }
    }
    
    for (int i = 0; i < g_state.anomaly_count; i++) {
        if (g_state.anomalies[i]->is_resolved) {
            stats.resolved_anomalies++;
        } else {
            stats.active_anomalies++;
        }
    }
    
    if (stats.total_anomalies > 0) {
        stats.detection_rate = (double)(stats.total_anomalies - stats.false_positives) / 
                               stats.total_anomalies;
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return stats;
}

double anomaly_get_false_positive_rate(void) {
    if (g_state.total_anomalies == 0) {
        return 0.0;
    }
    return (double)g_state.false_positives / g_state.total_anomalies;
}

int anomaly_update_baseline(const char *metric_name) {
    AnomalyDetector *detector = anomaly_get_detector(metric_name);
    if (!detector) {
        return -1;
    }
    
    Metric *metric = metrics_get(metric_name);
    if (!metric || metric->history_count < 10) {
        return -1;
    }
    
    double *values = malloc(metric->history_count * sizeof(double));
    if (!values) {
        return -1;
    }
    
    for (int i = 0; i < metric->history_count; i++) {
        values[i] = metric->history[i].value;
    }
    
    calculate_baseline_statistics(detector, values, metric->history_count);
    
    free(values);
    return 0;
}

int anomaly_update_all_baselines(void) {
    if (!g_state.is_initialized) {
        return -1;
    }
    
    int updated = 0;
    for (int i = 0; i < g_state.detector_count; i++) {
        if (anomaly_update_baseline(g_state.detectors[i]->metric_name) == 0) {
            updated++;
        }
    }
    
    return updated;
}
