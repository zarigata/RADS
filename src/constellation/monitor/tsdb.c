/*
 * RADS Constellation - Time-Series Database Implementation
 * Phase 7: Monitoring & Observability
 */

#define _GNU_SOURCE

#include "tsdb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static TSDBSeries **g_series = NULL;
static int g_series_count = 0;
static TSDBConfig g_config;
static pthread_mutex_t g_tsdb_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_compaction_thread;
static pthread_t g_cleanup_thread;
static bool g_initialized = false;
static bool g_running = false;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static int compare_datapoints(const void *a, const void *b) {
    const TSDBDataPoint *pa = (const TSDBDataPoint *)a;
    const TSDBDataPoint *pb = (const TSDBDataPoint *)b;
    return (pa->timestamp > pb->timestamp) - (pa->timestamp < pb->timestamp);
}

static TSDBBlock *create_block(time_t start_time) {
    TSDBBlock *block = calloc(1, sizeof(TSDBBlock));
    if (!block) return NULL;
    
    block->start_time = start_time;
    block->end_time = start_time + g_config.block_size_seconds;
    block->capacity = g_config.max_points_per_block;
    block->points = calloc(block->capacity, sizeof(TSDBDataPoint));
    block->point_count = 0;
    
    if (!block->points) {
        free(block);
        return NULL;
    }
    
    return block;
}

static void free_block(TSDBBlock *block) {
    if (!block) return;
    free(block->points);
    free(block);
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int tsdb_init(TSDBConfig *config) {
    pthread_mutex_lock(&g_tsdb_mutex);
    
    if (g_initialized) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        return 1;
    }
    
    if (config) {
        g_config = *config;
    } else {
        g_config.max_series = 10000;
        g_config.retention_seconds = DEFAULT_RETENTION_SECONDS;
        g_config.block_size_seconds = DEFAULT_BLOCK_SIZE;
        g_config.max_points_per_block = 3600;
        g_config.enable_compression = false;
        g_config.enable_disk_persistence = false;
        g_config.data_directory = NULL;
    }
    
    g_series = NULL;
    g_series_count = 0;
    g_initialized = true;
    g_running = true;
    
    pthread_create(&g_compaction_thread, NULL, tsdb_compaction_thread, NULL);
    pthread_create(&g_cleanup_thread, NULL, tsdb_cleanup_thread, NULL);
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return 0;
}

int tsdb_shutdown(void) {
    pthread_mutex_lock(&g_tsdb_mutex);
    
    if (!g_initialized) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        return -1;
    }
    
    g_running = false;
    g_initialized = false;
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    
    pthread_join(g_compaction_thread, NULL);
    pthread_join(g_cleanup_thread, NULL);
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    for (int i = 0; i < g_series_count; i++) {
        TSDBSeries *series = g_series[i];
        for (int j = 0; j < series->block_count; j++) {
            free_block(series->blocks[j]);
        }
        free(series->blocks);
        free(series);
    }
    free(g_series);
    g_series = NULL;
    g_series_count = 0;
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return 0;
}

bool tsdb_is_running(void) {
    return g_running;
}

/*
 * ============================================================================
 * SERIES MANAGEMENT
 * ============================================================================
 */

TSDBSeries *tsdb_create_series(const char *name, TSDBLabel *labels, int label_count) {
    if (!g_initialized || !name) return NULL;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    for (int i = 0; i < g_series_count; i++) {
        if (strcmp(g_series[i]->name, name) == 0) {
            pthread_mutex_unlock(&g_tsdb_mutex);
            return g_series[i];
        }
    }
    
    TSDBSeries *series = calloc(1, sizeof(TSDBSeries));
    if (!series) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        return NULL;
    }
    
    strncpy(series->name, name, MAX_SERIES_NAME - 1);
    
    if (labels && label_count > 0) {
        int count = label_count < MAX_LABEL_PAIRS ? label_count : MAX_LABEL_PAIRS;
        memcpy(series->labels, labels, count * sizeof(TSDBLabel));
        series->label_count = count;
    }
    
    series->blocks = NULL;
    series->block_count = 0;
    series->block_capacity = 0;
    series->total_points = 0;
    
    g_series = realloc(g_series, (g_series_count + 1) * sizeof(TSDBSeries *));
    g_series[g_series_count++] = series;
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return series;
}

