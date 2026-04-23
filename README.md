# 🚀 RADS v0.0.12 "QUASAR II"

---

## 🆕 What's New in v0.0.12

### ✨ New Standard Library Modules

**os Module 🖥️:**
- `os.name()` - OS name (Linux, Darwin, etc.)
- `os.version()` - OS release/version string
- `os.arch()` - CPU architecture (x86_64, aarch64, etc.)
- `os.hostname()` - System hostname
- `os.cwd()` - Current working directory
- `os.pid()` / `os.ppid()` - Process/parent process ID
- `os.uid()` / `os.gid()` - User/group ID
- `os.env(name)` / `os.setenv(name, val)` / `os.unsetenv(name)` - Environment variables
- `os.sleep(ms)` - Sleep N milliseconds
- `os.cpu_count()` - Number of logical CPUs
- `os.pagesize()` - Memory page size
- `os.mem_total()` / `os.mem_available()` - Memory info
- `os.tmpdir()` - System temp directory
- `os.info()` - Comprehensive system info array
- `os.exit(code)` - Exit process

**FFI Module 🔗 (real dlopen/dlsym):**
- `ffi.load(path)` - Load shared library, returns handle ID
- `ffi.sym(handle, name)` - Look up symbol address
- `ffi.close(handle)` - Close library handle
- `ffi.error()` - Last dlerror message

**hash Module #️⃣:**
- `hash.fnv1a(str)` - FNV-1a 32-bit hash (hex string)
- `hash.fnv1a64(str)` - FNV-1a 64-bit hash (hex string)
- `hash.djb2(str)` - DJB2 hash
- `hash.crc32(str)` - CRC32 checksum
- `hash.adler32(str)` - Adler-32 checksum
- `hash.xorshift(str)` - Xorshift hash (64-bit)
- `hash.bucket(str, n)` - Hash bucket index for hash tables

**uuid Module 🔑:**
- `uuid.v4()` - Generate RFC 4122 v4 UUID
- `uuid.short()` - Generate 8-char short ID
- `uuid.nanoid(len)` - Generate NanoID-style string (default 21 chars)
- `uuid.is_valid(str)` - Validate UUID format

**sort Module 📊:**
- `sort.asc(arr)` - Sort array ascending (in-place)
- `sort.desc(arr)` - Sort array descending (in-place)
- `sort.sorted(arr)` - Return new sorted copy
- `sort.reverse(arr)` - Reverse array in-place
- `sort.copy(arr)` - Shallow copy of array
- `sort.min(arr)` / `sort.max(arr)` - Find min/max element
- `sort.sum(arr)` - Sum numeric array elements
- `sort.shuffle(arr)` - Fisher-Yates shuffle

**New String Functions:**
- `str.format(template, args...)` - Format string with `{}` placeholders
- `str.interpolate(template, struct)` - Replace `{key}` with struct field values

**process Module ⚙️:**
- `process.exec(cmd)` - Execute command, return stdout
- `process.exec_status(cmd)` - Execute command, return [stdout, exit_code]
- `process.pid()` - Get current process ID
- `process.ppid()` - Get parent process ID
- `process.uid()` - Get user ID
- `process.sleep(secs)` - Sleep for N seconds
- `process.sleep_ms(ms)` - Sleep for N milliseconds
- `process.cwd()` - Get current working directory
- `process.chdir(path)` - Change directory
- `process.hostname()` - Get system hostname
- `process.cpu_count()` - Get number of CPU cores
- `process.uptime()` - Get process uptime in seconds
- `process.exit(code)` - Exit the process

### ✨ Major JSON Overhaul

**New Full-Featured JSON Parser & Serializer:**
- `json.parse(str)` - Full recursive JSON parser (objects, arrays, nested, unicode, escapes)
- `json.stringify(value)` - Serialize any value to compact JSON
- `json.stringify_pretty(value)` - Serialize to indented JSON
- `json.get(data, path)` - Get nested value by dot-path (e.g. `"user.address.city"`)
- `json.keys(obj)` - Get array of keys from parsed object
- `json.values(obj)` - Get array of values from parsed object
- `json.has_key(obj, key)` - Check if key exists
- `json.type_of(value)` - Get type name string

