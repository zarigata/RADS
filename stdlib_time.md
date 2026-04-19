# Time Module (`time`)

Cross-platform timing and sleep functions similar to Python's `time` module.

## Functions

### `time.sleep(seconds: number) -> null`

Pause execution for the specified number of seconds.

**Parameters:**
- `seconds` - Number of seconds to sleep (float supported for sub-second precision)

**Examples:**
```rads
time.sleep(1)        # Sleep for 1 second
time.sleep(0.5)      # Sleep for 0.5 seconds (500ms)
time.sleep(0.001)    # Sleep for 1 millisecond
```

**Notes:**
- Supports fractional seconds for sub-millisecond precision on supported platforms
- Uses `nanosleep()` on POSIX systems, `usleep()` as fallback
- Raises error if argument is negative

---

### `time.time() -> float`

Returns the current Unix timestamp in seconds since the epoch (January 1, 1970).

**Returns:** Current time in seconds as float with microsecond precision

**Examples:**
```rads
let now = time.time()
print("Current timestamp: " + str(now))
# Output: Current timestamp: 1713537600.123456
```

---

### `time.unix() -> float`

Alias for `time.time()` - returns the Unix timestamp.

**Returns:** Current Unix timestamp in seconds

**Examples:**
```rads
let ts = time.unix()
print("Unix timestamp: " + str(ts))
```

---

### `time.millis() -> float`

Returns the current Unix timestamp in milliseconds.

**Returns:** Current time in milliseconds as float

**Examples:**
```rads
let ms = time.millis()
print("Milliseconds since epoch: " + str(ms))
```

---

### `time.micros() -> float`

Returns the current Unix timestamp in microseconds.

**Returns:** Current time in microseconds as float

**Examples:**
```rads
let us = time.micros()
print("Microseconds since epoch: " + str(us))
```

---

### `time.perf_counter() -> float`

Returns a high-resolution monotonic clock for measuring short durations.

**Returns:** Fractional seconds from an undefined epoch

**Best for:**
- Benchmarking code execution time
- Measuring algorithm performance
- Profiling short operations

**Examples:**
```rads
let start = time.perf_counter()

# Do some work
let sum = 0
for i in range(1, 100000) {
    sum = sum + i
}

let elapsed = time.perf_counter() - start
print("Elapsed time: " + str(elapsed) + " seconds")
```

**Notes:**
- Monotonic: never decreases
- Highest resolution available on the system
- Not related to wall-clock time

---

### `time.monotonic() -> float`

Returns a monotonic clock that never goes backward.

**Returns:** Fractional seconds from system boot or undefined epoch

**Best for:**
- Measuring elapsed time in long-running applications
- Timeout calculations
- Rate limiting

**Examples:**
```rads
let timeout = time.monotonic() + 5.0  # 5 second timeout

while (time.monotonic() < timeout) {
    # Do work with timeout
}
```

**Notes:**
- Monotonic: never decreases, even when system clock changes
- More reliable than `time.time()` for elapsed time measurements
- Not suitable for calendar/datetime operations

---

### `time.clock() -> float`

Returns the CPU time used by the current process.

**Returns:** Process CPU time in seconds

**Examples:**
```rads
let cpu_before = time.clock()

# CPU-intensive work
let sum = 0
for i in range(1, 1000000) {
    sum = sum + i * i
}

let cpu_after = time.clock()
print("CPU time used: " + str(cpu_after - cpu_before) + " seconds")
```

**Notes:**
- Only counts time actually spent executing CPU instructions
- Sleep/IO wait time is not included
- Useful for profiling CPU usage

---

### `time.process_time() -> float`

High-resolution process time measurement.

**Returns:** Process CPU time in seconds with nanosecond precision

**Examples:**
```rads
let start = time.process_time()

# Do work
do_computation()

let elapsed = time.process_time() - start
print("Process time: " + str(elapsed) + "s")
```

**Notes:**
- Higher resolution than `time.clock()` on modern systems
- Similar to `time.clock()` but with better precision

---

### `time.thread_time() -> float`

Returns the CPU time used by the current thread.

**Returns:** Thread CPU time in seconds

**Examples:**
```rads
let thread_cpu = time.thread_time()
print("Thread CPU time: " + str(thread_cpu) + "s")
```

