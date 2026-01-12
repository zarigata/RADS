#include "dfs/dfs.h"
#include <stdio.h>

int main() {
    printf("Starting DFS test...\n");
    
    DFSConfig config = {3, DFS_CONSISTENCY_EVENTUAL, DFS_CONFLICT_LAST_WRITE_WINS, 1024, 5000};
    int result = dfs_init(&config);
    
    printf("DFS init result: %d\n", result);
    printf("DFS running: %d\n", dfs_is_running());
    
    dfs_shutdown();
    
    printf("DFS shutdown complete\n");
    return 0;
}
