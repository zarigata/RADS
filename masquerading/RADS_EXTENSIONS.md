# RADS-Specific Extensions Guide

**Version:** 0.0.5  
**Status:** Draft  
**Last Updated:** 2026-01-13

---

## 🎯 INTRODUCTION

When using RADS Masquerading, ~90% of your familiar language syntax works automatically. However, RADS provides powerful features that have no equivalent in traditional languages. This guide covers the **~10% of RADS-specific syntax** you need to learn to unlock RADS's full potential.

---

## 📚 EXTENSION CATEGORIES

### 1. Constellation (Distributed Computing)
### 2. FFI (Foreign Function Interface)
### 3. Low-Level Memory Operations
### 4. Performance Hints & Optimization
### 5. Resource Management (`defer`)

---

## 🌌 1. CONSTELLATION - DISTRIBUTED COMPUTING

**What it is:** RADS's built-in distributed computing framework that lets you run code across multiple machines/cores without complex setup.

**Why you need it:** Traditional languages require external frameworks (Celery, Ray, Dask) for distributed computing. RADS has it built-in.

---

### Basic Async Execution

**Python/JavaScript equivalent:** `async/await`, `Promise`

**RADS:**
```rads
import constellation;

@constellation.async
fn fetch_data(url) {
    let response = net.http_get(url);
    return json.decode(response.body);
}

@constellation.async
fn main() {
    let data = await fetch_data("https://api.example.com");
    io.print(data);
}

main();
```

**Key Differences:**
- Use `@constellation.async` decorator instead of `async` keyword
- Use `await` keyword (same as Python/JS)
- No need for event loop setup (RADS handles it)

---

### Parallel Execution

**Python equivalent:** `asyncio.gather()`, `multiprocessing.Pool`  
**JavaScript equivalent:** `Promise.all()`

**RADS:**
```rads
import constellation;

@constellation.parallel
fn process_batch(items) {
    let results = constellation.gather([
        process_item(items[0]),
        process_item(items[1]),
        process_item(items[2])
    ]);
    return results;
}
```

**Advanced: Distribute Across Nodes**
```rads
@constellation.distribute(nodes=8, strategy="round_robin")
fn process_large_dataset(data) {
    let results = [];
    for (let item in data) {
        let processed = expensive_computation(item);
        array.push(results, processed);
    }
    return results;
}
```

**Parameters:**
- `nodes`: Number of worker nodes to use
- `strategy`: Distribution strategy
  - `"round_robin"`: Distribute evenly
  - `"work_stealing"`: Dynamic load balancing
  - `"locality_aware"`: Keep related data together

---

### Map-Reduce Pattern

**Python equivalent:** `multiprocessing.Pool.map()`, PySpark  
**JavaScript equivalent:** Array methods with workers

**RADS:**
```rads
import constellation;

@constellation.map_reduce
fn word_count(documents) {
    // Map phase: Count words in each document
    let map_fn = fn(doc) {
        let words = string.split(doc, " ");
        let counts = {};
        for (let word in words) {
            counts[word] = (counts[word] || 0) + 1;
        }
        return counts;
    };
    
    // Reduce phase: Combine counts
    let reduce_fn = fn(counts1, counts2) {
        for (let word in counts2) {
            counts1[word] = (counts1[word] || 0) + counts2[word];
        }
        return counts1;
    };
    
    return constellation.map_reduce(documents, map_fn, reduce_fn);
}
```

---

### Constellation Configuration

**RADS:**
```rads
import constellation;

// Configure cluster
constellation.config({
    "nodes": 16,
    "memory_per_node": "8GB",
    "network": "high_bandwidth",
    "fault_tolerance": true
});

// Check cluster status
let status = constellation.status();
io.print("Active nodes: " + status.active_nodes);
io.print("Total memory: " + status.total_memory);
```

---

## 🔌 2. FFI - FOREIGN FUNCTION INTERFACE

