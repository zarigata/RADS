#!/bin/bash

RADS_TEST_DIR="tests"
RADS_BIN="./bin/rads"

total_passed=0
total_failed=0

echo "======================================"
echo "RADS v0.0.7 Test Runner"
echo "======================================"
echo ""

if [ ! -f "$RADS_BIN" ]; then
    echo "Error: $RADS_BIN not found. Please build RADS first."
    echo "Run: make"
    exit 1
fi

test_files=(
    "test.rads"
    "test_null.rads"
    "test_stream.rads"
    "test_structs.rads"
    "v0_0_7_comprehensive.rads"
    "test_debugger.rads"
    "test_typecheck.rads"
)

for test_file in "${test_files[@]}"; do
    test_path="$RADS_TEST_DIR/$test_file"

    if [ ! -f "$test_path" ]; then
        echo "⚠ Skipping $test_file (not found)"
        continue
    fi

    echo ""
    echo "─────────────────────────────────────────"
    echo "Running: $test_file"
    echo "─────────────────────────────────────────"

    if $RADS_BIN "$test_path" > /tmp/rads_test_output.txt 2>&1; then
        echo "✓ $test_file PASSED"
        ((total_passed++))
    else
        echo "✗ $test_file FAILED"
        cat /tmp/rads_test_output.txt
        ((total_failed++))
    fi
done

echo ""
echo "======================================"
echo "Test Summary"
echo "======================================"
echo "Total tests: $((${#test_files[@]} - 1))"
echo "Passed: $total_passed"
echo "Failed: $total_failed"
echo ""

if [ $total_failed -eq 0 ]; then
    echo "🎉 All tests passed!"
    exit 0
else
    echo "❌ Some tests failed"
    exit 1
fi
