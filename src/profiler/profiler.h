#ifndef RADS_PROFILER_H
#define RADS_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char* name;
    uint64_t start_time;
    uint64_t total_time;
    uint64_t call_count;
    int parent_index;
} ProfilerSample;

typedef struct {
    ProfilerSample* samples;
    int capacity;
    int count;
    int current_depth;
    uint64_t start_time;
    bool active;
} Profiler;

typedef struct {
    char* name;
    uint64_t total_time;
    uint64_t call_count;
    double percentage;
} ProfilerStat;

void profiler_init(Profiler* profiler);
void profiler_free(Profiler* profiler);
void profiler_start(Profiler* profiler);
void profiler_stop(Profiler* profiler);
void profiler_begin_sample(Profiler* profiler, const char* name);
void profiler_end_sample(Profiler* profiler);
void profiler_report_text(Profiler* profiler, const char* filename);
void profiler_report_flamegraph(Profiler* profiler, const char* filename);
ProfilerStat* profiler_get_stats(Profiler* profiler, int* count);

#endif
