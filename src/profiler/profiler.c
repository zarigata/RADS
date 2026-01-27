#include "profiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define PROFILER_MAX_SAMPLES 10000
#define PROFILER_MAX_STACK_DEPTH 100

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void profiler_init(Profiler* profiler) {
    profiler->samples = (ProfilerSample*)malloc(PROFILER_MAX_SAMPLES * sizeof(ProfilerSample));
    profiler->capacity = PROFILER_MAX_SAMPLES;
    profiler->count = 0;
    profiler->current_depth = 0;
    profiler->active = false;
    profiler->start_time = 0;
}

void profiler_free(Profiler* profiler) {
    if (profiler->samples) {
        for (int i = 0; i < profiler->count; i++) {
            if (profiler->samples[i].name) {
                free(profiler->samples[i].name);
            }
        }
        free(profiler->samples);
    }
    profiler->samples = NULL;
    profiler->count = 0;
}

void profiler_start(Profiler* profiler) {
    if (profiler->active) return;
    profiler->active = true;
    profiler->start_time = get_time_ns();
    profiler->count = 0;
    profiler->current_depth = 0;
}

void profiler_stop(Profiler* profiler) {
    if (!profiler->active) return;
    profiler->active = false;
}

void profiler_begin_sample(Profiler* profiler, const char* name) {
    if (!profiler->active) return;

    if (profiler->count >= profiler->capacity) {
        return;
    }

    ProfilerSample* sample = &profiler->samples[profiler->count];
    sample->name = strdup(name);
    sample->start_time = get_time_ns();
    sample->total_time = 0;
    sample->call_count = 0;
    sample->parent_index = profiler->current_depth > 0 ? profiler->count - 1 : -1;

    profiler->count++;
    profiler->current_depth++;
}

void profiler_end_sample(Profiler* profiler) {
    if (!profiler->active || profiler->current_depth <= 0) return;

    profiler->current_depth--;

    for (int i = profiler->count - 1; i >= 0; i--) {
        ProfilerSample* sample = &profiler->samples[i];
        if (sample->total_time == 0) {
            sample->total_time = get_time_ns() - sample->start_time;
            sample->call_count++;
            break;
        }
    }
}

static void merge_samples(Profiler* profiler) {
    for (int i = 0; i < profiler->count; i++) {
        for (int j = i + 1; j < profiler->count; j++) {
            if (strcmp(profiler->samples[i].name, profiler->samples[j].name) == 0 &&
                profiler->samples[i].parent_index == profiler->samples[j].parent_index) {
                profiler->samples[i].total_time += profiler->samples[j].total_time;
                profiler->samples[i].call_count += profiler->samples[j].call_count;
                free(profiler->samples[j].name);
                profiler->samples[j].name = NULL;
            }
        }
    }
}

ProfilerStat* profiler_get_stats(Profiler* profiler, int* count) {
    *count = profiler->count;

    ProfilerStat* stats = (ProfilerStat*)malloc(profiler->count * sizeof(ProfilerStat));

    for (int i = 0; i < profiler->count; i++) {
        if (!profiler->samples[i].name) {
            (*count)--;
            continue;
        }

        stats[i].name = strdup(profiler->samples[i].name);
        stats[i].total_time = profiler->samples[i].total_time;
        stats[i].call_count = profiler->samples[i].call_count;
    }

    uint64_t total_time = get_time_ns() - profiler->start_time;

    for (int i = 0; i < *count; i++) {
        stats[i].percentage = (double)stats[i].total_time / total_time * 100.0;
    }

    return stats;
}

void profiler_report_text(Profiler* profiler, const char* filename) {
    int count;
    ProfilerStat* stats = profiler_get_stats(profiler, &count);

    if (filename) {
        FILE* file = fopen(filename, "w");
        if (!file) {
            fprintf(stderr, "Failed to open file: %s\n", filename);
            return;
        }

        fprintf(file, "=== RADS Profiler Report ===\n");
        fprintf(file, "Total Samples: %d\n", count);
        fprintf(file, "Duration: %.3f ms\n\n", (get_time_ns() - profiler->start_time) / 1000000.0);

        fprintf(file, "%-20s %10s %12s %10s\n", "Function", "Calls", "Time (ms)", "Percent");
        fprintf(file, "%-20s %10s %12s %10s\n", "--------------------", "----------", "------------", "----------");

        for (int i = 0; i < count; i++) {
            fprintf(file, "%-20s %10llu %12.3f %9.1f%%\n",
                stats[i].name,
                stats[i].call_count,
                stats[i].total_time / 1000000.0,
                stats[i].percentage);
        }

        fclose(file);
        printf("Profiler report written to: %s\n", filename);
    } else {
        printf("=== RADS Profiler Report ===\n");
        printf("Total Samples: %d\n", count);
        printf("Duration: %.3f ms\n\n", (get_time_ns() - profiler->start_time) / 1000000.0);

        printf("%-20s %10s %12s %10s\n", "Function", "Calls", "Time (ms)", "Percent");
        printf("%-20s %10s %12s %10s\n", "--------------------", "----------", "------------", "----------");

        for (int i = 0; i < count; i++) {
            printf("%-20s %10llu %12.3f %9.1f%%\n",
                stats[i].name,
                stats[i].call_count,
                stats[i].total_time / 1000000.0,
                stats[i].percentage);
        }
    }

    for (int i = 0; i < count; i++) {
        free(stats[i].name);
    }
    free(stats);
}

