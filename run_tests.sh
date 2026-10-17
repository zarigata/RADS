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
    "test_pythonic_syntax.rads"
    "test_stream.rads"
    "test_structs.rads"
    "v0_0_7_comprehensive.rads"
    "test_debugger.rads"
    "test_typecheck.rads"
    "test_recursion.rads"
    "test_array_uaf.rads"
    "test_heapq.rads"
    "test_logging.rads"
    "test_statistics.rads"
    "test_math.rads"
    "test_string.rads"
    "test_bisect.rads"
    "test_itertools.rads"
    "test_time.rads"
    "test_struct.rads"
    "test_cruise_array.rads"
    "test_comprehension.rads"   # run 24: list & dict comprehensions
    "test_dict.rads"            # run 23: dicts (was never wired into the runner)
    "test_str_array.rads"
    "test_regex.rads"            # run 26: regex.replace overflow + zero-width parity, search uninit crash
    "test_v0.0.12.rads"
    "test_http.rads"
    "test_enum.rads"            # run 33: bare member resolution + explicit values
    "test_bc_concat.rads"       # run 40: BC_CONCAT fused-chain regression guard (run 39's 8 probe shapes)
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

    if $RADS_BIN "$test_path" > /tmp/rads_test_output.txt 2>&1 \
       && ! grep -q "Assertion failed" /tmp/rads_test_output.txt; then
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
echo "REPL Regression (run 27)"
echo "======================================"

# Run 27: the REPL must register blast/struct/enum lines and keep their ASTs
# alive (per-line arena). Old bug: functions never registered ("Undefined
# function"), and after registration the per-line ast_free dangled func_node.
repl_probe="$TMPDIR/rads_repl_probe.txt"
if [ -z "$TMPDIR" ]; then repl_probe="/tmp/rads_repl_probe.txt"; fi
cat > "$repl_probe" <<'EOF'
struct Pt { str tag; i32 n; }
blast mk(t) { return Pt { tag: t, n: 7 }; }
blast add(a, b) { return a + b; }
echo add(2, 3);
turbo Pt p = mk("arena");
echo p.tag;
blast fib(n) { if (n < 2) { return n; } return fib(n-1) + fib(n-2); }
echo fib(15);
.exit
EOF
repl_out="$($RADS_BIN < "$repl_probe" 2>&1 | sed 's/\x1b\[[0-9;]*m//g')"
repl_ok=1
for expected in "5" "arena" "610"; do
    if ! echo "$repl_out" | grep -qx "$expected"; then
        echo "✗ REPL output missing '$expected'"
        repl_ok=0
    fi
done
if echo "$repl_out" | grep -q "Undefined function"; then
    echo "✗ REPL failed to register blast function"
    repl_ok=0
fi
if [ $repl_ok -eq 1 ]; then
    echo "✓ REPL function/struct registration PASSED"
else
    echo "$repl_out" | grep -v "^rads\[" | tail -20
    ((total_failed++))
fi

echo "REPL -i/--interactive flag (run 34)"
# Run 34: `rads -i FILE` must run FILE then share its globals with the REPL
# (python -i semantics): plain global, blast fn, struct, enum all reachable.
i_probe="$TMPDIR/rads_i_probe.txt"
if [ -z "$TMPDIR" ]; then i_probe="/tmp/rads_i_probe.txt"; fi
cat > "$i_probe.rads" <<'EOF'
turbo i32 secret = 42;
blast double(x) { return x * 2; }
struct Pt { str tag; i32 n; }
enum Color { RED = 10, GREEN = 20 }
EOF
cat > "$i_probe" <<'EOF'
echo secret;
echo double(21);
turbo Pt p = Pt { tag: "t", n: 5 };
echo p.tag;
echo Color.GREEN;
.exit
EOF
i_out="$($RADS_BIN -i "$i_probe.rads" < "$i_probe" 2>&1 | sed 's/\x1b\[[0-9;]*m//g')"
i_ok=1
for expected in "42" "42" "t" "20"; do
    if ! echo "$i_out" | grep -qx "$expected"; then
        echo "✗ -i REPL output missing '$expected'"
        i_ok=0
    fi
done
if ! echo "$i_out" | grep -q "globals from the executed file are live"; then
    echo "✗ -i session notice missing"
    i_ok=0
fi
# -t without a file must fail with its dedicated error (run 34)
if $RADS_BIN -t >/dev/null 2>&1; then
    echo "✗ -t without file should exit non-zero"
    i_ok=0
fi
if [ $i_ok -eq 1 ]; then
    echo "✓ -i shared-globals REPL + -t guard PASSED"
else
    echo "$i_out" | grep -v "^rads\[" | tail -20
    ((total_failed++))
fi
rm -f "$i_probe" "$i_probe.rads"

echo ""
echo "======================================"
echo "Test Summary"
echo "======================================"
echo "Total tests: $((${#test_files[@]}))"
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
