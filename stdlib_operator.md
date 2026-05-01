# RADS Standard Library: operator

Standard operators as callable functions, inspired by Python's `operator` module.
Essential for use with `functools.map`, `functools.filter`, and `functools.reduce`.

## Functions (37)

### Comparison

| Function | Equivalent | Description |
|----------|-----------|-------------|
| `operator.eq(a, b)` | `a == b` | Equal |
| `operator.ne(a, b)` | `a != b` | Not equal |
| `operator.lt(a, b)` | `a < b` | Less than |
| `operator.le(a, b)` | `a <= b` | Less than or equal |
| `operator.gt(a, b)` | `a > b` | Greater than |
| `operator.ge(a, b)` | `a >= b` | Greater than or equal |

### Arithmetic

| Function | Equivalent | Description |
|----------|-----------|-------------|
| `operator.add(a, b)` | `a + b` | Addition (numbers) / concatenation (strings) |
| `operator.sub(a, b)` | `a - b` | Subtraction |
| `operator.mul(a, b)` | `a * b` | Multiplication / string repetition |
| `operator.truediv(a, b)` | `a / b` | True division (always float) |
| `operator.floordiv(a, b)` | `a // b` | Floor division (always int) |
| `operator.mod(a, b)` | `a % b` | Modulo (Python-style floor mod) |
| `operator.neg(a)` | `-a` | Negation |
| `operator.pos(a)` | `+a` | Unary plus |
| `operator.abs(a)` | `abs(a)` | Absolute value |

### Bitwise

| Function | Equivalent | Description |
|----------|-----------|-------------|
| `operator.band(a, b)` | `a & b` | Bitwise AND |
| `operator.bor(a, b)` | `a \| b` | Bitwise OR |
| `operator.bxor(a, b)` | `a ^ b` | Bitwise XOR |
| `operator.bnot(a)` | `~a` | Bitwise NOT |
| `operator.shl(a, b)` | `a << b` | Shift left |
| `operator.shr(a, b)` | `a >> b` | Shift right |

### Logical

| Function | Equivalent | Description |
|----------|-----------|-------------|
| `operator.and(a, b)` | `a && b` | Logical AND |
| `operator.or(a, b)` | `a \|\| b` | Logical OR |
| `operator.not(a)` | `!a` | Logical NOT |

### Sequence

| Function | Description |
|----------|-------------|
| `operator.concat(a, b)` | Concatenate strings or arrays |
| `operator.contains(seq, val)` | Check if value is in string/array |
| `operator.count_of(array, val)` | Count occurrences in array |
| `operator.index_of(array, val)` | Find first index of value, or -1 |

### Type Checks

| Function | Returns `true` for |
|----------|-------------------|
| `operator.is_number(x)` | int or float |
| `operator.is_string(x)` | string |
| `operator.is_array(x)` | array |
| `operator.is_bool(x)` | bool |
| `operator.is_null(x)` | null |
| `operator.is_function(x)` | function |

### Utility

| Function | Description |
|----------|-------------|
| `operator.item(seq, index)` | Index into array or string (supports negative indices) |
| `operator.length(x)` | Length of string or array |
| `operator.min(a, b)` | Smaller of two values |
| `operator.max(a, b)` | Larger of two values |

## Examples

```rads
# Use with functools
sum = functools.reduce(operator.add, [1, 2, 3, 4], 0)
# → 10

product = functools.reduce(operator.mul, [1, 2, 3, 4], 1)
# → 24

# Sort with custom comparator using operator
names = ["charlie", "alice", "bob"]

# Type checking
operator.is_number(42)       # true
operator.is_string("hello")  # true
operator.is_array([1, 2])    # true
operator.is_null(null)       # true

# Negative indexing
operator.item([10, 20, 30], -1)  # → 30

# Python-style floor mod
operator.mod(-7, 3)  # → 2 (Python behavior, not C's -1)

# Contains
operator.contains("hello world", "world")  # → true
operator.contains([1, 2, 3], 2)            # → true
```

## Notes

- All comparison operators use a unified `compare_values` function that handles mixed int/float comparisons
- `operator.mod` uses Python-style floor modulo (result has same sign as divisor), not C's truncation
- `operator.item` supports negative indices (Python-style)
- Bitwise operators require integer arguments
