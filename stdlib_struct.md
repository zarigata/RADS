# struct Module

Binary data packing and unpacking, compatible with Python's `struct` module.

## Format Characters

| Char | C Type          | Size  |
|------|-----------------|-------|
| `b`  | signed char     | 1     |
| `B`  | unsigned char   | 1     |
| `c`  | char            | 1     |
| `?`  | bool            | 1     |
| `h`  | signed short    | 2     |
| `H`  | unsigned short  | 2     |
| `i`  | signed int      | 4     |
| `I`  | unsigned int    | 4     |
| `l`  | signed long     | 4     |
| `L`  | unsigned long   | 4     |
| `q`  | signed long long| 8     |
| `Q`  | unsigned long long | 8  |
| `f`  | float           | 4     |
| `d`  | double          | 8     |

## Byte Order Prefixes

| Prefix | Order        |
|--------|-------------|
| `@`    | Native      |
| `=`    | Native      |
| `<`    | Little-endian |
| `>`    | Big-endian  |
| `!`    | Network (big) |

## Functions

### `struct.pack(fmt, v1, v2, ...)`
Pack values into a binary string according to format.

```rads
let data = struct.pack(">ih", 42, -1)
```

### `struct.unpack(fmt, data)`
Unpack binary data into an array of values.

```rads
let values = struct.unpack(">ih", data)
print(values[0])  // 42
print(values[1])  // -1
```

### `struct.calcsize(fmt)`
Return the number of bytes needed for the format.

```rads
let size = struct.calcsize(">ih")  // 6 (4 + 2)
```

### `struct.pack_into(fmt, buf, offset, v1, v2, ...)`
Pack values into an existing buffer at the given offset. Returns a new string.

```rads
let buf = struct.pack_into(">i", "\0\0\0\0\0\0\0\0", 0, 0x12345678)
```

### `struct.unpack_from(fmt, buf, [offset])`
Unpack from buffer starting at offset.

```rads
let val = struct.unpack_from(">i", buf, 0)
```

### `struct.iter_unpack(fmt, data)`
Iteratively unpack all records from data. Returns array of arrays.

```rads
let records = struct.iter_unpack("<if", binary_data)
for record in records {
    print(record[0])  // int
    print(record[1])  // float
}
```
