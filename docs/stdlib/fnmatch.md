# fnmatch — Filename Pattern Matching

Unix-style filename pattern matching, equivalent to Python's `fnmatch` module.

## Functions

### fnmatch.fnmatch(name, pattern)
Test whether `name` matches `pattern`. Returns `true` or `false`.

```
fnmatch.fnmatch("hello.txt", "*.txt")       // true
fnmatch.fnmatch("data.csv", "*.txt")         // false
fnmatch.fnmatch("file1.py", "file?.py")      // true
fnmatch.fnmatch("test.py", "test[0-9].py")   // false
fnmatch.fnmatch("test3.py", "test[0-9].py")  // true
```

### fnmatch.fnmatchcase(name, pattern)
Case-sensitive match (same as `fnmatch` in RADS).

```
fnmatch.fnmatchcase("README.md", "*.md")  // true
```

### fnmatch.filter(names, pattern)
Return a new array containing only the strings from `names` that match `pattern`.

```
let files = ["main.c", "test.py", "readme.md", "utils.py"]
let py_files = fnmatch.filter(files, "*.py")
// py_files == ["test.py", "utils.py"]
```

### fnmatch.translate(pattern)
Convert a glob pattern to an equivalent regex string.

```
fnmatch.translate("*.py")      // "^.*\\.py$"
fnmatch.translate("file?.txt") // "^file.\\.txt$"
```

## Pattern Syntax

| Pattern | Meaning |
|---------|---------|
| `*` | Matches everything |
| `?` | Matches any single character |
| `[seq]` | Matches any character in seq |
| `[!seq]` | Matches any character not in seq |

## Comparison to Python

| Python | RADS |
|--------|------|
| `fnmatch.fnmatch(n, p)` | `fnmatch.fnmatch(n, p)` |
| `fnmatch.fnmatchcase(n, p)` | `fnmatch.fnmatchcase(n, p)` |
| `fnmatch.filter(names, p)` | `fnmatch.filter(names, p)` |
| `fnmatch.translate(p)` | `fnmatch.translate(p)` |
