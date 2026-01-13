/*
 * RADS Constellation - Time-Series Database
 * Phase 7: Monitoring & Observability
 */

#ifndef CONSTELLATION_TSDB_H
#define CONSTELLATION_TSDB_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * TIME-SERIES DATABASE
 * ============================================================================
 */

#define MAX_SERIES_NAME 128
#define MAX_LABEL_PAIRS 16
#define MAX_LABEL_KEY 64
#define MAX_LABEL_VALUE 128
#define DEFAULT_RETENTION_SECONDS 86400  // 24 hours
#define DEFAULT_BLOCK_SIZE 3600          // 1 hour blocks

typedef struct {
    char key[MAX_LABEL_KEY];
    char value[MAX_LABEL_VALUE];
} TSDBLabel;

typedef struct {
    double value;
    time_t timestamp;
} TSDBDataPoint;

typedef struct {
    time_t start_time;
    time_t end_time;
    int point_count;
    int capacity;
    TSDBDataPoint *points;
} TSDBBlock;

typedef struct {
    char name[MAX_SERIES_NAME];
    TSDBLabel labels[MAX_LABEL_PAIRS];
    int label_count;
    
    TSDBBlock **blocks;
    int block_count;
    int block_capacity;
    
    time_t first_timestamp;
    time_t last_timestamp;
    uint64_t total_points;
} TSDBSeries;

typedef struct {
    int max_series;
    int retention_seconds;
    int block_size_seconds;
    int max_points_per_block;
    bool enable_compression;
    bool enable_disk_persistence;
    char *data_directory;
} TSDBConfig;

typedef struct {
    int total_series;
    int total_blocks;
    uint64_t total_points;
    uint64_t memory_bytes;
    uint64_t disk_bytes;
} TSDBStats;

typedef struct {
    char *name_pattern;
    TSDBLabel *labels;
    int label_count;
    time_t start_time;
    time_t end_time;
    int limit;
} TSDBQuery;

typedef enum {
    TSDB_AGG_SUM = 0,
    TSDB_AGG_AVG = 1,
    TSDB_AGG_MIN = 2,
    TSDB_AGG_MAX = 3,
    TSDB_AGG_COUNT = 4,
    TSDB_AGG_RATE = 5,
    TSDB_AGG_PERCENTILE = 6
} TSDBAggregate;

typedef struct {
    TSDBDataPoint *points;
    int point_count;
    char series_name[MAX_SERIES_NAME];
} TSDBQueryResult;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int tsdb_init(TSDBConfig *config);
int tsdb_shutdown(void);
bool tsdb_is_running(void);

/*
 * ============================================================================
 * SERIES MANAGEMENT
 * ============================================================================
 */

TSDBSeries *tsdb_create_series(const char *name, TSDBLabel *labels, int label_count);
int tsdb_delete_series(const char *name);
TSDBSeries *tsdb_get_series(const char *name);
TSDBSeries **tsdb_list_series(int *count);

/*
 * ============================================================================
 * DATA INGESTION
 * ============================================================================
 */

int tsdb_insert(const char *series_name, double value, time_t timestamp);
int tsdb_insert_batch(const char *series_name, TSDBDataPoint *points, int count);

/*
 * ============================================================================
 * QUERYING
 * ============================================================================
 */

TSDBQueryResult *tsdb_query(TSDBQuery *query);
TSDBQueryResult *tsdb_query_range(const char *series_name, time_t start, time_t end);
double tsdb_query_aggregate(const char *series_name, TSDBAggregate agg, time_t start, time_t end, double percentile);

/*
 * ============================================================================
 * AGGREGATION
 * ============================================================================
 */

double tsdb_sum(const char *series_name, time_t start, time_t end);
double tsdb_avg(const char *series_name, time_t start, time_t end);
double tsdb_min(const char *series_name, time_t start, time_t end);
double tsdb_max(const char *series_name, time_t start, time_t end);
double tsdb_rate(const char *series_name, time_t start, time_t end);
double tsdb_percentile(const char *series_name, double p, time_t start, time_t end);

/*
 * ============================================================================
 * RETENTION & CLEANUP
 * ============================================================================
 */

int tsdb_compact_blocks(TSDBSeries *series);
int tsdb_cleanup_old_data(void);
int tsdb_set_retention(int seconds);

/*
 * ============================================================================
 * PERSISTENCE
 * ============================================================================
 */

int tsdb_save_to_disk(const char *series_name);
int tsdb_load_from_disk(const char *series_name);
int tsdb_flush_all(void);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

TSDBStats tsdb_get_stats(void);

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *tsdb_compaction_thread(void *arg);
void *tsdb_cleanup_thread(void *arg);

#endif // CONSTELLATION_TSDB_H
