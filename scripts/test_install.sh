#!/bin/bash
# Test script for RADS installation

echo "=== RADS Installation Test ===" > test_output.txt
echo "" >> test_output.txt

echo "Step 1: Checking if rads executable exists..." >> test_output.txt
if [ -f "./rads" ]; then
    echo "✓ rads executable found" >> test_output.txt
    ls -lh ./rads >> test_output.txt
else
    echo "✗ rads executable NOT found" >> test_output.txt
fi
echo "" >> test_output.txt

echo "Step 2: Testing rads --version..." >> test_output.txt
./rads --version >> test_output.txt 2>&1
echo "" >> test_output.txt

echo "Step 3: Testing rads --help..." >> test_output.txt
./rads --help >> test_output.txt 2>&1
echo "" >> test_output.txt

echo "Step 4: Running hello_world.rads..." >> test_output.txt
./rads examples/hello_world.rads >> test_output.txt 2>&1
echo "" >> test_output.txt

echo "Step 5: Running test.rads..." >> test_output.txt
./rads test.rads >> test_output.txt 2>&1
echo "" >> test_output.txt

echo "=== Test Complete ===" >> test_output.txt
cat test_output.txt
