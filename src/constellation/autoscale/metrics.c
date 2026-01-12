/*
 * RADS Constellation - Metrics Collection Implementation
 * Phase 6: Auto-Scaling
 */

#define _GNU_SOURCE

#include "metrics.h"
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

static Metric **g_metrics = NULL;
static int g_metric_count = 0;
static MetricsConfig g_config;
static pthread_mutex_t g_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_collection_thread;
static pthread_t g_cleanup_thread;
static bool g_initialized = false;
static bool g_running = false;
static uint64_t g_total_samples = 0;
static uint64_t g_total_updates = 0;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static int compare_doubles(const void *a, const void *b) {
    double diff = (*(const MetricDataPoint *)a).value - (*(const MetricDataPoint *)b).value;
    return (diff > 0) - (diff < 0);
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int metrics_init(MetricsConfig *config) {
    pthread_mutex_lock(&g_metrics_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return 1;
    }

    // Set configuration
    if (config) {
        g_config = *config;
    } else {
        g_config.max_metrics = 1000;
        g_config.history_size = METRIC_HISTORY_SIZE;
        g_config.retention_seconds = 3600;  // 1 hour
        g_config.collection_interval_ms = 5000;  // 5 seconds
        g_config.enable_aggregation = true;
    }

    g_metrics = NULL;
    g_metric_count = 0;
    g_initialized = true;
    g_running = true;
    g_total_samples = 0;
    g_total_updates = 0;

    // Start background threads
    pthread_create(&g_collection_thread, NULL, metrics_collection_thread, NULL);
    pthread_create(&g_cleanup_thread, NULL, metrics_cleanup_thread, NULL);

    pthread_mutex_unlock(&g_metrics_mutex);
    return 0;
}

int metrics_shutdown(void) {
    pthread_mutex_lock(&g_metrics_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return -1;
    }

    g_running = false;
    g_initialized = false;

    pthread_mutex_unlock(&g_metrics_mutex);

    // Wait for threads
    pthread_join(g_collection_thread, NULL);
    pthread_join(g_cleanup_thread, NULL);

    pthread_mutex_lock(&g_metrics_mutex);

    // Free all metrics
    for (int i = 0; i < g_metric_count; i++) {
        free(g_metrics[i]);
    }
    free(g_metrics);
    g_metrics = NULL;
    g_metric_count = 0;

    pthread_mutex_unlock(&g_metrics_mutex);
    return 0;
}

bool metrics_is_running(void) {
    pthread_mutex_lock(&g_metrics_mutex);
    bool running = g_initialized && g_running;
    pthread_mutex_unlock(&g_metrics_mutex);
    return running;
}

/*
 * ============================================================================
 * METRIC MANAGEMENT
 * ============================================================================
 */

Metric *metrics_register(
    const char *name,
    MetricType type,
    MetricLabel *labels,
    int label_count
) {
    if (!g_initialized || !name) return NULL;

    pthread_mutex_lock(&g_metrics_mutex);

    // Check if already exists
    for (int i = 0; i < g_metric_count; i++) {
        if (strcmp(g_metrics[i]->name, name) == 0) {
            pthread_mutex_unlock(&g_metrics_mutex);
            return g_metrics[i];
        }
    }

    // Create new metric
    Metric *metric = calloc(1, sizeof(Metric));
    if (!metric) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return NULL;
    }

    strncpy(metric->name, name, sizeof(metric->name) - 1);
    metric->type = type;
    metric->current_value = 0.0;
    metric->last_updated = time(NULL);
    metric->history_count = 0;
    metric->history_index = 0;
    metric->min_value = INFINITY;
    metric->max_value = -INFINITY;
    metric->avg_value = 0.0;
    metric->sample_count = 0;

    // Copy labels
    if (labels && label_count > 0) {
        metric->label_count = label_count < MAX_METRIC_LABELS ? label_count : MAX_METRIC_LABELS;
        for (int i = 0; i < metric->label_count; i++) {
            metric->labels[i] = labels[i];
        }
    }

    // Add to list
    g_metrics = realloc(g_metrics, sizeof(Metric *) * (g_metric_count + 1));
    g_metrics[g_metric_count] = metric;
    g_metric_count++;

    pthread_mutex_unlock(&g_metrics_mutex);
    return metric;
}