**What it is:** Call C libraries directly from RADS without writing wrapper code.

**Why you need it:** Access existing C libraries (OpenSSL, SQLite, etc.) without reimplementation.

---

### Basic FFI Declaration

**Python equivalent:** `ctypes`, `cffi`  
**JavaScript equivalent:** `node-ffi`, `napi`

**RADS:**
```rads
import ffi;

// Load library
let libm = ffi.load("libm.so");

// Declare function signature
@ffi.extern(libm, "sqrt")
fn c_sqrt(x: float) -> float;

// Use it
let result = c_sqrt(16.0);
io.print(result);  // 4.0
```

---

### Complex Types

**Structs:**
```rads
import ffi;

// Define C struct
@ffi.struct
class Point {
    x: float;
    y: float;
}

// Declare C function that uses struct
@ffi.extern("libgraphics.so", "distance")
fn c_distance(p1: Point, p2: Point) -> float;

// Use it
let p1 = Point{x: 0.0, y: 0.0};
let p2 = Point{x: 3.0, y: 4.0};
let dist = c_distance(p1, p2);
io.print(dist);  // 5.0
```

**Pointers:**
```rads
import ffi;

@ffi.extern("libc.so", "malloc")
fn c_malloc(size: int) -> ffi.pointer;

@ffi.extern("libc.so", "free")
fn c_free(ptr: ffi.pointer) -> void;

// Allocate memory
let ptr = c_malloc(1024);
defer c_free(ptr);  // Auto-free when scope exits

// Use memory
ffi.write(ptr, 0, 42);
let value = ffi.read(ptr, 0);
io.print(value);  // 42
```

---

### Callbacks

**RADS:**
```rads
import ffi;

// C library expects a callback: void (*callback)(int)
@ffi.callback
fn my_callback(value: int) -> void {
    io.print("Callback received: " + value);
}

// Pass callback to C function
@ffi.extern("libevents.so", "register_callback")
fn register_callback(cb: ffi.callback) -> void;

register_callback(my_callback);
```

---

## 🧠 3. LOW-LEVEL MEMORY OPERATIONS

**What it is:** Direct memory access for performance-critical code.

**Why you need it:** Zero-copy operations, custom memory layouts, interfacing with hardware.

---

### Manual Memory Management

**C equivalent:** `malloc/free`  
**Rust equivalent:** `unsafe` blocks

**RADS:**
```rads
import mem;

@mem.unsafe
fn process_large_buffer(size: int) {
    // Allocate raw memory
    let buffer = mem.alloc(size);
    defer mem.free(buffer);
    
    // Write to memory
    for (let i = 0; i < size; i = i + 1) {
        mem.write_byte(buffer, i, i % 256);
    }
    
    // Read from memory
    let sum = 0;
    for (let i = 0; i < size; i = i + 1) {
        sum = sum + mem.read_byte(buffer, i);
    }
    
    return sum;
}
```

**Safety:**
- `@mem.unsafe` required for manual memory operations
- Compiler warns about potential memory leaks
- Use `defer` to ensure cleanup

---

### Zero-Copy Operations

**RADS:**
```rads
import mem;

@mem.unsafe
fn zero_copy_transfer(source: array, dest: array) {
    // Get raw pointers
    let src_ptr = mem.ptr(source);
    let dst_ptr = mem.ptr(dest);
    let size = array.length(source) * mem.sizeof(source[0]);
    
    // Direct memory copy (no intermediate allocation)
    mem.copy(dst_ptr, src_ptr, size);
}
```

---

### Memory-Mapped Files

**RADS:**
```rads
import mem;
import io;

@mem.unsafe
fn process_large_file(filename: string) {
    // Memory-map file (no loading into RAM)
    let mapped = mem.mmap(filename, "r");
    defer mem.munmap(mapped);
    
    // Access file as memory
    let size = mem.size(mapped);
    let sum = 0;
    for (let i = 0; i < size; i = i + 1) {
        sum = sum + mem.read_byte(mapped, i);
    }
    
    return sum;
}
```

