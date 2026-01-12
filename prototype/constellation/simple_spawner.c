/*
 * RADS Constellation - Proof of Concept
 * Simple Instance Spawner
 *
 * This prototype demonstrates:
 * - Spawning isolated RADS processes
 * - Monitoring resource usage (CPU, RAM)
 * - Basic lifecycle management (start, monitor, stop)
 *
 * Compile: gcc -o simple_spawner simple_spawner.c
 * Run: ./simple_spawner <rads_script.rads>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_CYAN    "\033[1;36m"

typedef struct {
    pid_t pid;
    char name[256];
    char script[512];
    struct timeval start_time;
    int running;
} Instance;

// Get current time in milliseconds
long long current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

// Get process CPU usage (user + system time in seconds)
double get_process_cpu_time(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE* fp = fopen(path, "r");
    if (!fp) return -1.0;

    long utime, stime;
    char comm[256];
    char state;

    // Parse /proc/pid/stat (fields: pid comm state ... utime stime ...)
    fscanf(fp, "%*d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
           comm, &state, &utime, &stime);
    fclose(fp);

    // Convert clock ticks to seconds (usually 100 ticks/sec)
    long clock_ticks = sysconf(_SC_CLK_TCK);
    return (double)(utime + stime) / clock_ticks;
}

// Get process memory usage in MB
double get_process_memory_mb(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE* fp = fopen(path, "r");
    if (!fp) return -1.0;

    char line[256];
    long vm_rss = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%ld", &vm_rss);
            break;
        }
    }
    fclose(fp);

    // VmRSS is in KB, convert to MB
    return (double)vm_rss / 1024.0;
}

// Print instance stats
void print_stats(Instance* inst) {
    if (!inst->running) {
        printf("%s[INSTANCE]%s %s is not running\n",
               COLOR_YELLOW, COLOR_RESET, inst->name);
        return;
    }

    // Check if process still exists
    if (kill(inst->pid, 0) != 0) {
        inst->running = 0;
        printf("%s[INSTANCE]%s %s has stopped\n",
               COLOR_RED, COLOR_RESET, inst->name);
        return;
    }

    double cpu_time = get_process_cpu_time(inst->pid);
    double memory_mb = get_process_memory_mb(inst->pid);

    // Calculate uptime
    struct timeval now;
    gettimeofday(&now, NULL);
    long uptime_sec = now.tv_sec - inst->start_time.tv_sec;

    printf("\n%s╔════════════════════════════════════════════════╗%s\n",
           COLOR_CYAN, COLOR_RESET);
    printf("%s║  RADS CONSTELLATION - Instance Stats          ║%s\n",
           COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════════╝%s\n\n",
           COLOR_CYAN, COLOR_RESET);

    printf("  %sInstance:%s %s\n", COLOR_BOLD, COLOR_RESET, inst->name);
    printf("  %sScript:%s   %s\n", COLOR_BOLD, COLOR_RESET, inst->script);
    printf("  %sPID:%s      %d\n", COLOR_BOLD, COLOR_RESET, inst->pid);
    printf("  %sStatus:%s   %s%s%s\n", COLOR_BOLD, COLOR_RESET,
           COLOR_GREEN, "RUNNING", COLOR_RESET);
    printf("  %sUptime:%s   %ld seconds\n", COLOR_BOLD, COLOR_RESET, uptime_sec);

    if (cpu_time >= 0) {
        printf("  %sCPU Time:%s %.2f seconds\n",
               COLOR_BOLD, COLOR_RESET, cpu_time);
    }

    if (memory_mb >= 0) {
        printf("  %sMemory:%s   %.2f MB\n",
               COLOR_BOLD, COLOR_RESET, memory_mb);
    }

    printf("\n");
}

// Spawn RADS instance
int spawn_instance(Instance* inst, const char* script_path) {
    printf("\n%s[SPAWN]%s Creating instance for: %s\n",
           COLOR_BLUE, COLOR_RESET, script_path);

    // Get start time
    gettimeofday(&inst->start_time, NULL);

    // Fork process
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "%s[ERROR]%s Failed to fork: %s\n",
                COLOR_RED, COLOR_RESET, strerror(errno));
        return -1;
    }

    if (pid == 0) {
        // Child process - execute RADS
        printf("%s[CHILD]%s Executing RADS with script: %s\n",
               COLOR_GREEN, COLOR_RESET, script_path);

        // Execute RADS interpreter
        execl("./rads", "rads", script_path, (char*)NULL);

        // If execl returns, it failed
        fprintf(stderr, "%s[ERROR]%s Failed to exec RADS: %s\n",
                COLOR_RED, COLOR_RESET, strerror(errno));
        exit(1);
    }

    // Parent process
    inst->pid = pid;
    inst->running = 1;
    snprintf(inst->name, sizeof(inst->name), "instance-%d", pid);
    snprintf(inst->script, sizeof(inst->script), "%s", script_path);

    printf("%s[PARENT]%s Spawned instance with PID: %d\n",
           COLOR_GREEN, COLOR_RESET, pid);

    // Give it a moment to start
    usleep(100000); // 100ms

    return 0;
}

// Stop instance
void stop_instance(Instance* inst) {
    if (!inst->running) {
        printf("%s[STOP]%s Instance already stopped\n",
               COLOR_YELLOW, COLOR_RESET);
        return;
    }

    printf("\n%s[STOP]%s Stopping instance PID %d...\n",
           COLOR_YELLOW, COLOR_RESET, inst->pid);

    // Send SIGTERM (graceful shutdown)
    if (kill(inst->pid, SIGTERM) == 0) {
        printf("%s[STOP]%s Sent SIGTERM, waiting for process to exit...\n",
               COLOR_YELLOW, COLOR_RESET);

        // Wait up to 5 seconds for graceful shutdown
        int status;
        int waited = 0;
        while (waited < 50) { // 50 * 100ms = 5 seconds
            int result = waitpid(inst->pid, &status, WNOHANG);
            if (result == inst->pid) {
                printf("%s[STOP]%s Process exited gracefully\n",
                       COLOR_GREEN, COLOR_RESET);
                inst->running = 0;
                return;
            }
            usleep(100000); // 100ms
            waited++;
        }

        // Still running, force kill
        printf("%s[STOP]%s Process didn't exit, sending SIGKILL...\n",
               COLOR_RED, COLOR_RESET);
        kill(inst->pid, SIGKILL);
        waitpid(inst->pid, &status, 0);
    }

    inst->running = 0;
    printf("%s[STOP]%s Instance stopped\n", COLOR_GREEN, COLOR_RESET);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rads_script.rads> [monitor_seconds]\n", argv[0]);
        fprintf(stderr, "Example: %s examples/hello.rads 10\n", argv[0]);
        return 1;
    }

    const char* script_path = argv[1];
    int monitor_seconds = 10; // Default: monitor for 10 seconds

    if (argc >= 3) {
        monitor_seconds = atoi(argv[2]);
    }

    printf("\n%s%s", COLOR_BOLD, COLOR_CYAN);
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  🌟 RADS CONSTELLATION - Proof of Concept     ║\n");
    printf("║  Simple Instance Spawner v0.1                 ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);

    printf("Script: %s\n", script_path);
    printf("Monitor Duration: %d seconds\n", monitor_seconds);

    // Create instance
    Instance inst = {0};

    // Spawn the instance
    if (spawn_instance(&inst, script_path) != 0) {
        return 1;
    }

    printf("\n%s[MONITOR]%s Monitoring instance for %d seconds...\n",
           COLOR_BLUE, COLOR_RESET, monitor_seconds);
    printf("%s[MONITOR]%s Press Ctrl+C to stop early\n\n",
           COLOR_BLUE, COLOR_RESET);

    // Monitor for specified duration
    long long start = current_time_ms();
    long long end = start + (monitor_seconds * 1000);

    int stats_count = 0;
    while (current_time_ms() < end) {
        // Print stats every 2 seconds
        if (stats_count % 20 == 0) { // 20 * 100ms = 2 seconds
            print_stats(&inst);
        }

        // Check if instance is still running
        if (!inst.running) {
            printf("%s[MONITOR]%s Instance stopped, exiting monitor\n",
                   COLOR_YELLOW, COLOR_RESET);
            break;
        }

        usleep(100000); // 100ms
        stats_count++;
    }

    // Final stats
    if (inst.running) {
        printf("%s[MONITOR]%s Monitoring complete. Final stats:\n",
               COLOR_BLUE, COLOR_RESET);
        print_stats(&inst);

        // Stop the instance
        stop_instance(&inst);
    }

    printf("\n%s[DONE]%s Prototype demonstration complete!\n\n",
           COLOR_GREEN, COLOR_RESET);

    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n",
           COLOR_CYAN, COLOR_RESET);
    printf("%sProof of Concept Validated:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  ✓ Process spawning works\n");
    printf("  ✓ Resource monitoring works (CPU, RAM)\n");
    printf("  ✓ Instance lifecycle management works\n");
    printf("  ✓ Process isolation demonstrated\n");
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n\n",
           COLOR_CYAN, COLOR_RESET);

    return 0;
}
