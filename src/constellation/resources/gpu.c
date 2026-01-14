/*
 * RADS Constellation - GPU Resource Management Implementation
 * Phase 9 (Iguana): Nice to Have Features
 */

#include "gpu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static GPUManager g_gpu_manager = {0};

int gpu_execute_command(const char *command, char *output, size_t output_size) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        return -1;
    }
    
    size_t total_read = 0;
    while (total_read < output_size - 1 && 
           fgets(output + total_read, output_size - total_read, fp) != NULL) {
        total_read = strlen(output);
    }
    
    int status = pclose(fp);
    return WEXITSTATUS(status);
}

GPUVendor gpu_detect_vendor(int device_id) {
    char output[1024];
    
    if (gpu_execute_command("nvidia-smi -L 2>/dev/null", output, sizeof(output)) == 0) {
        if (strlen(output) > 0) {
            return GPU_VENDOR_NVIDIA;
        }
    }
    
    if (gpu_execute_command("rocm-smi --showproductname 2>/dev/null", output, sizeof(output)) == 0) {
        if (strlen(output) > 0) {
            return GPU_VENDOR_AMD;
        }
    }
    
    if (gpu_execute_command("xpu-smi discovery 2>/dev/null", output, sizeof(output)) == 0) {
        if (strlen(output) > 0) {
            return GPU_VENDOR_INTEL;
        }
    }
    
    return GPU_VENDOR_UNKNOWN;
}

int gpu_init(void) {
    if (g_gpu_manager.is_initialized) {
        return 0;
    }
    
    memset(&g_gpu_manager, 0, sizeof(GPUManager));
    pthread_mutex_init(&g_gpu_manager.lock, NULL);
    
    g_gpu_manager.allocation_capacity = 100;
    g_gpu_manager.allocations = calloc(g_gpu_manager.allocation_capacity, sizeof(GPUAllocation));
    if (!g_gpu_manager.allocations) {
        return -1;
    }
    
    g_gpu_manager.is_initialized = true;
    
    gpu_discover_devices();
    
    return 0;
}

