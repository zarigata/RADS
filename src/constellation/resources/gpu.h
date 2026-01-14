/*
 * RADS Constellation - GPU Resource Management
 * Phase 9 (Iguana): Nice to Have Features
 *
 * GPU resource allocation, scheduling, and monitoring
 * Supports NVIDIA CUDA, AMD ROCm, and Intel oneAPI
 */

#ifndef CONSTELLATION_GPU_H
#define CONSTELLATION_GPU_H

#include "../constellation.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * GPU MANAGEMENT
 * ============================================================================
 */

#define MAX_GPUS_PER_NODE 16
#define MAX_GPU_NAME 128
#define MAX_GPU_PROCESSES 64

typedef enum {
    GPU_VENDOR_NVIDIA = 0,
    GPU_VENDOR_AMD = 1,
    GPU_VENDOR_INTEL = 2,
    GPU_VENDOR_UNKNOWN = 3
} GPUVendor;

typedef enum {
    GPU_STATE_IDLE = 0,
    GPU_STATE_ACTIVE = 1,
    GPU_STATE_BUSY = 2,
    GPU_STATE_ERROR = 3,
    GPU_STATE_OFFLINE = 4
} GPUState;

typedef enum {
    GPU_MEMORY_TYPE_GDDR6 = 0,
    GPU_MEMORY_TYPE_HBM2 = 1,
    GPU_MEMORY_TYPE_HBM3 = 2,
    GPU_MEMORY_TYPE_UNKNOWN = 3
} GPUMemoryType;

typedef struct {
    char name[MAX_GPU_NAME];
    char uuid[64];
    int device_id;
    
    GPUVendor vendor;
    char driver_version[64];
    int compute_capability_major;
    int compute_capability_minor;
    
    uint64_t memory_total_mb;
    uint64_t memory_free_mb;
    uint64_t memory_used_mb;
    GPUMemoryType memory_type;
    
    int cuda_cores;
    int sm_count;
    int clock_speed_mhz;
    int memory_clock_mhz;
    
    double utilization_gpu;
    double utilization_memory;
    double temperature_celsius;
    double power_usage_watts;
    double power_limit_watts;
    
    GPUState state;
    time_t last_updated;
} GPUInfo;

typedef struct {
    int gpu_id;
    pid_t process_id;
    char process_name[256];
    uint64_t memory_used_mb;
    double gpu_utilization;
    time_t started_at;
} GPUProcess;

typedef struct {
    int gpu_id;
    char instance_id[64];
    uint64_t memory_allocated_mb;
    double compute_share;
    bool exclusive;
    time_t allocated_at;
} GPUAllocation;

typedef struct {
    GPUInfo gpus[MAX_GPUS_PER_NODE];
    int gpu_count;
    
    GPUAllocation *allocations;
    int allocation_count;
    int allocation_capacity;
    
    bool enable_mig;
    bool enable_mps;
    bool enable_time_slicing;
    
    pthread_mutex_t lock;
    bool is_initialized;
} GPUManager;

typedef struct {
    int total_gpus;
    int available_gpus;
    int allocated_gpus;
    uint64_t total_memory_mb;
    uint64_t available_memory_mb;
    uint64_t allocated_memory_mb;
    double average_utilization;
    double average_temperature;
} GPUStats;

typedef struct {
    int gpu_count;
    uint64_t memory_per_gpu_mb;
    bool exclusive;
    double min_compute_capability;
    GPUVendor preferred_vendor;
} GPURequest;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int gpu_init(void);
int gpu_shutdown(void);
bool gpu_is_available(void);

/*
 * ============================================================================
 * GPU DISCOVERY
 * ============================================================================
 */

int gpu_discover_devices(void);
int gpu_get_device_count(void);
GPUInfo *gpu_get_device_info(int gpu_id);
GPUInfo **gpu_list_devices(int *count);

int gpu_refresh_device_info(int gpu_id);
int gpu_refresh_all_devices(void);

/*
 * ============================================================================
 * GPU ALLOCATION
 * ============================================================================
 */

GPUAllocation *gpu_allocate(
    const char *instance_id,
    const GPURequest *request
);

int gpu_deallocate(const char *instance_id);
GPUAllocation *gpu_get_allocation(const char *instance_id);
GPUAllocation **gpu_list_allocations(int *count);

bool gpu_can_allocate(const GPURequest *request);
int gpu_find_best_gpu(const GPURequest *request);

/*
 * ============================================================================
 * GPU MONITORING
 * ============================================================================
 */

int gpu_update_utilization(int gpu_id);
double gpu_get_utilization(int gpu_id);
double gpu_get_memory_utilization(int gpu_id);
double gpu_get_temperature(int gpu_id);
double gpu_get_power_usage(int gpu_id);

GPUProcess **gpu_get_processes(int gpu_id, int *count);
int gpu_kill_process(int gpu_id, pid_t process_id);

/*
 * ============================================================================
 * GPU FEATURES
 * ============================================================================
 */

bool gpu_supports_mig(int gpu_id);
bool gpu_supports_mps(int gpu_id);
bool gpu_supports_nvlink(int gpu_id);

int gpu_enable_mig(int gpu_id);
int gpu_disable_mig(int gpu_id);
int gpu_create_mig_instance(int gpu_id, int compute_slices, int memory_slices);

int gpu_enable_mps(int gpu_id);
int gpu_disable_mps(int gpu_id);

/*
 * ============================================================================
 * GPU SCHEDULING
 * ============================================================================
 */

int gpu_schedule_instance(Instance *instance, const GPURequest *request);
int gpu_reschedule_instance(Instance *instance);
int gpu_migrate_instance(Instance *instance, int target_gpu_id);

int gpu_set_compute_mode(int gpu_id, int mode);
int gpu_set_persistence_mode(int gpu_id, bool enabled);

/*
 * ============================================================================
 * GPU HEALTH
 * ============================================================================
 */

bool gpu_is_healthy(int gpu_id);
int gpu_run_diagnostics(int gpu_id);
int gpu_reset_device(int gpu_id);

bool gpu_check_ecc_errors(int gpu_id, uint64_t *error_count);
int gpu_clear_ecc_errors(int gpu_id);

/*
 * ============================================================================
 * GPU TOPOLOGY
 * ============================================================================
 */

int gpu_get_topology_affinity(int gpu_id1, int gpu_id2);
bool gpu_has_nvlink(int gpu_id1, int gpu_id2);
int gpu_get_pcie_bandwidth(int gpu_id);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

GPUStats gpu_get_stats(void);
int gpu_export_metrics(const char *filepath);

/*
 * ============================================================================
 * VENDOR-SPECIFIC FUNCTIONS
 * ============================================================================
 */

int gpu_nvidia_init(void);
int gpu_nvidia_get_device_info(int device_id, GPUInfo *info);
int gpu_nvidia_allocate(int device_id, uint64_t memory_mb);

int gpu_amd_init(void);
int gpu_amd_get_device_info(int device_id, GPUInfo *info);
int gpu_amd_allocate(int device_id, uint64_t memory_mb);

int gpu_intel_init(void);
int gpu_intel_get_device_info(int device_id, GPUInfo *info);
int gpu_intel_allocate(int device_id, uint64_t memory_mb);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

void gpu_parse_nvidia_smi_output(const char *output, GPUInfo *info);
void gpu_parse_rocm_smi_output(const char *output, GPUInfo *info);
void gpu_parse_intel_smi_output(const char *output, GPUInfo *info);

int gpu_execute_command(const char *command, char *output, size_t output_size);
GPUVendor gpu_detect_vendor(int device_id);

#endif // CONSTELLATION_GPU_H
