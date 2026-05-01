# RADS Standard Library: functools

Functional programming utilities inspired by Python's `functools` module.

## Functions (20)

### Transformation

| Function | Signature | Description |
|----------|-----------|-------------|
| `functools.map(f, array)` | `(func, array) → array` | Apply `f(element, index)` to each element |
| `functools.filter(f, array)` | `(func, array) → array` | Keep elements where `f(element, index)` is truthy |
| `functools.reduce(f, array, initial)` | `(func, array, any) → any` | Fold left: `f(f(f(initial, a[0]), a[1]), ...)` |
| `functools.flat_map(f, array)` | `(func, array) → array` | Map then flatten one level |
| `functools.zip_with(f, a, b)` | `(func, array, array) → array` | Apply `f` to paired elements |
| `functools.each(f, array)` | `(func, array) → null` | Iterate with side effects |

### Search

| Function | Signature | Description |
|----------|-----------|-------------|
| `functools.find(f, array)` | `(func, array) → any` | First element matching predicate, or `null` |
| `functools.find_index(f, array)` | `(func, array) → int` | Index of first match, or `-1` |

### Quantifiers

| Function | Signature | Description |
|----------|-----------|-------------|
| `functools.any(f, array)` | `(func, array) → bool` | True if any element satisfies predicate |
| `functools.all(f, array)` | `(func, array) → bool` | True if all elements satisfy predicate |
| `functools.none(f, array)` | `(func, array) → bool` | True if no element satisfies predicate |

### Grouping & Partitioning

| Function | Signature | Description |
|----------|-----------|-------------|
| `functools.group_by(key_fn, array)` | `(func, array) → array` | Group into `[[key, [items]], ...]` |
| `functools.count_by(key_fn, array)` | `(func, array) → array` | Count per key: `[[key, count], ...]` |
| `functools.partition(f, array)` | `(func, array) → [pass, fail]` | Split into two arrays |

### Combinators

| Function | Signature | Description |
|----------|-----------|-------------|
| `functools.zip(a, b)` | `(array, array) → array` | Pair elements into `[[a0,b0], [a1,b1], ...]` |
| `functools.enumerate(array, start?)` | `(array, int?) → array` | `[[0, val], [1, val], ...]` |
| `functools.chain(a, b, ...)` | `(*arrays) → array` | Concatenate arrays |
| `functools.compose(f, g)` | `(func, func) → array` | Compose marker `[f, g]` |
| `functools.unfold(f, initial)` | `(func, any) → array` | Build array from `f` returning `[value, next_state]` or `null` |

### Utility

| Function | Signature | Description |
|----------|-----------|-------------|
| `functools.identity(x)` | `(any) → any` | Returns argument unchanged |

## Examples

```rads
# Map and filter
doubled = functools.map(fn(x) x * 2, [1, 2, 3, 4])
# → [2, 4, 6, 8]

evens = functools.filter(fn(x) x % 2 == 0, [1, 2, 3, 4, 5, 6])
# → [2, 4, 6]

# Reduce (sum)
total = functools.reduce(fn(acc, x) acc + x, [1, 2, 3, 4], 0)
# → 10

# Zip two arrays
pairs = functools.zip(["a", "b", "c"], [1, 2, 3])
# → [["a", 1], ["b", 2], ["c", 3]]

# Group by
groups = functools.group_by(fn(x) x % 2, [1, 2, 3, 4, 5])
# → [[1, [1, 3, 5]], [0, [2, 4]]]

# Enumerate
for pair in functools.enumerate(["a", "b", "c"]) {
    print(pair[0].to_string() + ": " + pair[1])
}
# 0: a
# 1: b
# 2: c

# Partition
parts = functools.partition(fn(x) x > 3, [1, 2, 3, 4, 5])
# → [[4, 5], [1, 2, 3]]

# Chain (concatenate)
all = functools.chain([1, 2], [3, 4], [5])
# → [1, 2, 3, 4, 5]
```

## Performance

- All functions use single-pass iteration (O(n)) where possible
- Minimal memory allocation: pre-sized arrays, no intermediate structures
- `group_by` uses linear scan matching (suitable for typical small-to-medium datasets)
- `unfold` has a 10,000 element safety limit to prevent infinite loops