### ✨ Array Module — 12 New Functions
- `array.includes(arr, val)` - Check if value exists
- `array.index_of(arr, val)` - Find first index of value
- `array.last_index_of(arr, val)` - Find last index of value
- `array.flat(arr, depth)` - Flatten nested arrays
- `array.slice(arr, start, end)` - Extract sub-array (supports negative indices)
- `array.unique(arr)` - Remove duplicates
- `array.count(arr)` - Get array length
- `array.is_empty(arr)` - Check if empty
- `array.concat(a, b)` - Merge two arrays
- `array.push(arr, val)` - Append (mutating)
- `array.pop(arr)` - Remove and return last (mutating)
- `array.shift(arr)` - Remove and return first (mutating)

### ✨ String Module — 10 New Functions
- `str.repeat(str, n)` - Repeat string N times
- `str.pad_left(str, width, fill)` - Left-pad string
- `str.pad_right(str, width, fill)` - Right-pad string
- `str.char_at(str, index)` - Get character at index
- `str.to_int(str)` - Parse string to integer
- `str.to_float(str)` - Parse string to float
- `str.is_empty(str)` - Check if empty string
- `str.reverse(str)` - Reverse string
- `str.count(str, substr)` - Count occurrences
- `str.title(str)` - Title-case string
- `str.capitalize(str)` - Capitalize first character

### ✨ Math Module — 40+ Functions (Expanded)
**Constants:**
- `math.PI()` - π constant
- `math.E()` - e constant
- `math.INF()` - Positive infinity
- `math.NAN()` - Not-a-Number

**Basic Operations:**
- `math.abs(x)` - Absolute value
- `math.min(a, b)` - Minimum of two values
- `math.max(a, b)` - Maximum of two values
- `math.sign(x)` - Sign of number (-1, 0, 1)
- `math.clamp(x, min, max)` - Clamp to range
- `math.lerp(a, b, t)` - Linear interpolation

**Rounding:**
- `math.floor(x)` - Round down
- `math.ceil(x)` - Round up
- `math.round(x)` - Round to nearest
- `math.trunc(x)` - Truncate toward zero

**Powers & Roots:**
- `math.sqrt(x)` - Square root
- `math.cbrt(x)` - Cube root
- `math.pow(x, y)` - x raised to power y
- `math.hypot(a, b)` - Hypotenuse

**Trigonometric (radians):**
- `math.sin(x)`, `math.cos(x)`, `math.tan(x)`
- `math.asin(x)`, `math.acos(x)`, `math.atan(x)`
- `math.atan2(y, x)` - Arc tangent of y/x

**Hyperbolic:**
- `math.sinh(x)`, `math.cosh(x)`, `math.tanh(x)`

**Logarithmic & Exponential:**
- `math.exp(x)` - e^x
- `math.log(x)` - Natural logarithm
- `math.log2(x)` - Base-2 logarithm
- `math.log10(x)` - Base-10 logarithm

**Modulo:**
- `math.fmod(x, y)` - Floating-point remainder

**Special Value Checks:**
- `math.isnan(x)` - Check for NaN
- `math.isinf(x)` - Check for infinity
- `math.isfinite(x)` - Check for finite number

**Angle Conversion:**
- `math.deg_to_rad(deg)` - Degrees to radians
- `math.rad_to_deg(rad)` - Radians to degrees

**Array Operations:**
- `math.sum(arr)` - Sum of array
- `math.avg(arr)` - Average of array
- `math.median(arr)` - Median of array

**Random (legacy, prefer `random` module):**
- `math.random()` - Random float [0, 1)
- `math.random_int(max)` - Random integer [0, max]
- `math.random_int(min, max)` - Random integer [min, max]

---

## 🆕 What's New in v0.0.11

### ✨ New Standard Library Modules

