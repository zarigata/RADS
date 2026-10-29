#!/bin/bash

RADS_TEST_DIR="tests"
RADS_BIN="./bin/rads"

total_passed=0
total_failed=0

echo "======================================"
echo "RADS v0.0.9 Test Runner"
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
    "test_structs.rads"
    "v0_0_7_comprehensive.rads"
    "test_debugger.rads"
    "test_typecheck.rads"
    "test_heapq.rads"
    "test_logging.rads"
    "test_statistics.rads"
    "test_nostalgia.rads"
    "test_regex.rads"
    "test_datetime.rads"
    "test_decimal.rads"
    "test_fractions.rads"
    "test_uaf_regression.rads"
    "test_itertools_lazy.rads"
    "test_template_literals.rads"
    "test_promise.rads"
    "test_numeric_mixed.rads"
    "test_sci_notation.rads"
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

    # Output goes to a per-suite temp file under TMPDIR (cron-safe, no /tmp litter)
    out_file="$TMPDIR/rads_suite_$test_file.out"
    [ -z "$TMPDIR" ] && out_file="/tmp/rads_suite_$test_file.out"

    if $RADS_BIN "$test_path" > "$out_file" 2>&1 \
       && ! grep -q "FAIL" "$out_file"; then
        echo "✓ $test_file PASSED"
        ((total_passed++))
    else
        echo "✗ $test_file FAILED"
        cat "$out_file"
        ((total_failed++))
    fi
done

echo ""
echo "======================================"
echo "Test Summary"
echo "======================================"
echo "Total tests: ${#test_files[@]}"
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