int tsdb_delete_series(const char *name) {
    if (!g_initialized || !name) return -1;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    for (int i = 0; i < g_series_count; i++) {
        if (strcmp(g_series[i]->name, name) == 0) {
            TSDBSeries *series = g_series[i];
            
            for (int j = 0; j < series->block_count; j++) {
                free_block(series->blocks[j]);
            }
            free(series->blocks);
            free(series);
            
            for (int j = i; j < g_series_count - 1; j++) {
                g_series[j] = g_series[j + 1];
            }
            g_series_count--;
            
            pthread_mutex_unlock(&g_tsdb_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return -1;
}

TSDBSeries *tsdb_get_series(const char *name) {
    if (!g_initialized || !name) return NULL;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    for (int i = 0; i < g_series_count; i++) {
        if (strcmp(g_series[i]->name, name) == 0) {
            TSDBSeries *series = g_series[i];
            pthread_mutex_unlock(&g_tsdb_mutex);
            return series;
        }
    }
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return NULL;
}

TSDBSeries **tsdb_list_series(int *count) {
    if (!g_initialized || !count) return NULL;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    TSDBSeries **list = malloc(g_series_count * sizeof(TSDBSeries *));
    if (list) {
        memcpy(list, g_series, g_series_count * sizeof(TSDBSeries *));
        *count = g_series_count;
    }
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return list;
}

/*
 * ============================================================================
 * DATA INGESTION
 * ============================================================================
 */

int tsdb_insert(const char *series_name, double value, time_t timestamp) {
    if (!g_initialized || !series_name) return -1;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    TSDBSeries *series = NULL;
    for (int i = 0; i < g_series_count; i++) {
        if (strcmp(g_series[i]->name, series_name) == 0) {
            series = g_series[i];
            break;
        }
    }
    
    if (!series) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        series = tsdb_create_series(series_name, NULL, 0);
        if (!series) return -1;
        pthread_mutex_lock(&g_tsdb_mutex);
    }
    
    time_t block_start = (timestamp / g_config.block_size_seconds) * g_config.block_size_seconds;
    TSDBBlock *target_block = NULL;
    
    for (int i = 0; i < series->block_count; i++) {
        if (series->blocks[i]->start_time == block_start) {
            target_block = series->blocks[i];
            break;
        }
    }
    
    if (!target_block) {
        target_block = create_block(block_start);
        if (!target_block) {
            pthread_mutex_unlock(&g_tsdb_mutex);
            return -1;
        }
        
        if (series->block_count >= series->block_capacity) {
            series->block_capacity = series->block_capacity == 0 ? 8 : series->block_capacity * 2;
            series->blocks = realloc(series->blocks, series->block_capacity * sizeof(TSDBBlock *));
        }
        series->blocks[series->block_count++] = target_block;
    }
    
    if (target_block->point_count >= target_block->capacity) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        return -1;
    }
    
    target_block->points[target_block->point_count].value = value;
    target_block->points[target_block->point_count].timestamp = timestamp;
    target_block->point_count++;
    
    series->total_points++;
    if (series->total_points == 1 || timestamp < series->first_timestamp) {
        series->first_timestamp = timestamp;
    }
    if (series->total_points == 1 || timestamp > series->last_timestamp) {
        series->last_timestamp = timestamp;
    }
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return 0;
}

int tsdb_insert_batch(const char *series_name, TSDBDataPoint *points, int count) {
    if (!g_initialized || !series_name || !points || count <= 0) return -1;
    
    for (int i = 0; i < count; i++) {
        if (tsdb_insert(series_name, points[i].value, points[i].timestamp) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/*
 * ============================================================================
 * QUERYING
 * ============================================================================
 */

TSDBQueryResult *tsdb_query_range(const char *series_name, time_t start, time_t end) {
    if (!g_initialized || !series_name) return NULL;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    TSDBSeries *series = NULL;
    for (int i = 0; i < g_series_count; i++) {
        if (strcmp(g_series[i]->name, series_name) == 0) {
            series = g_series[i];
            break;
        }
    }
    
    if (!series) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        return NULL;
    }
    
    TSDBQueryResult *result = calloc(1, sizeof(TSDBQueryResult));
    if (!result) {
        pthread_mutex_unlock(&g_tsdb_mutex);
        return NULL;
    }
    
    strncpy(result->series_name, series_name, MAX_SERIES_NAME - 1);
    
    int estimated_points = 0;
    for (int i = 0; i < series->block_count; i++) {
        TSDBBlock *block = series->blocks[i];
        if (block->end_time < start || block->start_time > end) continue;
        estimated_points += block->point_count;
    }
    
    if (estimated_points == 0) {
        result->points = NULL;
        result->point_count = 0;
        pthread_mutex_unlock(&g_tsdb_mutex);
        return result;
    }
    
    result->points = malloc(estimated_points * sizeof(TSDBDataPoint));
    result->point_count = 0;
    
    for (int i = 0; i < series->block_count; i++) {
        TSDBBlock *block = series->blocks[i];
        if (block->end_time < start || block->start_time > end) continue;
        
        for (int j = 0; j < block->point_count; j++) {
            if (block->points[j].timestamp >= start && block->points[j].timestamp <= end) {
                result->points[result->point_count++] = block->points[j];
            }
        }
    }
    
    qsort(result->points, result->point_count, sizeof(TSDBDataPoint), compare_datapoints);
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return result;
}

double tsdb_query_aggregate(const char *series_name, TSDBAggregate agg, time_t start, time_t end, double percentile) {
    TSDBQueryResult *result = tsdb_query_range(series_name, start, end);
    if (!result || result->point_count == 0) {
        if (result) {
            free(result->points);
            free(result);
        }
        return 0.0;
    }
    
    double value = 0.0;
    
    switch (agg) {
        case TSDB_AGG_SUM:
            for (int i = 0; i < result->point_count; i++) {
                value += result->points[i].value;
            }
            break;
            
        case TSDB_AGG_AVG:
            for (int i = 0; i < result->point_count; i++) {
                value += result->points[i].value;
            }
            value /= result->point_count;
            break;
            
        case TSDB_AGG_MIN:
            value = result->points[0].value;
            for (int i = 1; i < result->point_count; i++) {
                if (result->points[i].value < value) {
                    value = result->points[i].value;
                }
            }
            break;
            
        case TSDB_AGG_MAX:
            value = result->points[0].value;
            for (int i = 1; i < result->point_count; i++) {
                if (result->points[i].value > value) {
                    value = result->points[i].value;
                }
            }
            break;
            
        case TSDB_AGG_COUNT:
            value = result->point_count;
            break;
            
        case TSDB_AGG_RATE:
            if (result->point_count >= 2) {
                double value_delta = result->points[result->point_count - 1].value - result->points[0].value;
                time_t time_delta = result->points[result->point_count - 1].timestamp - result->points[0].timestamp;
                value = time_delta > 0 ? value_delta / time_delta : 0.0;
            }
            break;
            
        case TSDB_AGG_PERCENTILE:
            if (percentile >= 0.0 && percentile <= 100.0) {
                int idx = (int)((percentile / 100.0) * (result->point_count - 1));
                value = result->points[idx].value;
            }
            break;
    }
    
    free(result->points);
    free(result);
    return value;
}

/*
 * ============================================================================
 * AGGREGATION HELPERS
 * ============================================================================
 */

double tsdb_sum(const char *series_name, time_t start, time_t end) {
    return tsdb_query_aggregate(series_name, TSDB_AGG_SUM, start, end, 0.0);
}

double tsdb_avg(const char *series_name, time_t start, time_t end) {
    return tsdb_query_aggregate(series_name, TSDB_AGG_AVG, start, end, 0.0);
}

double tsdb_min(const char *series_name, time_t start, time_t end) {
    return tsdb_query_aggregate(series_name, TSDB_AGG_MIN, start, end, 0.0);
}

double tsdb_max(const char *series_name, time_t start, time_t end) {
    return tsdb_query_aggregate(series_name, TSDB_AGG_MAX, start, end, 0.0);
}

double tsdb_rate(const char *series_name, time_t start, time_t end) {
    return tsdb_query_aggregate(series_name, TSDB_AGG_RATE, start, end, 0.0);
}

double tsdb_percentile(const char *series_name, double p, time_t start, time_t end) {
    return tsdb_query_aggregate(series_name, TSDB_AGG_PERCENTILE, start, end, p);
}

/*
 * ============================================================================
 * RETENTION & CLEANUP
 * ============================================================================
 */

int tsdb_cleanup_old_data(void) {
    if (!g_initialized) return -1;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    time_t now = time(NULL);
    time_t cutoff = now - g_config.retention_seconds;
    
    for (int i = 0; i < g_series_count; i++) {
        TSDBSeries *series = g_series[i];
        
        for (int j = 0; j < series->block_count; ) {
            if (series->blocks[j]->end_time < cutoff) {
                free_block(series->blocks[j]);
                
                for (int k = j; k < series->block_count - 1; k++) {
                    series->blocks[k] = series->blocks[k + 1];
                }
                series->block_count--;
            } else {
                j++;
            }
        }
    }
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return 0;
}

int tsdb_set_retention(int seconds) {
    if (!g_initialized || seconds < 0) return -1;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    g_config.retention_seconds = seconds;
    pthread_mutex_unlock(&g_tsdb_mutex);
    
    return 0;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

TSDBStats tsdb_get_stats(void) {
    TSDBStats stats = {0};
    
    if (!g_initialized) return stats;
    
    pthread_mutex_lock(&g_tsdb_mutex);
    
    stats.total_series = g_series_count;
    
    for (int i = 0; i < g_series_count; i++) {
        TSDBSeries *series = g_series[i];
        stats.total_blocks += series->block_count;
        stats.total_points += series->total_points;
        
        for (int j = 0; j < series->block_count; j++) {
            stats.memory_bytes += sizeof(TSDBBlock);
            stats.memory_bytes += series->blocks[j]->capacity * sizeof(TSDBDataPoint);
        }
    }
    
    pthread_mutex_unlock(&g_tsdb_mutex);
    return stats;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *tsdb_compaction_thread(void *arg) {
    (void)arg;
    
    while (g_running) {
        for (int i = 0; i < 3600 / 100 && g_running; i++) {
            usleep(100000);
        }
        
        if (!g_running) continue;
        
        // Compaction logic would go here
    }
    
    return NULL;
}

void *tsdb_cleanup_thread(void *arg) {
    (void)arg;
    
    while (g_running) {
        for (int i = 0; i < 300 / 1 && g_running; i++) {
            sleep(1);
        }
        
        if (!g_running) continue;
        
        tsdb_cleanup_old_data();
    }
    
    return NULL;
}
