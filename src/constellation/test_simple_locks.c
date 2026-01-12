#include "dfs/locks.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Starting locks test...\n");
    
    LockConfig config = {30000, 60000, 10000};
    int result = locks_init(&config);
    
    printf("Locks init result: %d\n", result);
    printf("Locks running: %d\n", locks_is_running());
    
    printf("Sleeping 2 seconds...\n");
    sleep(2);
    
    printf("Shutting down...\n");
    locks_shutdown();
    
    printf("Locks shutdown complete\n");
    return 0;
}
