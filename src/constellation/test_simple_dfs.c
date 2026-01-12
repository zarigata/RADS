#include "dfs/dht.h"
#include <stdio.h>

int main() {
    printf("Starting DHT test...\n");
    
    DHTConfig config = {150, 3};
    int result = dht_init(&config);
    
    printf("DHT init result: %d\n", result);
    printf("DHT running: %d\n", dht_is_running());
    
    dht_shutdown();
    
    printf("DHT shutdown complete\n");
    return 0;
}
