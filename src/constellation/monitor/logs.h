/*
 * RADS Constellation - Log Aggregation System
 * Phase 7: Monitoring & Observability
 */

#ifndef CONSTELLATION_LOGS_H
#define CONSTELLATION_LOGS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * LOG AGGREGATION
 * ============================================================================
 */

#define MAX_LOG_MESSAGE 2048
#define MAX_LOG_SOURCE 128
#define MAX_LOG_LABELS 16
#define MAX_LOG_BUFFER 10000

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} LogLevel;

typedef struct {
    char key[64];
    char value[128];
} LogLabel;

typedef struct {
    time_t timestamp;
    LogLevel level;
    char source[MAX_LOG_SOURCE];
    char message[MAX_LOG_MESSAGE];
    LogLabel labels[MAX_LOG_LABELS];
    int label_count;
    uint64_t id;
} LogEntry;

typedef struct {
    int max_entries;
    int retention_seconds;
    bool enable_compression;
    bool enable_disk_persistence;
    char *log_directory;
    LogLevel min_level;
} LogConfig;

typedef struct {
    uint64_t total_logs;
    uint64_t logs_by_level[5];
    uint64_t memory_bytes;
    uint64_t disk_bytes;
} LogStats;

typedef struct {
    char *source_pattern;
    LogLevel min_level;
    LogLevel max_level;
    time_t start_time;
    time_t end_time;
    char *message_pattern;
    LogLabel *labels;
    int label_count;
    int limit;
} LogQuery;

typedef struct {
    LogEntry *entries;
    int entry_count;
} LogQueryResult;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int logs_init(LogConfig *config);
int logs_shutdown(void);
bool logs_is_running(void);

/*
 * ============================================================================
 * LOG INGESTION
 * ============================================================================
 */

int logs_write(LogLevel level, const char *source, const char *message, LogLabel *labels, int label_count);
int logs_debug(const char *source, const char *message);
int logs_info(const char *source, const char *message);
int logs_warn(const char *source, const char *message);
int logs_error(const char *source, const char *message);
int logs_fatal(const char *source, const char *message);

/*
 * ============================================================================
 * LOG QUERYING
 * ============================================================================
 */

LogQueryResult *logs_query(LogQuery *query);
LogQueryResult *logs_query_recent(int count);
LogQueryResult *logs_query_by_level(LogLevel level, int count);
LogQueryResult *logs_query_by_source(const char *source, int count);

/*
 * ============================================================================
 * LOG STREAMING
 * ============================================================================
 */

typedef void (*LogStreamCallback)(LogEntry *entry, void *user_data);

int logs_stream_start(LogStreamCallback callback, void *user_data);
int logs_stream_stop(void);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

LogStats logs_get_stats(void);

/*
 * ============================================================================
 * PERSISTENCE
 * ============================================================================
 */

int logs_flush_to_disk(void);
int logs_load_from_disk(void);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *logs_flush_thread(void *arg);
void *logs_cleanup_thread(void *arg);

#endif // CONSTELLATION_LOGS_H
