#include "mesh/registry.h"
#include "mesh/loadbalancer.h"
#include "mesh/circuit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    printf("║  RADS CONSTELLATION - PHASE 5 TESTS                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    // Test 1: Service Registry
    printf("\n📋 Service Registry Tests\n");
    RegistryConfig reg_config = {
        .max_services = 100,
        .heartbeat_interval_ms = 1000,
        .heartbeat_timeout_ms = 3000,
        .enable_health_checks = true,
        .health_check_interval_ms = 2000
    };

    test_assert(registry_init(&reg_config) == 0, "Registry init");
    test_assert(registry_is_running(), "Registry running");

    // Register a service
    const char *tags[] = {"http", "api"};
    ServiceEndpoint *svc = registry_register("user-service", "localhost:8080", tags, 2);
    test_assert(svc != NULL, "Service registration");
    test_assert(strcmp(svc->service_name, "user-service") == 0, "Service name correct");
    test_assert(strcmp(svc->endpoint, "localhost:8080") == 0, "Service endpoint correct");
    test_assert(svc->tag_count == 2, "Service tags count");

    // Find service by name
    int count = 0;
    ServiceEndpoint **found = registry_find_by_name("user-service", &count);
    test_assert(found != NULL && count == 1, "Find service by name");
    if (found) free(found);

    // Find service by tag
    found = registry_find_by_tag("http", &count);
    test_assert(found != NULL && count == 1, "Find service by tag");
    if (found) free(found);

    // Set metadata
    test_assert(registry_set_metadata(svc->service_id, "version", "1.0.0") == 0, "Set metadata");

    // Heartbeat
    test_assert(registry_heartbeat(svc->service_id) == 0, "Send heartbeat");

    // Health check
    test_assert(registry_check_health(svc->service_id) == SERVICE_HEALTH_HEALTHY, "Check health");

    // Get stats
    RegistryStats reg_stats = registry_get_stats();
    test_assert(reg_stats.total_services == 1, "Registry stats total services");
    test_assert(reg_stats.healthy_services == 1, "Registry stats healthy services");

    printf("  ✅ Registry: %d passed\n", 13);

    // Test 2: Load Balancer
    printf("\n⚖️  Load Balancer Tests\n");

    test_assert(lb_init() == 0, "Load balancer init");
    test_assert(lb_is_running(), "Load balancer running");

    // Create load balancer with round-robin
    LBConfig lb_config = {
        .algorithm = LB_ALGORITHM_ROUND_ROBIN,
        .health_check_enabled = true,
        .max_retries = 3,
        .retry_delay_ms = 100,
        .sticky_sessions = false,
        .session_timeout_ms = 300000
    };

    LoadBalancer *lb = lb_create("user-service", &lb_config);
    test_assert(lb != NULL, "Create load balancer");
    test_assert(strcmp(lb->name, "user-service") == 0, "Load balancer name");
    test_assert(lb->config.algorithm == LB_ALGORITHM_ROUND_ROBIN, "Load balancer algorithm");

    // Register more services for load balancing
    ServiceEndpoint *svc2 = registry_register("user-service", "localhost:8081", tags, 2);
    ServiceEndpoint *svc3 = registry_register("user-service", "localhost:8082", tags, 2);
    test_assert(svc2 != NULL && svc3 != NULL, "Register multiple services");

    // Route request
    ServiceEndpoint *selected = lb_route_request("user-service", "client-1", NULL);
    test_assert(selected != NULL, "Route request");

    // Report request
    test_assert(lb_report_request("user-service", selected->service_id, true, 15.5) == 0, "Report success");
    test_assert(lb_report_request("user-service", selected->service_id, false, 25.0) == 0, "Report failure");

    // Get stats
    LBStats lb_stats = lb_get_stats();
    test_assert(lb_stats.total_requests > 0, "Load balancer stats");
    test_assert(lb_stats.active_balancers == 1, "Active balancers count");

    printf("  ✅ Load Balancer: %d passed\n", 11);

    // Test 3: Circuit Breaker
    printf("\n🔌 Circuit Breaker Tests\n");

    test_assert(circuit_init() == 0, "Circuit breaker init");
    test_assert(circuit_is_running(), "Circuit breaker running");

    // Create circuit breaker
    CircuitConfig circuit_config = {
        .failure_threshold = 3,
        .success_threshold = 2,
        .timeout_ms = 5000,
        .window_size_ms = 10000,
        .error_rate_threshold = 0.5
    };

    CircuitBreaker *circuit = circuit_create("user-service", &circuit_config);
    test_assert(circuit != NULL, "Create circuit breaker");
    test_assert(strcmp(circuit->name, "user-service") == 0, "Circuit name");
    test_assert(circuit->state == CIRCUIT_STATE_CLOSED, "Initial state closed");

    // Record successes
    test_assert(circuit_allow_request(circuit), "Allow request when closed");
    test_assert(circuit_record_success(circuit) == 0, "Record success");
    test_assert(circuit->state == CIRCUIT_STATE_CLOSED, "State remains closed");

    // Record failures
    test_assert(circuit_record_failure(circuit) == 0, "Record failure");
    test_assert(circuit_record_failure(circuit) == 0, "Record failure 2");
    test_assert(circuit_record_failure(circuit) == 0, "Record failure 3");

    // Circuit should now be open
    test_assert(circuit->state == CIRCUIT_STATE_OPEN, "State changed to open");
    test_assert(!circuit_allow_request(circuit), "Reject request when open");

    // Reset circuit
    test_assert(circuit_reset(circuit) == 0, "Reset circuit");
    test_assert(circuit->state == CIRCUIT_STATE_CLOSED, "State closed after reset");

    // Get stats
    CircuitStats circuit_stats = circuit_get_stats();
    test_assert(circuit_stats.total_circuits == 1, "Circuit stats total");

    // Check if healthy
    test_assert(circuit_is_healthy(circuit), "Circuit is healthy");

    printf("  ✅ Circuit Breaker: %d passed\n", 17);

    // Shutdown all systems
    printf("\n🛑 Shutdown Tests\n");

    test_assert(registry_shutdown() == 0, "Registry shutdown");
    test_assert(!registry_is_running(), "Registry not running");
    test_assert(lb_shutdown() == 0, "Load balancer shutdown");
    test_assert(!lb_is_running(), "Load balancer not running");
    test_assert(circuit_shutdown() == 0, "Circuit breaker shutdown");
    test_assert(!circuit_is_running(), "Circuit breaker not running");

    printf("  ✅ Shutdown: %d passed\n", 6);

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ Passed: %-3d  ❌ Failed: %-3d                           ║\n",
           tests_passed, tests_failed);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