**datetime Module 🕐:**
- `datetime.now()` - Current timestamp as struct
- `datetime.format(ts, fmt)` - Format timestamp to string
- `datetime.parse(str, fmt)` - Parse string to timestamp
- `datetime.unix(ts)` - Get Unix timestamp
- `datetime.from_unix(unix)` - Create from Unix timestamp
- `datetime.add(ts, days, hours, minutes, seconds)` - Add time
- `datetime.diff(ts1, ts2)` - Time difference in seconds
- `datetime.weekday(ts)` - Day of week (0=Sunday)

**regex Module 🔍:**
- `regex.compile(pattern)` - Compile regex pattern (cached)
- `regex.match(regex, str)` - Boolean match check
- `regex.findall(regex, str)` - Find all matches
- `regex.search(regex, str)` - First match with position
- `regex.replace(regex, str, replacement)` - Replace matches
- `regex.split(regex, str)` - Split by pattern
- `regex.groups(regex, str)` - Extract capture groups

**Test Framework 🧪:**
- `test.describe(name, fn)` - Define test suite
- `test.it(name, fn)` - Define test case
- `test.expect(val).toBe(expected)` - Strict equality
- `test.expect(val).toEqual(expected)` - Deep equality
- `test.expect(val).toBeTruthy()` - Truthy check
- `test.expect(val).toBeFalsy()` - Falsy check
- `test.expect(val).toBeNull()` - Null check
- `test.beforeEach(fn)` - Setup before each test
- `test.afterEach(fn)` - Cleanup after each test
- `test.run()` - Execute all tests

**env Module 🌍:**
- `env.get(name)` - Get environment variable
- `env.set(name, value)` - Set environment variable
- `env.has(name)` - Check if variable exists
- `env.unset(name)` - Remove environment variable
- `env.list()` - Get all variables as struct
- `env.paths()` - Parse PATH into array
- `env.home()` - Get HOME directory
- `env.cwd()` - Get current working directory
- `env.user()` - Get current username
- `env.shell()` - Get current shell

**cli Module 🖥️:**
- `cli.args()` - Get raw argument array
- `cli.program()` - Get program name
- `cli.count()` - Get argument count
- `cli.has_flag(name)` - Check if flag present
- `cli.flag(name)` - Get flag value
- `cli.flags()` - Get all flags
- `cli.parse(spec)` - Parse with specification

**csv Module 📊:**
- `csv.parse(str)` - Parse CSV string
- `csv.stringify(data)` - Convert to CSV string
- `csv.read(path)` - Read CSV file
- `csv.write(path, data)` - Write CSV file

**base64 Module 🔐:**
- `base64.encode(str)` - Encode string to base64
- `base64.decode(str)` - Decode base64 to string
- `base64.encode_bytes(arr)` - Encode byte array
- `base64.decode_bytes(str)` - Decode to byte array

**random Module 🎲:**
- `random.seed(n)` - Seed the PRNG (auto-seeded from time+PID+clock)
- `random.int(min, max)` - Random integer in [min, max]
- `random.float()` - Random float in [0.0, 1.0)
- `random.range(min, max)` - Random float in [min, max)
- `random.choice(arr)` - Random element from array
- `random.shuffle(arr)` - Shuffle array in-place
- `random.sample(arr, n)` - N unique random elements
- `random.bool()` - Random true/false
- `random.uuid()` - Generate random UUID string

**path Module 📁:**
- `path.join(...parts)` - Join path components
- `path.basename(path)` - Get filename
- `path.dirname(path)` - Get directory name
- `path.extname(path)` - Get file extension
- `path.stem(path)` - Get filename without extension
- `path.is_absolute(path)` - Check if path is absolute
- `path.normalize(path)` - Normalize path (resolve `..`, `.`)
- `path.split(path)` - Split into (dirname, basename)
- `path.splitext(path)` - Split into (root, ext)
- `path.sep` - Path separator (`/` or `\\`)
- `path.cwd()` - Current working directory
- `path.home()` - User home directory
- `path.glob(pattern)` - Find files matching pattern

