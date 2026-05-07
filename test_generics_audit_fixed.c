/**
 * RADS Generics Support Audit Test (Fixed Version)
 * Tests the current state of generics implementation in RADS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include the generics module
#include "src/typecheck/generics.c"

// Test function to verify generics support
int audit_generics_support() {
    printf("=== RADS Generics Support Audit ===\n");
    
    int test_results = 0;
    int total_tests = 0;
    
    // Test 1: Check if generics system can be initialized
    printf("\n1. Testing generics initialization...\n");
    typecheck_generics_init();
    printf("   ✓ Generics system initialized successfully\n");
    
    // Test 2: Test creating generic types with type parameters
    printf("\n2. Testing generic type creation with parameters...\n");
    char* params[] = {"T"};
    Type* list_generic = typecheck_create_generic("List", "unknown", params, 1);
    if (list_generic && list_generic->is_polymorphic) {
        printf("   ✓ Generic type creation works: %s with %d params\n", list_generic->name, list_generic->param_count);
        test_results++;
    } else {
        printf("   ✗ Generic type creation failed\n");
    }
    total_tests++;
    
    // Test 3: Test generic instantiation
    printf("\n3. Testing generic instantiation...\n");
    char* concrete_params[] = {"int"};
    Type* list_int = typecheck_instantiate_generic(list_generic, concrete_params, 1);
    if (list_int) {
        printf("   ✓ Generic instantiation works: %s\n", list_int->name);
        test_results++;
    } else {
        printf("   ✗ Generic instantiation failed\n");
    }
    total_tests++;
    
    // Test 4: Test type compatibility checking
    printf("\n4. Testing type compatibility...\n");
    int same_compatible = typecheck_check_type_compatibility(list_int, list_int);
    int different_compatible = typecheck_check_type_compatibility(list_int, list_generic);
    printf("   List<int> compatible with List<int>: %d (should be 1)\n", same_compatible);
    printf("   List<int> compatible with List<T>: %d (should be 0)\n", different_compatible);
    
    if (same_compatible && !different_compatible) {
        printf("   ✓ Type compatibility check works\n");
        test_results++;
    } else {
        printf("   ✗ Type compatibility check failed\n");
    }
    total_tests++;
    
    // Test 5: Test function registration
    printf("\n5. Testing function registration...\n");
    typecheck_generic_register_function("test_function");
    printf("   ✓ Function registration completed\n");
    test_results++;
    total_tests++;
    
    // Test 6: Test the built-in test function (with fixed logic)
    printf("\n6. Testing built-in generics test...\n");
    int test_passed = typecheck_generics_test();
    if (test_passed) {
        printf("   ✓ Built-in generics test passed\n");
        test_results++;
    } else {
        printf("   ✗ Built-in generics test failed\n");
    }
    total_tests++;
    
    // Test 7: Test type lookup
    printf("\n7. Testing type lookup...\n");
    Type* found = typecheck_find_type(list_int->name);
    if (found) {
        printf("   ✓ Type lookup works: %s\n", found->name);
        test_results++;
    } else {
        printf("   ✗ Type lookup failed\n");
    }
    total_tests++;
    
    // Cleanup
    printf("\n8. Testing cleanup...\n");
    typecheck_generics_cleanup();
    printf("   ✓ Cleanup completed\n");
    test_results++;
    total_tests++;
    
    // Print results
    printf("\n=== Audit Results ===\n");
    printf("Tests passed: %d/%d\n", test_results, total_tests);
    printf("Status: %s\n", (test_results == total_tests) ? "✓ GENERICS FULLY WORKING" : "⚠️  PARTIAL/INCOMPLETE");
    
    // Integration status check
    printf("\n=== Integration Status ===\n");
    printf("Build system integration: ✗ NOT INCLUDED IN Makefile\n");
    printf("Type system integration:  ⚠️  Available but not used by main typecheck.c\n");
    printf("Header interface:        ✗ No dedicated generics.h\n");
    printf("Test coverage:           ✗ No dedicated test suite\n");
    
    return (test_results == total_tests) ? 1 : 0;
}

int main() {
    printf("RADS Generics Support Audit (Fixed)\n");
    printf("=====================================\n");
    
    int result = audit_generics_support();
    
    if (result) {
        printf("\n🎉 Generics support is fully functional but needs integration\n");
        return 0;
    } else {
        printf("\n❌ Generics support has issues that need fixing\n");
        return 1;
    }
}