**Notes:**
- Measures CPU time for the calling thread only
- Falls back to process time on platforms without thread-specific timing
- Useful in multi-threaded applications

---

## Usage Examples

### Basic Timing

```rads
import "stdlib/time.rads"

# Get current time
let now = time.time()
print("Current time: " + str(now))

# Sleep for 2 seconds
time.sleep(2)

# Sleep for 0.5 seconds
time.sleep(0.5)
```

### Benchmarking

```rads
import "stdlib/time.rads"

let start = time.perf_counter()

# Algorithm to benchmark
let result = fibonacci(40)

let elapsed = time.perf_counter() - start
print("fibonacci(40) = " + str(result))
print("Time elapsed: " + str(elapsed) + " seconds")
```

### Timeout Pattern

```rads
import "stdlib/time.rads"

let deadline = time.monotonic() + 10.0  # 10 second timeout

while (time.monotonic() < deadline) {
    # Process with timeout
    if (process_item()) {
        break
    }
}
```

### Rate Limiting

```rads
import "stdlib/time.rads"

let rate_limit = 1.0  # 1 second between requests
let last_request = 0.0

function make_request(url) {
    let now = time.monotonic()
    let elapsed = now - last_request

    if (elapsed < rate_limit) {
        let wait_time = rate_limit - elapsed
        time.sleep(wait_time)
    }

    # Make the request
    last_request = time.monotonic()
    return http_get(url)
}
```

### CPU Profiling

```rads
import "stdlib/time.rads"

let cpu_start = time.process_time()

# Run algorithm
run_algorithm()

let cpu_end = time.process_time()
let cpu_time = cpu_end - cpu_start

let wall_start = time.perf_counter()
run_algorithm()
let wall_end = time.perf_counter()
let wall_time = wall_end - wall_start

print("CPU time: " + str(cpu_time) + "s")
print("Wall time: " + str(wall_time) + "s")
print("CPU utilization: " + str(cpu_time / wall_time * 100) + "%")
```

---

## Platform Notes

### POSIX Systems (Linux, macOS, BSD)
- Uses `clock_gettime()` when available with `CLOCK_MONOTONIC`, `CLOCK_REALTIME`, `CLOCK_PROCESS_CPUTIME_ID`, and `CLOCK_THREAD_CPUTIME_ID`
- Nanosecond precision on systems that support it
- `nanosleep()` for high-resolution sleep

### Fallback Platforms
- Uses `gettimeofday()` for wall-clock time when `clock_gettime()` is unavailable
- Uses `clock()` for CPU time when process/thread clocks are unavailable
- Uses `usleep()` for sleep when `nanosleep()` is unavailable
- Microsecond precision on fallback platforms

---

## Performance Characteristics

| Function | Resolution | Monotonic | Best For |
|----------|-----------|-----------|----------|
| `time.time()` | ~µs | No | Calendar operations |
| `time.perf_counter()` | ~ns | Yes | Benchmarking |
| `time.monotonic()` | ~ns | Yes | Timeouts |
| `time.clock()` | ~µs-ms | N/A | CPU profiling |
| `time.process_time()` | ~ns | N/A | CPU profiling |

---

## Comparison to Python `time` Module

| RADS | Python | Notes |
|------|--------|-------|
| `time.sleep()` | `time.sleep()` | Identical behavior |
| `time.time()` | `time.time()` | Identical behavior |
| `time.perf_counter()` | `time.perf_counter()` | Identical behavior |
| `time.monotonic()` | `time.monotonic()` | Identical behavior |
| `time.clock()` | `time.clock()` | Similar (deprecated in Python 3.8+) |
| `time.process_time()` | `time.process_time()` | Identical behavior |
| `time.thread_time()` | `time.thread_time()` | Identical behavior |
| `time.unix()` | N/A | Convenience alias in RADS |
| `time.millis()` | N/A | Convenience function in RADS |
| `time.micros()` | N/A | Convenience function in RADS |

---

## See Also

- [datetime module](stdlib_datetime.md) - Date and time operations
- [random module](stdlib_random.md) - Random number generation
- [Test suite](../tests/test_time.rads) - Complete test examples