---

## ⚡ 4. PERFORMANCE HINTS & OPTIMIZATION

**What it is:** Compiler directives to optimize specific code sections.

**Why you need it:** Fine-tune performance without rewriting in C.

---

### Inline Functions

**C equivalent:** `inline` keyword  
**Rust equivalent:** `#[inline]`

**RADS:**
```rads
@optimize.inline
fn fast_add(a: int, b: int) -> int {
    return a + b;
}

// Compiler will inline this function at call sites
let result = fast_add(2, 3);  // Becomes: let result = 2 + 3;
```

---

### Loop Unrolling

**RADS:**
```rads
@optimize.unroll(factor=4)
fn sum_array(arr: array) -> int {
    let sum = 0;
    for (let i = 0; i < array.length(arr); i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
}

// Compiler unrolls loop 4x for better performance
```

---

### SIMD Vectorization

**RADS:**
```rads
@optimize.simd
fn vector_add(a: array, b: array) -> array {
    let result = [];
    for (let i = 0; i < array.length(a); i = i + 1) {
        array.push(result, a[i] + b[i]);
    }
    return result;
}

// Compiler uses SIMD instructions (SSE, AVX, NEON)
```

---

### Hot Path Optimization

**RADS:**
```rads
@optimize.hot
fn critical_function(data: array) {
    // This function is called frequently
    // Compiler applies aggressive optimizations
    return array.reduce(data, fn(acc, x) { return acc + x; }, 0);
}
```

---

### Compile-Time Evaluation

**RADS:**
```rads
@optimize.const_eval
fn compute_lookup_table() -> array {
    let table = [];
    for (let i = 0; i < 256; i = i + 1) {
        array.push(table, i * i);
    }
    return table;
}

// Computed at compile-time, not runtime
let SQUARES = compute_lookup_table();
```

---

## 🔒 5. RESOURCE MANAGEMENT - `defer`

**What it is:** Automatic cleanup when scope exits (like Go's `defer` or Rust's `Drop`).

**Why you need it:** Replaces Python's `with` statement and JavaScript's try/finally.

---

### Basic Usage

**Python equivalent:**
```python
with open("file.txt") as f:
    data = f.read()
# File automatically closed
```

**RADS:**
```rads
import io;

fn read_file(filename: string) -> string {
    let f = io.open(filename, "r");
    defer io.close(f);  // Executes when function returns
    
    let data = io.read(f);
    return data;
}
```

---

### Multiple Defers

**RADS:**
```rads
fn complex_operation() {
    let file = io.open("data.txt", "r");
    defer io.close(file);
    
    let lock = mutex.acquire();
    defer mutex.release(lock);
    
    let memory = mem.alloc(1024);
    defer mem.free(memory);
    
    // All defers execute in reverse order (LIFO)
    // 1. mem.free(memory)
    // 2. mutex.release(lock)
    // 3. io.close(file)
}
```

---

### Defer with Error Handling

**RADS:**
```rads
fn safe_operation() {
    let resource = acquire_resource();
    defer release_resource(resource);
    
    if (some_error_condition()) {
        return error("Operation failed");
        // defer still executes before return
    }
    
    return success();
    // defer executes here too
}
```

---

## 📖 LEARNING PATH

### Stage 1: Start with Familiar Syntax (Week 1)
- Write Python/JavaScript as usual
- Let `rads-mask` handle conversion
- **Learning:** 0%

### Stage 2: Add `defer` (Week 2)
- Replace `with` statements with `defer`
- Understand scope-based cleanup
- **Learning:** 2%

### Stage 3: Use Constellation (Week 3-4)
- Replace `async/await` with `@constellation.async`
- Use `@constellation.parallel` for concurrency
- **Learning:** 5%