int metrics_unregister(const char *name) {
    if (!g_initialized || !name) return -1;

    pthread_mutex_lock(&g_metrics_mutex);

    for (int i = 0; i < g_metric_count; i++) {
        if (strcmp(g_metrics[i]->name, name) == 0) {
            free(g_metrics[i]);

            for (int j = i; j < g_metric_count - 1; j++) {
                g_metrics[j] = g_metrics[j + 1];
            }

            g_metric_count--;

            pthread_mutex_unlock(&g_metrics_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return -1;
}

Metric *metrics_get(const char *name) {
    if (!g_initialized || !name) return NULL;

    pthread_mutex_lock(&g_metrics_mutex);

    for (int i = 0; i < g_metric_count; i++) {
        if (strcmp(g_metrics[i]->name, name) == 0) {
            Metric *metric = g_metrics[i];
            pthread_mutex_unlock(&g_metrics_mutex);
            return metric;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return NULL;
}

Metric **metrics_list_all(int *count) {
    if (!g_initialized || !count) return NULL;

    pthread_mutex_lock(&g_metrics_mutex);

    Metric **results = malloc(sizeof(Metric *) * g_metric_count);
    if (results) {
        memcpy(results, g_metrics, sizeof(Metric *) * g_metric_count);
        *count = g_metric_count;
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return results;
}

/*
 * ============================================================================
 * METRIC UPDATES
 * ============================================================================
 */

int metrics_update(const char *name, double value, time_t timestamp) {
    if (!name) return -1;

    pthread_mutex_lock(&g_metrics_mutex);

    Metric *metric = NULL;
    for (int i = 0; i < g_metric_count; i++) {
        if (strcmp(g_metrics[i]->name, name) == 0) {
            metric = g_metrics[i];
            break;
        }
    }

    if (!metric) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return -1;
    }

    // Update current value
    metric->current_value = value;
    metric->last_updated = timestamp;

    // Add to history
    int idx = metric->history_index;
    metric->history[idx].value = value;
    metric->history[idx].timestamp = timestamp;
    metric->history_index = (idx + 1) % METRIC_HISTORY_SIZE;

    if (metric->history_count < METRIC_HISTORY_SIZE) {
        metric->history_count++;
    }

    // Update statistics
    if (value < metric->min_value) metric->min_value = value;
    if (value > metric->max_value) metric->max_value = value;

    metric->avg_value = (metric->avg_value * metric->sample_count + value) / (metric->sample_count + 1);
    metric->sample_count++;

    g_total_samples++;
    g_total_updates++;

    pthread_mutex_unlock(&g_metrics_mutex);
    return 0;
}

int metrics_set(const char *name, double value) {
    return metrics_update(name, value, time(NULL));
}

int metrics_increment(const char *name, double delta) {
    pthread_mutex_lock(&g_metrics_mutex);

    Metric *metric = NULL;
    for (int i = 0; i < g_metric_count; i++) {
        if (strcmp(g_metrics[i]->name, name) == 0) {
            metric = g_metrics[i];
            break;
        }
    }

    if (!metric) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return -1;
    }

    double new_value = metric->current_value + delta;
    pthread_mutex_unlock(&g_metrics_mutex);

    return metrics_update(name, new_value, time(NULL));
}

int metrics_record(const char *name, double value) {
    return metrics_update(name, value, time(NULL));
}

/*
 * ============================================================================
 * METRIC QUERIES
 * ============================================================================
 */

double metrics_get_current(const char *name) {
    Metric *metric = metrics_get(name);
    if (!metric) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);
    double value = metric->current_value;
    pthread_mutex_unlock(&g_metrics_mutex);

    return value;
}

double metrics_get_average(const char *name, int seconds) {
    Metric *metric = metrics_get(name);
    if (!metric) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);

    time_t now = time(NULL);
    time_t cutoff = now - seconds;
    double sum = 0.0;
    int count = 0;

    for (int i = 0; i < metric->history_count; i++) {
        if (metric->history[i].timestamp >= cutoff) {
            sum += metric->history[i].value;
            count++;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);

    return count > 0 ? sum / count : 0.0;
}

double metrics_get_min(const char *name) {
    Metric *metric = metrics_get(name);
    if (!metric) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);
    double value = metric->min_value;
    pthread_mutex_unlock(&g_metrics_mutex);

    return value;
}

double metrics_get_max(const char *name) {
    Metric *metric = metrics_get(name);
    if (!metric) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);
    double value = metric->max_value;
    pthread_mutex_unlock(&g_metrics_mutex);

    return value;
}

double metrics_get_rate(const char *name, int seconds) {
    Metric *metric = metrics_get(name);
    if (!metric || seconds <= 0) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);

    if (metric->history_count < 2) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return 0.0;
    }

    time_t now = time(NULL);
    time_t cutoff = now - seconds;

    double first_value = 0.0;
    time_t first_time = 0;
    double last_value = 0.0;
    time_t last_time = 0;
    bool found_first = false;

    for (int i = 0; i < metric->history_count; i++) {
        if (metric->history[i].timestamp >= cutoff) {
            if (!found_first) {
                first_value = metric->history[i].value;
                first_time = metric->history[i].timestamp;
                found_first = true;
            }
            last_value = metric->history[i].value;
            last_time = metric->history[i].timestamp;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);

    if (!found_first || last_time == first_time) return 0.0;

    return (last_value - first_value) / (last_time - first_time);
}