**time Module ⏱️:**
- `time.sleep(seconds)` - Sleep for N seconds (float supported)
- `time.time()` - Unix timestamp in seconds
- `time.unix()` - Alias for time.time()
- `time.millis()` - Unix timestamp in milliseconds
- `time.micros()` - Unix timestamp in microseconds
- `time.perf_counter()` - High-resolution monotonic clock (benchmarking)
- `time.monotonic()` - Monotonic clock (never goes backward)
- `time.clock()` - Process CPU time
- `time.process_time()` - High-resolution process time
- `time.thread_time()` - Thread CPU time

### ✨ v0.0.10 Features (Still Available)

**datetime Module 🕐:**
- `datetime.now()` - Current timestamp as struct
- `datetime.format(ts, fmt)` - Format timestamp to string
- `datetime.parse(str, fmt)` - Parse string to timestamp
- `datetime.unix(ts)` - Get Unix timestamp
- `datetime.from_unix(unix)` - Create from Unix timestamp
- `datetime.add(ts, days, hours, minutes, seconds)` - Add time
- `datetime.diff(ts1, ts2)` - Time difference in seconds
- `datetime.weekday(ts)` - Day of week (0=Sunday)

**regex Module 🔍:**
- `regex.compile(pattern)` - Compile regex pattern (cached)
- `regex.match(regex, str)` - Boolean match check
- `regex.findall(regex, str)` - Find all matches
- `regex.search(regex, str)` - First match with position
- `regex.replace(regex, str, replacement)` - Replace matches
- `regex.split(regex, str)` - Split by pattern
- `regex.groups(regex, str)` - Extract capture groups

**Test Framework 🧪:**
- `test.describe(name, fn)` - Define test suite
- `test.it(name, fn)` - Define test case
- `test.expect(val).toBe(expected)` - Strict equality
- `test.expect(val).toEqual(expected)` - Deep equality
- `test.expect(val).toBeTruthy()` - Truthy check
- `test.expect(val).toBeFalsy()` - Falsy check
- `test.expect(val).toBeNull()` - Null check
- `test.beforeEach(fn)` - Setup before each test
- `test.afterEach(fn)` - Cleanup after each test
- `test.run()` - Execute all tests

### ✨ v0.0.9 Features (Still Available)

**Destructuring:**
- `turbo [a, b, c] = [1, 2, 3]` - Array destructuring
- `turbo [first, ...rest] = arr` - Rest elements in destructuring
- `turbo { name, age } = person` - Struct destructuring
- `turbo { x: px, y: py } = point` - Struct destructuring with rename

**Spread Operator:**
- `turbo combined = [...arr1, ...arr2]` - Spread arrays
- `turbo extended = [0, ...base, 30]` - Mix spread with elements

**Default Parameters:**
- `blast greet(name = "World")` - Functions with default values

**Optional Chaining:**
- `obj?.property` - Safe property access
- `arr?.[index]` - Safe index access

**Nullish Coalescing:**
- `x ?? default` - Return default only if x is null

### ✨ v0.0.8 Features (Still Available)

**Error Handling:**
- `try-catch-finally` - Full exception handling support
- `throw` - Throw custom exceptions

**Import System:**
- `import "file.rads"` - Import and use external modules

**Type Inspection:**
- `typeof` operator - Get runtime type information

### ✨ New Standard Library Functions (40+)

**Array Operations:**
- `array.map()`, `array.filter()`, `array.reduce()` - Functional programming
- `array.find()`, `array.some()`, `array.every()` - Search operations
- `array.sort()`, `array.reverse()` - In-place mutations

**String Operations:**
- `string.split()`, `string.join()` - Split and join
- `string.trim()`, `string.upper()`, `string.lower()` - Case manipulation
- `string.replace()`, `string.substring()` - String transformation

**Math Functions:**
- `math.min()`, `math.max()`, `math.clamp()` - Value bounds
- `math.floor()`, `math.ceil()`, `math.round()` - Rounding
- `math.random()`, `math.srand()` - Random number generation