void profiler_report_flamegraph(Profiler* profiler, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return;
    }

    fprintf(file, "<!DOCTYPE html>\n");
    fprintf(file, "<html>\n");
    fprintf(file, "<head>\n");
    fprintf(file, "<title>RADS Flame Graph</title>\n");
    fprintf(file, "<style>\n");
    fprintf(file, "  body { font-family: sans-serif; margin: 20px; }\n");
    fprintf(file, "  .frame { position: absolute; border: 1px solid #ccc; overflow: hidden; }\n");
    fprintf(file, "  .frame:hover { opacity: 0.8; }\n");
    fprintf(file, "  #graph { position: relative; }\n");
    fprintf(file, "  #tooltip { position: absolute; display: none; background: #333; color: #fff; padding: 5px; border-radius: 3px; }\n");
    fprintf(file, "</style>\n");
    fprintf(file, "<script>\n");
    fprintf(file, "function showTooltip(event, text) {\n");
    fprintf(file, "  var tooltip = document.getElementById('tooltip');\n");
    fprintf(file, "  tooltip.style.display = 'block';\n");
    fprintf(file, "  tooltip.style.left = event.pageX + 10 + 'px';\n");
    fprintf(file, "  tooltip.style.top = event.pageY + 10 + 'px';\n");
    fprintf(file, "  tooltip.innerHTML = text;\n");
    fprintf(file, "}\n");
    fprintf(file, "function hideTooltip() {\n");
    fprintf(file, "  document.getElementById('tooltip').style.display = 'none';\n");
    fprintf(file, "}\n");
    fprintf(file, "</script>\n");
    fprintf(file, "</head>\n");
    fprintf(file, "<body>\n");
    fprintf(file, "<h1>RADS Flame Graph</h1>\n");
    fprintf(file, "<div id=\"tooltip\"></div>\n");
    fprintf(file, "<div id=\"graph\" style=\"height: 600px; width: 100%%; border: 1px solid #ccc;\"></div>\n");

    uint64_t total_time = get_time_ns() - profiler->start_time;
    int stack_depth = 0;
    uint64_t current_x = 0;
    const int row_height = 25;

    for (int i = 0; i < profiler->count; i++) {
        ProfilerSample* sample = &profiler->samples[i];

        if (sample->total_time == 0) continue;

        double percent = (double)sample->total_time / total_time * 100.0;
        int width = (int)(percent * 10);
        if (width < 1) width = 1;

        uint32_t color = 0x0000FF;
        color = (color + (i * 12345)) & 0xFFFFFF;

        fprintf(file, "<div class=\"frame\" \n");
        fprintf(file, "  style=\"\n");
        fprintf(file, "    left: %dpx;\n", current_x);
        fprintf(file, "    top: %dpx;\n", stack_depth * row_height);
        fprintf(file, "    width: %dpx;\n", width);
        fprintf(file, "    height: %dpx;\n", row_height - 1);
        fprintf(file, "    background-color: #%06x;\n", color);
        fprintf(file, "  \"\n");
        fprintf(file, "  onmouseover=\"showTooltip(event, '<b>%s</b><br/>Time: %.3fms<br/>Calls: %llu')\"\n",
            sample->name,
            sample->total_time / 1000000.0,
            (unsigned long long)sample->call_count);
        fprintf(file, "  onmouseout=\"hideTooltip()\"\n");
        fprintf(file, ">%s</div>\n", sample->name);

        current_x += width;

        if (sample->parent_index == -1) {
            current_x = 0;
            stack_depth = 0;
        } else {
            stack_depth++;
        }
    }

    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");

    fclose(file);
    printf("Flame graph written to: %s\n", filename);
}
