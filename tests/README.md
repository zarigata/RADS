# RADS Tests

This directory contains test files for the RADS programming language.

## Test Files

- **test.rads** - Basic language tests
- **test_null.rads** - Null handling tests
- **test_stream.rads** - Stream type tests
- **test_output.txt** - Test output file (auto-generated)

## Running Tests

Run individual test files:
```bash
./rads tests/test.rads
```

Run all tests:
```bash
make test
```

## Adding New Tests

Test files should:
1. Use the `.rads` extension
2. Include clear test descriptions
3. Output expected results
4. Test a specific language feature or fix