double metrics_get_percentile(const char *name, double percentile) {
    Metric *metric = metrics_get(name);
    if (!metric || percentile < 0.0 || percentile > 1.0) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);

    if (metric->history_count == 0) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return 0.0;
    }

    // Copy and sort history
    MetricDataPoint *sorted = malloc(sizeof(MetricDataPoint) * metric->history_count);
    memcpy(sorted, metric->history, sizeof(MetricDataPoint) * metric->history_count);
    qsort(sorted, metric->history_count, sizeof(MetricDataPoint), compare_doubles);

    int index = (int)(percentile * (metric->history_count - 1));
    double value = sorted[index].value;

    free(sorted);
    pthread_mutex_unlock(&g_metrics_mutex);

    return value;
}

/*
 * ============================================================================
 * AGGREGATION
 * ============================================================================
 */

double metrics_sum(const char *name_pattern) {
    if (!g_initialized || !name_pattern) return 0.0;

    pthread_mutex_lock(&g_metrics_mutex);

    double sum = 0.0;
    for (int i = 0; i < g_metric_count; i++) {
        if (strstr(g_metrics[i]->name, name_pattern)) {
            sum += g_metrics[i]->current_value;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return sum;
}

int metrics_count(const char *name_pattern) {
    if (!g_initialized || !name_pattern) return 0;

    pthread_mutex_lock(&g_metrics_mutex);

    int count = 0;
    for (int i = 0; i < g_metric_count; i++) {
        if (strstr(g_metrics[i]->name, name_pattern)) {
            count++;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return count;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

MetricsStats metrics_get_stats(void) {
    MetricsStats stats = {0};

    if (!g_initialized) return stats;

    pthread_mutex_lock(&g_metrics_mutex);

    stats.total_metrics = g_metric_count;
    stats.active_metrics = g_metric_count;  // All registered metrics are active
    stats.total_samples = g_total_samples;
    stats.total_updates = g_total_updates;

    pthread_mutex_unlock(&g_metrics_mutex);
    return stats;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *metrics_collection_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks for quick exit
        for (int i = 0; i < g_config.collection_interval_ms / 100 && g_running; i++) {
            usleep(100000);  // 100ms
        }

        if (!g_running) continue;

        // Collection logic would go here
        // (e.g., scrape system metrics, poll services, etc.)
    }

    return NULL;
}

void *metrics_cleanup_thread(void *arg) {
    (void)arg;

    while (g_running) {
        // Sleep in smaller chunks for quick exit
        for (int i = 0; i < 60000 / 100 && g_running; i++) {  // 60 seconds
            usleep(100000);  // 100ms
        }

        if (!g_running) continue;

        pthread_mutex_lock(&g_metrics_mutex);

        time_t now = time(NULL);
        time_t cutoff = now - g_config.retention_seconds;

        // Clean up old history entries
        for (int i = 0; i < g_metric_count; i++) {
            Metric *metric = g_metrics[i];

            for (int j = 0; j < metric->history_count; ) {
                if (metric->history[j].timestamp < cutoff) {
                    // Remove entry by shifting
                    for (int k = j; k < metric->history_count - 1; k++) {
                        metric->history[k] = metric->history[k + 1];
                    }
                    metric->history_count--;
                } else {
                    j++;
                }
            }
        }

        pthread_mutex_unlock(&g_metrics_mutex);
    }

    return NULL;
}
