# String Module (stdlib_string)

The string module provides 29 functions for string manipulation and inspection, matching Python's string API.

## String Manipulation

### `str.length(str: string) → int`
Returns the length of the string.

```rads
str.length("hello")  // → 5
```

### `str.upper(str: string) → string`
Returns a copy of the string with all characters uppercase.

```rads
str.upper("Hello")  // → "HELLO"
```

### `str.lower(str: string) → string`
Returns a copy of the string with all characters lowercase.

```rads
str.lower("Hello")  // → "hello"
```

### `str.trim(str: string) → string`
Removes leading and trailing whitespace from the string.

```rads
str.trim("  hello  ")  // → "hello"
```

### `str.substring(str: string, start: int, length: int) → string`
Extracts a substring from `start` position with `length` characters.

```rads
str.substring("hello", 1, 3)  // → "ell"
```

### `str.replace(str: string, search: string, replace: string) → string`
Replaces all occurrences of `search` with `replace`.

```rads
str.replace("hello hello", "l", "x")  // → "hexxo hexxo"
```

### `str.repeat(str: string, count: int) → string`
Repeats the string `count` times.

```rads
str.repeat("ab", 3)  // → "ababab"
```

### `str.reverse(str: string) → string`
Returns the string reversed.

```rads
str.reverse("hello")  // → "olleh"
```

### `str.title(str: string) → string`
Returns a title-cased string (first character of each word uppercase).

```rads
str.title("hello world")  // → "Hello World"
```

### `str.capitalize(str: string) → string`
Returns the string with first character uppercase and the rest lowercase.

```rads
str.capitalize("HELLO")  // → "Hello"
```

### `str.split(str: string, delim: string) → array`
Splits the string by delimiter into an array of substrings.

```rads
str.split("a,b,c", ",")  // → ["a", "b", "c"]
str.split("hello world", " ")  // → ["hello", "world"]
```

### `str.join(arr: array) → string`
Concatenates an array of strings into a single string.

```rads
str.join(["a", "b", "c"])  // → "abc"
```

### `str.pad_left(str: string, width: int, char: string) → string`
Pads the string on the left with spaces (or `char` if provided) to reach `width`.

```rads
str.pad_left("hello", 10)  // → "     hello"
```

### `str.pad_right(str: string, width: int, char: string) → string`
Pads the string on the right with spaces (or `char` if provided) to reach `width`.

```rads
str.pad_right("hello", 10)  // → "hello     "
```

### `str.ljust(str: string, width: int) → string`
Left-justifies the string in a field of `width` characters (Python-compatible).

```rads
str.ljust("hello", 10)  // → "hello     "
```

### `str.rjust(str: string, width: int) → string`
Right-justifies the string in a field of `width` characters (Python-compatible).

```rads
str.rjust("hello", 10)  // → "     hello"
```

### `str.center(str: string, width: int) → string`
Centers the string in a field of `width` characters.

```rads
str.center("hello", 10)  // → "  hello   "
```

## String Inspection

### `str.contains(str: string, sub: string) → int`
Returns `1` if the string contains `sub`, `0` otherwise.

```rads
str.contains("hello", "ell")  // → 1
str.contains("hello", "xyz")  // → 0
```

### `str.starts_with(str: string, prefix: string) → int`
Returns `1` if the string starts with `prefix`, `0` otherwise.

```rads
str.starts_with("hello", "he")  // → 1
str.starts_with("hello", "lo")  // → 0
```

### `str.ends_with(str: string, suffix: string) → int`
Returns `1` if the string ends with `suffix`, `0` otherwise.

```rads
str.ends_with("hello", "lo")  // → 1
str.ends_with("hello", "he")  // → 0
```

### `str.is_empty(str: string) → int`
Returns `1` if the string is empty, `0` otherwise.

```rads
str.is_empty("")  // → 1
str.is_empty("hello")  // → 0
```

### `str.isdigit(str: string) → int`
Returns `1` if all characters in the string are digits, `0` otherwise.

```rads
str.isdigit("12345")  // → 1
str.isdigit("123a5")  // → 0
```

### `str.isalpha(str: string) → int`
Returns `1` if all characters in the string are alphabetic, `0` otherwise.

```rads
str.isalpha("hello")  // → 1
str.isalpha("hello123")  // → 0
```

### `str.isalnum(str: string) → int`
Returns `1` if all characters in the string are alphanumeric, `0` otherwise.

```rads
str.isalnum("hello123")  // → 1
str.isalnum("hello 123")  // → 0
```

### `str.char_at(str: string, index: int) → string`
Returns the character at `index` as a single-character string.

```rads
str.char_at("hello", 1)  // → "e"
```

### `str.find(str: string, sub: string) → int`
Returns the index of the first occurrence of `sub`, or `-1` if not found.

```rads
str.find("hello", "ll")  // → 2
str.find("hello", "xyz")  // → -1
```

### `str.rfind(str: string, sub: string) → int`
Returns the index of the last occurrence of `sub`, or `-1` if not found.

```rads
str.rfind("hello hello", "ll")  // → 8
str.rfind("hello", "xyz")  // → -1
```

### `str.count(str: string, sub: string) → int`
Returns the number of (non-overlapping) occurrences of `sub` in the string.

```rads
str.count("hello hello", "l")  // → 4
```

## Type Conversion

### `str.to_int(str: string) → int`
Parses the string as an integer.

```rads
str.to_int("42")  // → 42
```

### `str.to_float(str: string) → float`
Parses the string as a floating-point number.

```rads
str.to_float("3.14")  // → 3.14
```

## Complete Function List (29 functions)

**Manipulation:**
1. str.length
2. str.upper
3. str.lower
4. str.trim
5. str.substring
6. str.replace
7. str.repeat
8. str.reverse
9. str.title
10. str.capitalize
11. str.split
12. str.join
13. str.pad_left
14. str.pad_right
15. str.ljust
16. str.rjust
17. str.center

**Inspection:**
18. str.contains
19. str.starts_with
20. str.ends_with
21. str.is_empty
22. str.isdigit
23. str.isalpha
24. str.isalnum
25. str.char_at
26. str.find
27. str.rfind
28. str.count

**Type Conversion:**
29. str.to_int
30. str.to_float

## Notes

- All string functions return new strings (strings are immutable in RADS)
- Indexing is 0-based
- Empty strings are handled gracefully (most functions return null or -1 for empty input)
- The string module is designed to match Python's string API for familiarity