**Filesystem Operations:**
- `fs.readdir()`, `fs.stat()` - Directory and file info
- `fs.mkdir()`, `fs.rmdir()`, `fs.unlink()` - Directory/file operations
- `fs.exists()`, `fs.rename()`, `fs.chmod()` - File management
- `fs.getcwd()` - Current directory

**Async Utilities:**
- `async_utils.parallel()`, `async_utils.series()` - Parallel/sequential execution
- `async_utils.retry()` - Retry with exponential backoff
- `async_utils.timeout()`, `async_utils.delay()` - Time-based operations
- `async_utils.each()` - Async iteration

### 🐛 Bug Fixes
- Fixed array printing in string concatenation
- Fixed anonymous function crash (NULL name handling)
- Fixed `array.sort()`/`array.reverse()` memory corruption
- Fixed duplicate function definitions in stdlib modules
- Fixed lexer keyword detection for try/catch/finally/throw

---

## ⚡ ONE COMMAND INSTALLATION

\`\`\`bash
curl -fsSL https://raw.githubusercontent.com/zarigata/RADS/main/install.sh | bash
\`\`\`

That is it! RADS downloads, builds itself, and installs automatically.

**Why This Works Everywhere:**
- ✅ Detects your Linux distribution automatically
- ✅ Installs all dependencies (libuv, readline, sqlite3, gcc, make)
- ✅ Builds from source with optimal settings
- ✅ Installs to /usr/local/bin for system-wide access
- ✅ Works on Debian, Ubuntu, Fedora, RHEL, CentOS, Arch, Alpine, and more

**After Installation:**
\`\`\`bash
# Verify it works
rads --version

# Start coding
rads
\`\`\`

---

## 🎯 WHAT IS RADS?

RADS (Rapid Asynchronous Data Server) is a blazing fast C-based programming language that combines raw C power with modern async capabilities.

**Perfect for:**
- 🖥️ Microservers & IoT (Raspberry Pi, embedded systems)
- ⚡ High-speed data processing (video, audio, image at CPU speeds)
- 🛠️ Reliable tooling (F2A file servers with precision)
- 🎨 Retro-futuristic aesthetics (Cyberpunk, Frutiger Aero)

**Language Power:**
\`\`\`rads
// Lightning fast with bytecode VM (3-5x speedup)
blast main() {
    turbo int x = 42;
    echo("x * 2 = " + (x * 2));  // JIT compiled!
}
\`\`\`

---

## ✨ v0.0.8 "SUPERNOVA" FEATURES

### 🛩️ Core Engine
- **Bytecode VM** - Stack-based virtual machine with 3-5x performance boost
- **Disassembler** - View compiled bytecode for debugging
- **Sampling Profiler** - Real-time profiling with flamegraph output
- **Hotspot Analysis** - Automatic performance bottleneck detection

### 🐛 Advanced Debugger
- **DAP Support** - Debug Adapter Protocol for VS Code, Neovim, etc.
- **Conditional Breakpoints** - Break when expressions are true
- **Variable Watch** - Monitor variables in real-time
- **Call Stack Inspection** - Navigate through function calls
- **Step Controls** - Step-over, step-into, step-out commands

### 🌐 Networking & Web
- **WebSocket Server** - Real-time bidirectional communication (128+ concurrent connections)
- **GraphQL Server** - Queries, mutations, subscriptions with JSON responses
- **HTTP Client** - Built-in HTTP request handling
- **HTTP Server** - Create web servers with minimal code

### 📊 Database Integration
- **SQLite3 Support** - Full database with transactions
- **Parameterized Queries** - Secure query building
- **Connection Pooling** - Efficient connection management

### 🎮 Development Tools
- **LSP Server** - Rust-based language server for code completion, go-to-definition, refactoring
- **RGB REPL** - Gaming keyboard aesthetic with rainbow cursor effects
- **GNU Readline** - Command history with arrow key navigation
- **Beautiful Errors** - Color-coded error messages with helpful suggestions

### 🛠️ Package Ecosystem
- **rstar** - Package manager for installing and managing RADS packages
- **radpkg** - Resource package manager for bundling assets
- **rads-mask** - Python to RADS transpiler (95%+ accuracy)
- **Registry** - Full npmjs.com-style package registry at https://rads.rads-lang.org

### 🧪 Type System
- **Types**: int, float, string, bool, array, struct, enum, function, pointer
- **Control Flow**: if/else, for, while, switch, break, continue, return
- **Functions**: Named and anonymous, multiple return values, default parameters
- **Structs**: Custom data types with member access
- **Enums**: Type-safe enumerations
- **Arrays**: Dynamic arrays with push, pop, length operations

---

## 📚 START CODING NOW

### Your First Program

\`\`\`rads
blast main() {
    echo("Hello, RADS World! 🚀");
}
\`\`\`

Save as hello.rads and run:
\`\`\`bash
rads hello.rads
\`\`\`

### Interactive REPL

\`\`\`bash
rads
\`\`\`

REPL Commands:
- Type RADS code directly and press Enter to execute
- .help - Show all commands
- .exit or .quit - Leave REPL
- Use arrow keys for command history
- Press Tab for auto-completion

### Example: Variables & Types

\`\`\`rads
blast main() {
    // Numbers
    turbo int age = 25;
    
    // Float
    turbo float price = 19.99;
    
    // String
    str name = "RADS";
    
    // Boolean
    turbo bool active = true;
    
    // Array
    array<int> numbers = [1, 2, 3, 4, 5];
    
    // Enum
    enum Status {
        Pending,
        InProgress,
        Completed
    }
    
    // Struct
    struct Point {
        int x;
        int y;
    }
    
    turbo Point origin = { x: 0, y: 0 };
    
    echo("Age: " + age);
    echo("Price: $" + price);
    echo("Name: " + name);
    echo("Active: " + active);
    echo("Count: " + length(numbers));
    echo("Origin: " + origin.x + ", " + origin.y);
}
\`\`\`

### Example: Control Flow

\`\`\`rads
blast main() {
    turbo int x = 10;
    
    // If-else
    if (x > 5) {
        echo("x is greater than 5");
    } else {
        echo("x is 5 or less");
    }
    
    // For loop
    for (int i = 0; i < 5; i = i + 1) {
        echo("Count: " + i);
    }
    
    // While loop
    turbo int count = 0;
    while (count < 3) {
        echo("While count: " + count);
        count = count + 1;
    }
    
    // Switch statement
    enum Day {
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday
    }
    
    turbo Day today = Day.Wednesday;
    
    switch (today) {
        case Monday:
            echo("It's Monday");
            break;
        case Wednesday:
            echo("It's Wednesday");
            break;
        default:
            echo("It's another day");
    }
}
\`\`\`

### Example: Functions

\`\`\`rads
blast int add(int a, int b) {
    return a + b;
}

blast int multiply(int x, int y) {
    return x * y;
}

blast main() {
    turbo int result = add(5, 3);
    echo("5 + 3 = " + result);
    
    // Anonymous function with default parameter
    blast greet(str name = "World") {
        echo("Hello, " + name + "!");
    }
    
    greet("RADS");
    greet("Developer");
}
\`\`\`

### Example: HTTP Server

\`\`\`rads
import web;

blast main() {
    web_init();
    
    // Start HTTP server on port 8080
    web_serve("0.0.0.0", 8080);
    
    echo("Server started on http://localhost:8080");
    echo("Press Ctrl+C to stop");
}
\`\`\`

### Example: WebSocket Server

\`\`\`rads
import websocket;

blast void handle_message(str msg) {
    echo("Received: " + msg);
}

blast main() {
    websocket_start("0.0.0.0", 8080, handle_message);
    echo("WebSocket server running on port 8080");
}
\`\`\`

### Example: Database Operations

\`\`\`rads
import db;

blast main() {
    db_init(":memory:");
    
    // Create table
    db_exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT);");
    
    // Insert data
    db_exec("INSERT INTO users (name, email) VALUES ('Alice', 'alice@example.com');");
    
    // Query data
    db_result result = db_query("SELECT * FROM users;");
    
    while (db_next_row(result)) {
        echo("User: " + db_get_string(result, 1));
    }
    
    db_close();
}
\`\`\`

### Example: Async Operations

\`\`\`rads
import async;

blast void callback(int result) {
    echo("Operation completed. Result: " + result);
}

blast main() {
    async_exec(callback);
    echo("Async operation started...");
}
\`\`\`

---

## 🛠️ API REFERENCE

### String Operations

\`\`\`rads
str concat(str a, str b)           // Concatenate strings
str len(str s)                    // Get string length
str substring(str s, int start, int end)  // Extract substring
str to_upper(str s)              // Convert to uppercase
str to_lower(str s)              // Convert to lowercase
str trim(str s)                   // Remove whitespace
str string.split(str s, str delim)    // Split into array
str string.join(array arr, str delim) // Join array into string
str string.replace(str s, str search, str replace) // Replace substrings
\`\`\`

### Math Functions

\`\`\`rads
float math_abs(float x)           // Absolute value
float math_floor(float x)         // Round down
float math_ceil(float x)          // Round up
float math_sqrt(float x)          // Square root
float math_pow(float base, float exp)  // Power
float math_sin(float x)            // Sine
float math_cos(float x)            // Cosine
float math_min(float a, float b)   // Minimum
float math_max(float a, float b)   // Maximum
float math_clamp(float val, float min, float max) // Clamp to range
float math_random()                // Random float [0, 1)
int math_random_int(int min, int max) // Random int in range
\`\`\`

### Array Functions

\`\`\`rads
int length(array arr)           // Get array length
int push(array arr, value)     // Add element to end
int pop(array arr)              // Remove and return last element
array array.sort(array arr)           // Sort array
array array.reverse(array arr)          // Reverse array
array array.map(array arr, fn callback)    // Transform elements
array array.filter(array arr, fn predicate) // Filter elements
value array.reduce(array arr, fn reducer, value initial) // Reduce to single value
value array.find(array arr, fn predicate)  // Find first match
bool array.some(array arr, fn predicate)   // Check if any match
bool array.every(array arr, fn predicate)  // Check if all match
\`\`\`

### Filesystem Functions

\`\`\`rads
array fs.readdir(str path)      // List directory contents
array fs.stat(str path)         // Get file info [size, is_dir, is_file, mode, mtime]
bool fs.unlink(str path)        // Delete file
bool fs.exists(str path)        // Check if path exists
bool fs.mkdir(str path)         // Create directory
bool fs.rmdir(str path)         // Remove empty directory
bool fs.rename(str old, str new) // Rename/move file
str fs.getcwd()                 // Get current working directory
bool fs.chmod(str path, int mode) // Change file permissions
\`\`\`

### Async Functions

\`\`\`rads
void async_utils.parallel(array funcs, fn callback) // Execute in parallel
void async_utils.series(array funcs, fn callback)   // Execute in sequence
void async_utils.retry(fn func, int count, fn callback) // Retry with backoff
void async_utils.timeout(fn func, int ms, fn callback)  // Timeout after time
void async_utils.delay(int ms, fn callback)       // Delay execution
void async_utils.each(array arr, fn iterator, fn callback) // Iterate async
\`\`\`

### I/O Functions

\`\`\`rads
void echo(str message)           // Print to stdout
void error(str message)          // Print to stderr
void input(str prompt)           // Read from stdin
int read_file(str path)         // Read file contents
void write_file(str path, str content)  // Write file
\`\`\`

### Network Functions

\`\`\`rads
void http_request(str url)        // Make HTTP request
void ws_send(str message)         // Send WebSocket message
void ws_connect(str url)          // Connect to WebSocket server
\`\`\`

### Database Functions

\`\`\`rads
void db_init(str path)            // Initialize database
void db_exec(str sql)             // Execute SQL
db_result db_query(str sql)        // Query database
bool db_next_row(db_result res)     // Move to next row
str db_get_string(db_result res, int col)  // Get string value
void db_close()                     // Close database
\`\`\`

---

## 📦 PACKAGE MANAGEMENT

### Install a Package

\`\`\`bash
rstar install package-name
\`\`\`

### List Packages

\`\`\`bash
rstar list
\`\`\`

### Search Packages

\`\`\`bash
rstar search database
rstar search graphql
\`\`\`

### Package Info

\`\`\`bash
rstar info package-name
\`\`\`

### Create Your Own Package

\`\`\`bash
# 1. Create package directory
mkdir my-rads-package
cd my-rads-package

# 2. Create package manifest
cat > rads.json << 'EOF'
{
  "name": "my-package",
  "version": "1.0.0",
  "description": "A useful RADS package",
  "main": "src/main.rads",
  "keywords": ["rads", "library"],
  "author": "Your Name",
  "license": "MIT",
  "rads_version": "0.0.8"
}
EOF

# 3. Add your RADS files
mkdir src
cat > src/main.rads << 'EOF'
blast main() {
    echo("Hello from my package!");
}
EOF

# 4. Publish
rstar publish
\`\`\`

---

## 🔧 COMPILER OPTIONS

### Basic Compilation

\`\`\`bash
# Compile and run
rads program.rads

# Compile to bytecode
rads --bytecode program.rads

# Enable optimization
rads -O2 program.rads

# Output assembly
rads --asm program.rads
\`\`\`

### Environment Variables

\`\`\`bash
RADS_PATH=/usr/local/lib/rads      # Additional library path
RADS_LOG_LEVEL=debug              # Logging level
RADS_ASYNC_THREADS=4              # Number of async threads
\`\`\`

---

## 📂 PROJECT STRUCTURE

\`\`\`
Rads/
├── bin/                    # Compiled tools
│   ├── rads             # Main compiler
│   ├── rstar           # Package manager
│   ├── radpkg          # Resource package manager
│   └── rads-mask       # Python transpiler
├── src/
│   ├── core/            # Compiler components (lexer, parser, AST, bytecode, interpreter, profiler, debugger)
│   ├── stdlib/          # Standard library (strings, arrays, math, I/O, async, network, DB, etc.)
│   └── vm/             # Bytecode VM
├── tests/                # Test suites
├── examples/             # Example programs
├── docs/                # Documentation
└── tools/                # Additional tools (LSP server, etc.)
\`\`\`

---

## 🔍 TROUBLESHOOTING

### Build Issues

**Problem**: readline/readline.h: No such file or directory

**Solution**:
\`\`\`bash
# Debian/Ubuntu
sudo apt-get install libreadline-dev

# Fedora/RHEL
sudo dnf install readline-devel

# macOS
brew install readline
\`\`\`

### Runtime Issues

**Problem**: "command not found" error

**Solution**:
\`\`\`bash
# Ensure RADS is in your PATH
export PATH=$PATH:/usr/local/bin

# Verify installation
which rads
rads --version
\`\`\`

### Performance Issues

**Problem**: Program running slowly

**Solution**:
\`\`\`bash
# Enable optimization
rads -O2 program.rads

# Use bytecode mode
rads --bytecode program.rads

# Profile with built-in profiler
rads --profile program.rads
\`\`\`

---

## 📚 LEARN MORE

- **Package Registry**: https://zarigata.github.io/RADS/ - Browse and install RADS packages
- **Documentation**: https://zarigata.github.io/RADS/documentation.html - Complete API reference and tutorials
- **GitHub Repository**: https://github.com/zarigata/RADS - Source code, issues, discussions

---

## 🤝 CONTRIBUTING

We welcome contributions! See CONTRIBUTING.md for guidelines.

- Code should follow existing style
- Add tests for new features
- Update documentation
- Keep changes minimal and focused

---

## 📜 LICENSE

MIT License - see LICENSE file for details.

---

## 🙏 ACKNOWLEDGMENTS

- libuv for excellent async I/O
- GNU readline for command-line editing
- SQLite for embedded database
- LLVM for compilation infrastructure
- The RADS community

---

**One command to rule them all. Start coding today!** 🚀
