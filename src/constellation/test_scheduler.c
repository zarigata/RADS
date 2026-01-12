/*
 * RADS Constellation - Phase 3 Scheduler Tests (Simplified)
 */

#include "constellation.h"
#include "cluster/cluster.h"
#include "scheduler/scheduler.h"
#include "consensus/raft.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        printf("  ❌ FAIL: %s\n", msg); \
        tests_failed++; \
        return; \
    }

#define TEST_PASS() \
    printf("  ✅ PASS\n"); \
    tests_passed++;

void test_scheduler_init() {
    printf("\n🧪 Test: Scheduler initialization\n");
    int result = scheduler_init();
    TEST_ASSERT(result == 0, "Scheduler init failed");
    TEST_ASSERT(scheduler_is_running(), "Scheduler not running");
    TEST_PASS();
}

void test_raft_init() {
    printf("\n🧪 Test: Raft initialization\n");
    RaftConfig config = {150, 300, 50, 3};
    int result = raft_init("test-node", &config);
    TEST_ASSERT(result == 0, "Raft init failed");
    TEST_ASSERT(raft_is_running(), "Raft not running");
    RaftState state = raft_get_state();
    TEST_ASSERT(state == RAFT_STATE_FOLLOWER, "Should start as follower");
    TEST_PASS();
}

void test_cleanup() {
    printf("\n🧪 Test: Cleanup\n");
    raft_shutdown();
    scheduler_shutdown();
    printf("  ✅ PASS\n");
    tests_passed++;
}

int main() {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  RADS CONSTELLATION - PHASE 3 TESTS                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    test_scheduler_init();
    test_raft_init();
    test_cleanup();

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  TEST SUMMARY                                             ║\n");
    printf("║  ✅ Passed: %-3d  ❌ Failed: %-3d                           ║\n", 
           tests_passed, tests_failed);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
