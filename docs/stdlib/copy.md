# copy — Shallow and Deep Copy

Object copying utilities, equivalent to Python's `copy` module.

## Functions

### copy.copy(value)
Return a shallow copy of `value`. For arrays, creates a new array with copies of string elements but shared references to nested arrays/structs.

```
let original = [1, 2, 3]
let shallow = copy.copy(original)
shallow[0] = 99
// original[0] is still 1
```

### copy.deepcopy(value)
Return a deep copy of `value`. Recursively copies all nested arrays, structs, and strings.

```
let nested = [[1, 2], [3, 4]]
let deep = copy.deepcopy(nested)
deep[0][0] = 99
// nested[0][0] is still 1
```

### copy.is_primitive(value)
Return `true` if the value is a primitive type (null, bool, int, float) that doesn't need copying.

```
copy.is_primitive(42)      // true
copy.is_primitive("hello") // false
copy.is_primitive([1,2])   // false
```

## Comparison to Python

| Python | RADS |
|--------|------|
| `copy.copy(x)` | `copy.copy(x)` |
| `copy.deepcopy(x)` | `copy.deepcopy(x)` |
