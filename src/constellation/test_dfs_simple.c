#include "dfs/dht.h"
#include "dfs/dfs.h"
#include "dfs/locks.h"
#include <stdio.h>

int tests_passed = 0;
int tests_failed = 0;

void test_assert(int cond, const char *msg) {
    if (!cond) {
        printf("  ❌ FAIL: %s\n", msg);
        tests_failed++;
    } else {
        tests_passed++;
    }
}

int main() {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  RADS CONSTELLATION - PHASE 4 TESTS                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    printf("\n📦 DHT Tests\n");
    DHTConfig dht_config = {150, 3};
    test_assert(dht_init(&dht_config) == 0, "DHT init");
    test_assert(dht_is_running(), "DHT running");
    test_assert(dht_shutdown() == 0, "DHT shutdown");
    printf("  ✅ DHT: %d/3 passed\n", tests_passed);

    printf("\n📁 DFS Tests\n");
    DFSConfig dfs_config = {3, 1, 0, 1024, 5000};
    test_assert(dfs_init(&dfs_config) == 0, "DFS init");
    test_assert(dfs_is_running(), "DFS running");
    test_assert(dfs_shutdown() == 0, "DFS shutdown");
    printf("  ✅ DFS: %d/6 passed\n", tests_passed - 3);

    printf("\n🔒 Lock Tests\n");
    LockConfig lock_config = {1000, 2000, 500};  // Shorter intervals
    test_assert(locks_init(&lock_config) == 0, "Locks init");
    test_assert(locks_is_running(), "Locks running");
    test_assert(locks_shutdown() == 0, "Locks shutdown");
    printf("  ✅ Locks: %d/9 passed\n", tests_passed - 6);

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ Passed: %-3d  ❌ Failed: %-3d                           ║\n",
           tests_passed, tests_failed);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