int gpu_shutdown(void) {
    if (!g_gpu_manager.is_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    if (g_gpu_manager.allocations) {
        free(g_gpu_manager.allocations);
    }
    
    g_gpu_manager.is_initialized = false;
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    pthread_mutex_destroy(&g_gpu_manager.lock);
    
    return 0;
}

bool gpu_is_available(void) {
    return g_gpu_manager.is_initialized && g_gpu_manager.gpu_count > 0;
}

void gpu_parse_nvidia_smi_output(const char *output, GPUInfo *info) {
    if (!output || !info) {
        return;
    }
    
    info->vendor = GPU_VENDOR_NVIDIA;
    
    sscanf(output, "%*[^:]: %[^(]", info->name);
    
    char *mem_line = strstr(output, "MiB /");
    if (mem_line) {
        uint64_t used, total;
        if (sscanf(mem_line - 10, "%lu MiB / %lu MiB", &used, &total) == 2) {
            info->memory_used_mb = used;
            info->memory_total_mb = total;
            info->memory_free_mb = total - used;
        }
    }
    
    char *util_line = strstr(output, "GPU-Util");
    if (util_line) {
        sscanf(util_line, "GPU-Util : %lf%%", &info->utilization_gpu);
    }
    
    char *temp_line = strstr(output, "Temp");
    if (temp_line) {
        sscanf(temp_line, "Temp : %lf C", &info->temperature_celsius);
    }
    
    info->state = GPU_STATE_ACTIVE;
    info->last_updated = time(NULL);
}

int gpu_nvidia_get_device_info(int device_id, GPUInfo *info) {
    if (!info) {
        return -1;
    }
    
    char command[256];
    char output[4096];
    
    snprintf(command, sizeof(command),
             "nvidia-smi -i %d --query-gpu=name,memory.total,memory.used,memory.free,"
             "utilization.gpu,utilization.memory,temperature.gpu,power.draw,power.limit "
             "--format=csv,noheader,nounits 2>/dev/null",
             device_id);
    
    if (gpu_execute_command(command, output, sizeof(output)) != 0) {
        return -1;
    }
    
    info->device_id = device_id;
    info->vendor = GPU_VENDOR_NVIDIA;
    
    char name[MAX_GPU_NAME];
    uint64_t mem_total, mem_used, mem_free;
    double util_gpu, util_mem, temp, power_draw, power_limit;
    
    if (sscanf(output, "%[^,], %lu, %lu, %lu, %lf, %lf, %lf, %lf, %lf",
               name, &mem_total, &mem_used, &mem_free, &util_gpu, &util_mem,
               &temp, &power_draw, &power_limit) >= 9) {
        
        strncpy(info->name, name, MAX_GPU_NAME - 1);
        info->memory_total_mb = mem_total;
        info->memory_used_mb = mem_used;
        info->memory_free_mb = mem_free;
        info->utilization_gpu = util_gpu;
        info->utilization_memory = util_mem;
        info->temperature_celsius = temp;
        info->power_usage_watts = power_draw;
        info->power_limit_watts = power_limit;
        info->state = GPU_STATE_ACTIVE;
        info->last_updated = time(NULL);
        
        return 0;
    }
    
    return -1;
}

int gpu_discover_devices(void) {
    if (!g_gpu_manager.is_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    g_gpu_manager.gpu_count = 0;
    
    GPUVendor vendor = gpu_detect_vendor(0);
    
    if (vendor == GPU_VENDOR_NVIDIA) {
        char output[1024];
        if (gpu_execute_command("nvidia-smi -L 2>/dev/null | wc -l", output, sizeof(output)) == 0) {
            int count = atoi(output);
            
            for (int i = 0; i < count && i < MAX_GPUS_PER_NODE; i++) {
                GPUInfo *info = &g_gpu_manager.gpus[g_gpu_manager.gpu_count];
                if (gpu_nvidia_get_device_info(i, info) == 0) {
                    g_gpu_manager.gpu_count++;
                }
            }
        }
    } else if (vendor == GPU_VENDOR_AMD) {
        g_gpu_manager.gpu_count = 0;
    } else if (vendor == GPU_VENDOR_INTEL) {
        g_gpu_manager.gpu_count = 0;
    }
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    
    return g_gpu_manager.gpu_count;
}

int gpu_get_device_count(void) {
    return g_gpu_manager.gpu_count;
}

GPUInfo *gpu_get_device_info(int gpu_id) {
    if (!g_gpu_manager.is_initialized || gpu_id < 0 || gpu_id >= g_gpu_manager.gpu_count) {
        return NULL;
    }
    
    return &g_gpu_manager.gpus[gpu_id];
}

int gpu_refresh_device_info(int gpu_id) {
    if (!g_gpu_manager.is_initialized || gpu_id < 0 || gpu_id >= g_gpu_manager.gpu_count) {
        return -1;
    }
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    GPUInfo *info = &g_gpu_manager.gpus[gpu_id];
    
    if (info->vendor == GPU_VENDOR_NVIDIA) {
        gpu_nvidia_get_device_info(gpu_id, info);
    }
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    
    return 0;
}

int gpu_find_best_gpu(const GPURequest *request) {
    if (!g_gpu_manager.is_initialized || !request) {
        return -1;
    }
    
    int best_gpu = -1;
    uint64_t max_free_memory = 0;
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    for (int i = 0; i < g_gpu_manager.gpu_count; i++) {
        GPUInfo *info = &g_gpu_manager.gpus[i];
        
        if (info->state != GPU_STATE_ACTIVE && info->state != GPU_STATE_IDLE) {
            continue;
        }
        
        if (request->preferred_vendor != GPU_VENDOR_UNKNOWN && 
            info->vendor != request->preferred_vendor) {
            continue;
        }
        
        if (info->memory_free_mb < request->memory_per_gpu_mb) {
            continue;
        }
        
        if (request->exclusive) {
            bool has_allocation = false;
            for (int j = 0; j < g_gpu_manager.allocation_count; j++) {
                if (g_gpu_manager.allocations[j].gpu_id == i) {
                    has_allocation = true;
                    break;
                }
            }
            if (has_allocation) {
                continue;
            }
        }
        
        if (info->memory_free_mb > max_free_memory) {
            max_free_memory = info->memory_free_mb;
            best_gpu = i;
        }
    }
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    
    return best_gpu;
}

GPUAllocation *gpu_allocate(const char *instance_id, const GPURequest *request) {
    if (!g_gpu_manager.is_initialized || !instance_id || !request) {
        return NULL;
    }
    
    int gpu_id = gpu_find_best_gpu(request);
    if (gpu_id < 0) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    if (g_gpu_manager.allocation_count >= g_gpu_manager.allocation_capacity) {
        pthread_mutex_unlock(&g_gpu_manager.lock);
        return NULL;
    }
    
    GPUAllocation *allocation = &g_gpu_manager.allocations[g_gpu_manager.allocation_count];
    allocation->gpu_id = gpu_id;
    strncpy(allocation->instance_id, instance_id, sizeof(allocation->instance_id) - 1);
    allocation->memory_allocated_mb = request->memory_per_gpu_mb;
    allocation->compute_share = 1.0;
    allocation->exclusive = request->exclusive;
    allocation->allocated_at = time(NULL);
    
    g_gpu_manager.allocation_count++;
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    
    return allocation;
}

int gpu_deallocate(const char *instance_id) {
    if (!g_gpu_manager.is_initialized || !instance_id) {
        return -1;
    }
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    for (int i = 0; i < g_gpu_manager.allocation_count; i++) {
        if (strcmp(g_gpu_manager.allocations[i].instance_id, instance_id) == 0) {
            for (int j = i; j < g_gpu_manager.allocation_count - 1; j++) {
                g_gpu_manager.allocations[j] = g_gpu_manager.allocations[j + 1];
            }
            g_gpu_manager.allocation_count--;
            
            pthread_mutex_unlock(&g_gpu_manager.lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    return -1;
}

bool gpu_can_allocate(const GPURequest *request) {
    if (!request) {
        return false;
    }
    
    return gpu_find_best_gpu(request) >= 0;
}

double gpu_get_utilization(int gpu_id) {
    GPUInfo *info = gpu_get_device_info(gpu_id);
    return info ? info->utilization_gpu : 0.0;
}

double gpu_get_memory_utilization(int gpu_id) {
    GPUInfo *info = gpu_get_device_info(gpu_id);
    return info ? info->utilization_memory : 0.0;
}

double gpu_get_temperature(int gpu_id) {
    GPUInfo *info = gpu_get_device_info(gpu_id);
    return info ? info->temperature_celsius : 0.0;
}

bool gpu_is_healthy(int gpu_id) {
    GPUInfo *info = gpu_get_device_info(gpu_id);
    if (!info) {
        return false;
    }
    
    if (info->state == GPU_STATE_ERROR || info->state == GPU_STATE_OFFLINE) {
        return false;
    }
    
    if (info->temperature_celsius > 90.0) {
        return false;
    }
    
    return true;
}

GPUStats gpu_get_stats(void) {
    GPUStats stats = {0};
    
    if (!g_gpu_manager.is_initialized) {
        return stats;
    }
    
    pthread_mutex_lock(&g_gpu_manager.lock);
    
    stats.total_gpus = g_gpu_manager.gpu_count;
    
    double total_util = 0.0;
    double total_temp = 0.0;
    
    for (int i = 0; i < g_gpu_manager.gpu_count; i++) {
        GPUInfo *info = &g_gpu_manager.gpus[i];
        
        stats.total_memory_mb += info->memory_total_mb;
        stats.available_memory_mb += info->memory_free_mb;
        
        total_util += info->utilization_gpu;
        total_temp += info->temperature_celsius;
        
        if (info->state == GPU_STATE_ACTIVE || info->state == GPU_STATE_IDLE) {
            stats.available_gpus++;
        }
    }
    
    stats.allocated_gpus = g_gpu_manager.allocation_count;
    
    for (int i = 0; i < g_gpu_manager.allocation_count; i++) {
        stats.allocated_memory_mb += g_gpu_manager.allocations[i].memory_allocated_mb;
    }
    
    if (g_gpu_manager.gpu_count > 0) {
        stats.average_utilization = total_util / g_gpu_manager.gpu_count;
        stats.average_temperature = total_temp / g_gpu_manager.gpu_count;
    }
    
    pthread_mutex_unlock(&g_gpu_manager.lock);
    
    return stats;
}

bool gpu_supports_mig(int gpu_id) {
    GPUInfo *info = gpu_get_device_info(gpu_id);
    if (!info || info->vendor != GPU_VENDOR_NVIDIA) {
        return false;
    }
    
    return (info->compute_capability_major >= 8);
}

int gpu_nvidia_init(void) {
    return 0;
}

int gpu_amd_init(void) {
    return 0;
}

int gpu_intel_init(void) {
    return 0;
}