### Stage 4: Add FFI (Month 2)
- Call C libraries directly
- Use existing native code
- **Learning:** 7%

### Stage 5: Optimize (Month 3+)
- Add `@optimize.*` hints
- Use low-level memory operations
- **Learning:** 10%

---

## 🎓 CHEAT SHEET

### Constellation
```rads
@constellation.async              // Async function
@constellation.parallel           // Parallel execution
@constellation.distribute(nodes=N) // Distributed computing
constellation.gather([...])       // Wait for multiple tasks
```

### FFI
```rads
@ffi.extern(lib, "func")         // Declare C function
@ffi.struct                      // Define C struct
@ffi.callback                    // Create callback
ffi.load("lib.so")               // Load library
```

### Memory
```rads
@mem.unsafe                      // Unsafe memory operations
mem.alloc(size)                  // Allocate memory
mem.free(ptr)                    // Free memory
mem.mmap(file, mode)             // Memory-map file
```

### Optimization
```rads
@optimize.inline                 // Inline function
@optimize.unroll(factor=N)       // Unroll loops
@optimize.simd                   // SIMD vectorization
@optimize.hot                    // Hot path optimization
@optimize.const_eval             // Compile-time evaluation
```

### Resource Management
```rads
defer cleanup();                 // Execute on scope exit
```

---

## 🔍 EXAMPLES

### Example 1: Distributed Web Scraper

```rads
import constellation;
import net;
import array;

@constellation.distribute(nodes=10, strategy="work_stealing")
fn scrape_websites(urls: array) -> array {
    let results = [];
    
    for (let url in urls) {
        let response = net.http_get(url);
        let data = extract_data(response.body);
        array.push(results, data);
    }
    
    return results;
}

fn main() {
    let urls = load_urls("sites.txt");
    let data = scrape_websites(urls);
    save_results(data);
}
```

---

### Example 2: FFI with OpenSSL

```rads
import ffi;

let libssl = ffi.load("libssl.so");

@ffi.extern(libssl, "SHA256")
fn sha256(data: ffi.pointer, len: int, output: ffi.pointer) -> void;

fn hash_string(text: string) -> string {
    let input = string.to_bytes(text);
    let output = mem.alloc(32);  // SHA256 = 32 bytes
    defer mem.free(output);
    
    sha256(mem.ptr(input), array.length(input), output);
    
    return bytes_to_hex(output, 32);
}

let hash = hash_string("Hello, RADS!");
io.print(hash);
```

---

### Example 3: Optimized Image Processing

```rads
import mem;

@optimize.simd
@optimize.hot
fn apply_filter(image: array, width: int, height: int) -> array {
    let result = array.new(width * height);
    
    for (let y = 1; y < height - 1; y = y + 1) {
        for (let x = 1; x < width - 1; x = x + 1) {
            let sum = 0;
            
            // 3x3 kernel
            for (let ky = -1; ky <= 1; ky = ky + 1) {
                for (let kx = -1; kx <= 1; kx = kx + 1) {
                    let idx = (y + ky) * width + (x + kx);
                    sum = sum + image[idx];
                }
            }
            
            result[y * width + x] = sum / 9;
        }
    }
    
    return result;
}
```

---

## 🚀 NEXT STEPS

1. **Start Simple**: Use `defer` to replace `with` statements
2. **Add Concurrency**: Use `@constellation.async` for async code
3. **Optimize**: Add `@optimize.*` hints to hot paths
4. **Go Native**: Use FFI for C library integration
5. **Master RADS**: Write native RADS for maximum performance

---

## 📚 ADDITIONAL RESOURCES

- [Constellation Deep Dive](../CONSTELLATION.md)
- [FFI Reference](../FFI.md)
- [Memory Management Guide](../MEMORY.md)
- [Optimization Techniques](../OPTIMIZATION.md)

---

**Questions?** Join Discord or open a GitHub issue!
