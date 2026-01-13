/*
 * RADS Constellation - Log Aggregation Implementation
 * Phase 7: Monitoring & Observability
 */

#define _GNU_SOURCE

#include "logs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static LogEntry *g_log_buffer = NULL;
static int g_log_count = 0;
static int g_log_capacity = 0;
static uint64_t g_next_log_id = 1;
static LogConfig g_config;
static pthread_mutex_t g_logs_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_flush_thread;
static pthread_t g_cleanup_thread;
static bool g_initialized = false;
static bool g_running = false;
static LogStreamCallback g_stream_callback = NULL;
static void *g_stream_user_data = NULL;

/*
 * ============================================================================
 * HELPERS
 * ============================================================================
 */

static const char *level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int logs_init(LogConfig *config) {
    pthread_mutex_lock(&g_logs_mutex);
    
    if (g_initialized) {
        pthread_mutex_unlock(&g_logs_mutex);
        return 1;
    }
    
    if (config) {
        g_config = *config;
    } else {
        g_config.max_entries = MAX_LOG_BUFFER;
        g_config.retention_seconds = 86400;  // 24 hours
        g_config.enable_compression = false;
        g_config.enable_disk_persistence = false;
        g_config.log_directory = NULL;
        g_config.min_level = LOG_LEVEL_DEBUG;
    }
    
    g_log_capacity = g_config.max_entries;
    g_log_buffer = calloc(g_log_capacity, sizeof(LogEntry));
    if (!g_log_buffer) {
        pthread_mutex_unlock(&g_logs_mutex);
        return -1;
    }
    
    g_log_count = 0;
    g_next_log_id = 1;
    g_initialized = true;
    g_running = true;
    
    pthread_create(&g_flush_thread, NULL, logs_flush_thread, NULL);
    pthread_create(&g_cleanup_thread, NULL, logs_cleanup_thread, NULL);
    
    pthread_mutex_unlock(&g_logs_mutex);
    return 0;
}

int logs_shutdown(void) {
    pthread_mutex_lock(&g_logs_mutex);
    
    if (!g_initialized) {
        pthread_mutex_unlock(&g_logs_mutex);
        return -1;
    }
    
    g_running = false;
    g_initialized = false;
    
    pthread_mutex_unlock(&g_logs_mutex);
    
    pthread_join(g_flush_thread, NULL);
    pthread_join(g_cleanup_thread, NULL);
    
    pthread_mutex_lock(&g_logs_mutex);
    
    free(g_log_buffer);
    g_log_buffer = NULL;
    g_log_count = 0;
    g_log_capacity = 0;
    
    pthread_mutex_unlock(&g_logs_mutex);
    return 0;
}

bool logs_is_running(void) {
    return g_running;
}

/*
 * ============================================================================
 * LOG INGESTION
 * ============================================================================
 */

int logs_write(LogLevel level, const char *source, const char *message, LogLabel *labels, int label_count) {
    if (!g_initialized || !source || !message) return -1;
    if (level < g_config.min_level) return 0;
    
    pthread_mutex_lock(&g_logs_mutex);
    
    if (g_log_count >= g_log_capacity) {
        memmove(&g_log_buffer[0], &g_log_buffer[1], (g_log_capacity - 1) * sizeof(LogEntry));
        g_log_count = g_log_capacity - 1;
    }
    
    LogEntry *entry = &g_log_buffer[g_log_count];
    entry->timestamp = time(NULL);
    entry->level = level;
    entry->id = g_next_log_id++;
    
    strncpy(entry->source, source, MAX_LOG_SOURCE - 1);
    entry->source[MAX_LOG_SOURCE - 1] = '\0';
    
    strncpy(entry->message, message, MAX_LOG_MESSAGE - 1);
    entry->message[MAX_LOG_MESSAGE - 1] = '\0';
    
    if (labels && label_count > 0) {
        int count = label_count < MAX_LOG_LABELS ? label_count : MAX_LOG_LABELS;
        memcpy(entry->labels, labels, count * sizeof(LogLabel));
        entry->label_count = count;
    } else {
        entry->label_count = 0;
    }
    
    g_log_count++;
    
    if (g_stream_callback) {
        g_stream_callback(entry, g_stream_user_data);
    }
    
    pthread_mutex_unlock(&g_logs_mutex);
    return 0;
}

int logs_debug(const char *source, const char *message) {
    return logs_write(LOG_LEVEL_DEBUG, source, message, NULL, 0);
}

int logs_info(const char *source, const char *message) {
    return logs_write(LOG_LEVEL_INFO, source, message, NULL, 0);
}

int logs_warn(const char *source, const char *message) {
    return logs_write(LOG_LEVEL_WARN, source, message, NULL, 0);
}

int logs_error(const char *source, const char *message) {
    return logs_write(LOG_LEVEL_ERROR, source, message, NULL, 0);
}

int logs_fatal(const char *source, const char *message) {
    return logs_write(LOG_LEVEL_FATAL, source, message, NULL, 0);
}

/*
 * ============================================================================
 * LOG QUERYING
 * ============================================================================
 */

