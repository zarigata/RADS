/*
 * RADS Constellation - Resource Limiting (cgroups)
 *
 * Uses Linux cgroups v2 for resource limiting:
 * - CPU: Limit CPU usage to specified percentage
 * - Memory: Limit RAM usage
 * - I/O: Limit disk I/O (future)
 * - Network: Limit bandwidth (future)
 */

#include "../constellation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define CGROUP_ROOT "/sys/fs/cgroup"
#define CONSTELLATION_CGROUP "constellation"

/*
 * Check if cgroups v2 is available
 */
static bool cgroups_available(void) {
    struct stat st;
    return (stat(CGROUP_ROOT, &st) == 0 && S_ISDIR(st.st_mode));
}

/*
 * Write value to cgroup file
 */
static int write_cgroup_file(const char *cgroup_path, const char *filename,
                              const char *value) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", cgroup_path, filename);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        if (errno != ENOENT) { // ENOENT is ok, file might not exist
            fprintf(stderr, "\033[1;33m[WARN]\033[0m Failed to open %s: %s\n",
                    path, strerror(errno));
        }
        return -1;
    }

    if (fprintf(fp, "%s", value) < 0) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to write to %s: %s\n",
                path, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

/*
 * Read value from cgroup file
 */
static int read_cgroup_file(const char *cgroup_path, const char *filename,
                             char *buffer, size_t size) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", cgroup_path, filename);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    if (!fgets(buffer, size, fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

/*
 * Setup cgroups for an instance
 */
int resources_setup_cgroups(Instance *instance) {
    if (!instance) {
        return -1;
    }

    // Check if cgroups are available
    if (!cgroups_available()) {
        printf("\033[1;33m[WARN]\033[0m cgroups not available, resource limits disabled\n");
        return 0; // Not fatal, just no limits
    }

    printf("\033[1;36m[CGROUPS]\033[0m Setting up resource limits for: %s\n",
           instance->name);

    // Create cgroup path: /sys/fs/cgroup/constellation/<instance_id>
    char cgroup_path[512];
    snprintf(cgroup_path, sizeof(cgroup_path),
             "%s/%s/%s", CGROUP_ROOT, CONSTELLATION_CGROUP, instance->id);

    // Create constellation parent cgroup if it doesn't exist
    char parent_path[512];
    snprintf(parent_path, sizeof(parent_path), "%s/%s", CGROUP_ROOT, CONSTELLATION_CGROUP);
    mkdir(parent_path, 0755); // Ignore error if exists

    // Create instance cgroup
    if (mkdir(cgroup_path, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to create cgroup: %s\n",
                strerror(errno));
        return -1;
    }

    // Configure CPU limit (if specified)
    if (instance->limits.cpu_percent > 0 && instance->limits.cpu_percent <= 100) {
        // cgroups v2 uses cpu.max: "max period"
        // For N% CPU: max = (period * N) / 100
        // Standard period is 100000 microseconds (100ms)
        char cpu_max[64];
        long period = 100000; // 100ms
        long max = (period * instance->limits.cpu_percent) / 100;
        snprintf(cpu_max, sizeof(cpu_max), "%ld %ld", max, period);

        if (write_cgroup_file(cgroup_path, "cpu.max", cpu_max) == 0) {
            printf("\033[1;32m[CGROUPS]\033[0m CPU limit set: %u%%\n",
                   instance->limits.cpu_percent);
        }
    }

    // Configure memory limit (if specified)
    if (instance->limits.ram_mb > 0) {
        char mem_max[64];
        long bytes = instance->limits.ram_mb * 1024 * 1024;
        snprintf(mem_max, sizeof(mem_max), "%ld", bytes);

        if (write_cgroup_file(cgroup_path, "memory.max", mem_max) == 0) {
            printf("\033[1;32m[CGROUPS]\033[0m Memory limit set: %luMB\n",
                   instance->limits.ram_mb);
        }
    }

    // Add process to cgroup (when it's spawned)
    // This will be done in resources_add_process_to_cgroup()

    printf("\033[1;32m[CGROUPS]\033[0m Resource limits configured\n");
    return 0;
}

/*
 * Add process to cgroup
 */
int resources_add_process_to_cgroup(Instance *instance, pid_t pid) {
    if (!instance || pid <= 0) {
        return -1;
    }

    if (!cgroups_available()) {
        return 0; // No cgroups, skip
    }

    char cgroup_path[512];
    snprintf(cgroup_path, sizeof(cgroup_path),
             "%s/%s/%s", CGROUP_ROOT, CONSTELLATION_CGROUP, instance->id);

    // Write PID to cgroup.procs
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    if (write_cgroup_file(cgroup_path, "cgroup.procs", pid_str) == 0) {
        printf("\033[1;32m[CGROUPS]\033[0m Process %d added to cgroup\n", pid);
        return 0;
    }

    fprintf(stderr, "\033[1;31m[ERROR]\033[0m Failed to add process to cgroup\n");
    return -1;
}

/*
 * Collect resource usage statistics from cgroups
 */
int resources_collect_stats(Instance *instance, ResourceUsage *usage) {
    if (!instance || !usage) {
        return -1;
    }

    if (!cgroups_available()) {
        // Fall back to /proc-based stats (from prototype)
        return -1; // TODO: Implement /proc fallback
    }

    char cgroup_path[512];
    snprintf(cgroup_path, sizeof(cgroup_path),
             "%s/%s/%s", CGROUP_ROOT, CONSTELLATION_CGROUP, instance->id);

    // Read CPU usage
    char buffer[256];
    if (read_cgroup_file(cgroup_path, "cpu.stat", buffer, sizeof(buffer)) == 0) {
        // Parse cpu.stat: "usage_usec <value>"
        unsigned long usage_usec = 0;
        if (sscanf(buffer, "usage_usec %lu", &usage_usec) == 1) {
            // Convert to percentage (approximate)
            usage->cpu_percent = (double)usage_usec / 10000.0; // Rough estimate
        }
    }

    // Read memory usage
    if (read_cgroup_file(cgroup_path, "memory.current", buffer, sizeof(buffer)) == 0) {
        unsigned long bytes = 0;
        if (sscanf(buffer, "%lu", &bytes) == 1) {
            usage->ram_mb = bytes / (1024 * 1024);
        }
    }

    usage->last_update = time(NULL);
    return 0;
}

/*
 * Teardown cgroups
 */
int resources_teardown_cgroups(Instance *instance) {
    if (!instance) {
        return -1;
    }

    if (!cgroups_available()) {
        return 0;
    }

    printf("\033[1;36m[CGROUPS]\033[0m Cleaning up cgroup for: %s\n",
           instance->name);

    char cgroup_path[512];
    snprintf(cgroup_path, sizeof(cgroup_path),
             "%s/%s/%s", CGROUP_ROOT, CONSTELLATION_CGROUP, instance->id);

    // Remove cgroup directory
    if (rmdir(cgroup_path) != 0 && errno != ENOENT) {
        fprintf(stderr, "\033[1;33m[WARN]\033[0m Failed to remove cgroup: %s\n",
                strerror(errno));
        return -1;
    }

    printf("\033[1;32m[CGROUPS]\033[0m Cgroup removed\n");
    return 0;
}

/*
 * Update resource limits
 */
int resources_update_limits(Instance *instance, ResourceLimits *limits) {
    if (!instance || !limits) {
        return -1;
    }

    // Update instance limits
    memcpy(&instance->limits, limits, sizeof(ResourceLimits));

    // Reapply cgroup limits
    if (cgroups_available()) {
        resources_teardown_cgroups(instance);
        resources_setup_cgroups(instance);
    }

    return 0;
}