LogQueryResult *logs_query(LogQuery *query) {
    if (!g_initialized || !query) return NULL;
    
    pthread_mutex_lock(&g_logs_mutex);
    
    LogQueryResult *result = calloc(1, sizeof(LogQueryResult));
    if (!result) {
        pthread_mutex_unlock(&g_logs_mutex);
        return NULL;
    }
    
    int matched = 0;
    for (int i = 0; i < g_log_count; i++) {
        LogEntry *entry = &g_log_buffer[i];
        
        if (query->start_time > 0 && entry->timestamp < query->start_time) continue;
        if (query->end_time > 0 && entry->timestamp > query->end_time) continue;
        if (entry->level < query->min_level || entry->level > query->max_level) continue;
        
        if (query->source_pattern && strstr(entry->source, query->source_pattern) == NULL) continue;
        if (query->message_pattern && strstr(entry->message, query->message_pattern) == NULL) continue;
        
        matched++;
    }
    
    if (matched == 0) {
        result->entries = NULL;
        result->entry_count = 0;
        pthread_mutex_unlock(&g_logs_mutex);
        return result;
    }
    
    int limit = query->limit > 0 ? query->limit : matched;
    if (limit > matched) limit = matched;
    
    result->entries = malloc(limit * sizeof(LogEntry));
    result->entry_count = 0;
    
    for (int i = g_log_count - 1; i >= 0 && result->entry_count < limit; i--) {
        LogEntry *entry = &g_log_buffer[i];
        
        if (query->start_time > 0 && entry->timestamp < query->start_time) continue;
        if (query->end_time > 0 && entry->timestamp > query->end_time) continue;
        if (entry->level < query->min_level || entry->level > query->max_level) continue;
        
        if (query->source_pattern && strstr(entry->source, query->source_pattern) == NULL) continue;
        if (query->message_pattern && strstr(entry->message, query->message_pattern) == NULL) continue;
        
        result->entries[result->entry_count++] = *entry;
    }
    
    pthread_mutex_unlock(&g_logs_mutex);
    return result;
}

LogQueryResult *logs_query_recent(int count) {
    LogQuery query = {
        .source_pattern = NULL,
        .min_level = LOG_LEVEL_DEBUG,
        .max_level = LOG_LEVEL_FATAL,
        .start_time = 0,
        .end_time = 0,
        .message_pattern = NULL,
        .labels = NULL,
        .label_count = 0,
        .limit = count
    };
    return logs_query(&query);
}

LogQueryResult *logs_query_by_level(LogLevel level, int count) {
    LogQuery query = {
        .source_pattern = NULL,
        .min_level = level,
        .max_level = level,
        .start_time = 0,
        .end_time = 0,
        .message_pattern = NULL,
        .labels = NULL,
        .label_count = 0,
        .limit = count
    };
    return logs_query(&query);
}

LogQueryResult *logs_query_by_source(const char *source, int count) {
    LogQuery query = {
        .source_pattern = (char *)source,
        .min_level = LOG_LEVEL_DEBUG,
        .max_level = LOG_LEVEL_FATAL,
        .start_time = 0,
        .end_time = 0,
        .message_pattern = NULL,
        .labels = NULL,
        .label_count = 0,
        .limit = count
    };
    return logs_query(&query);
}

/*
 * ============================================================================
 * LOG STREAMING
 * ============================================================================
 */

int logs_stream_start(LogStreamCallback callback, void *user_data) {
    if (!g_initialized || !callback) return -1;
    
    pthread_mutex_lock(&g_logs_mutex);
    g_stream_callback = callback;
    g_stream_user_data = user_data;
    pthread_mutex_unlock(&g_logs_mutex);
    
    return 0;
}

int logs_stream_stop(void) {
    if (!g_initialized) return -1;
    
    pthread_mutex_lock(&g_logs_mutex);
    g_stream_callback = NULL;
    g_stream_user_data = NULL;
    pthread_mutex_unlock(&g_logs_mutex);
    
    return 0;
}

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

LogStats logs_get_stats(void) {
    LogStats stats = {0};
    
    if (!g_initialized) return stats;
    
    pthread_mutex_lock(&g_logs_mutex);
    
    stats.total_logs = g_log_count;
    
    for (int i = 0; i < g_log_count; i++) {
        if (g_log_buffer[i].level >= 0 && g_log_buffer[i].level <= 4) {
            stats.logs_by_level[g_log_buffer[i].level]++;
        }
    }
    
    stats.memory_bytes = g_log_capacity * sizeof(LogEntry);
    
    pthread_mutex_unlock(&g_logs_mutex);
    return stats;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *logs_flush_thread(void *arg) {
    (void)arg;
    
    while (g_running) {
        for (int i = 0; i < 60 / 1 && g_running; i++) {
            sleep(1);
        }
        
        if (!g_running) continue;
        
        // Flush logic would go here
    }
    
    return NULL;
}

void *logs_cleanup_thread(void *arg) {
    (void)arg;
    
    while (g_running) {
        for (int i = 0; i < 300 / 1 && g_running; i++) {
            sleep(1);
        }
        
        if (!g_running) continue;
        
        pthread_mutex_lock(&g_logs_mutex);
        
        time_t now = time(NULL);
        time_t cutoff = now - g_config.retention_seconds;
        
        int removed = 0;
        for (int i = 0; i < g_log_count; ) {
            if (g_log_buffer[i].timestamp < cutoff) {
                memmove(&g_log_buffer[i], &g_log_buffer[i + 1], (g_log_count - i - 1) * sizeof(LogEntry));
                g_log_count--;
                removed++;
            } else {
                i++;
            }
        }
        
        pthread_mutex_unlock(&g_logs_mutex);
    }
    
    return NULL;
}
