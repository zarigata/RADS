# Python → RADS Masquerading Guide

**Version:** 0.0.5  
**Status:** Draft  
**Last Updated:** 2026-01-13

---

## 🐍 INTRODUCTION

This guide shows Python developers how to use RADS without learning a new syntax. Write Python, run RADS.

**Conversion Rate:** ~92% of Python code converts automatically  
**Learning Required:** ~8% for RADS-specific features

---

## 🚀 QUICK START

### Installation
```bash
# Install RADS with masquerading support
curl -sSL https://rads-lang.org/install.sh | bash
rads-mask --version
```

### Your First Conversion
```bash
# Write Python as usual
cat > hello.py << EOF
def greet(name):
    print(f"Hello, {name}!")
    
greet("World")
EOF

# Convert to RADS
rads-mask --from python hello.py -o hello.rads

# Run with RADS
rads hello.rads
```

**Generated RADS:**
```rads
import io;

fn greet(name) {
    io.print("Hello, " + name + "!");
}

greet("World");
```

---

## ✅ WHAT WORKS OUT-OF-THE-BOX

### 1. Basic Syntax (100% Compatible)

#### Variables
```python
# Python
x = 10
name = "Alice"
is_valid = True
data = None
```

```rads
// RADS (auto-generated)
let x = 10;
let name = "Alice";
let is_valid = true;
let data = null;
```

#### Functions
```python
# Python
def add(a, b):
    return a + b

def greet(name="World"):
    print(f"Hello, {name}")
```

```rads
// RADS
fn add(a, b) {
    return a + b;
}

fn greet(name = "World") {
    io.print("Hello, " + name);
}
```

#### Control Flow
```python
# Python
if x > 10:
    print("Large")
elif x > 5:
    print("Medium")
else:
    print("Small")

for i in range(10):
    print(i)

while x > 0:
    x -= 1
```

```rads
// RADS
if (x > 10) {
    io.print("Large");
} else if (x > 5) {
    io.print("Medium");
} else {
    io.print("Small");
}

for (let i in array.range(10)) {
    io.print(i);
}

while (x > 0) {
    x = x - 1;
}
```

---

### 2. Data Structures (95% Compatible)

#### Lists
```python
# Python
numbers = [1, 2, 3, 4, 5]
numbers.append(6)
numbers.pop()
length = len(numbers)
first = numbers[0]
last = numbers[-1]
```

```rads
// RADS
let numbers = [1, 2, 3, 4, 5];
array.push(numbers, 6);
array.pop(numbers);
let length = array.length(numbers);
let first = numbers[0];
let last = numbers[array.length(numbers) - 1];
```

#### Dictionaries
```python
# Python
person = {
    "name": "Alice",
    "age": 30,
    "city": "NYC"
}
person["email"] = "alice@example.com"
name = person.get("name", "Unknown")
```

```rads
// RADS
let person = {
    "name": "Alice",
    "age": 30,
    "city": "NYC"
};
person["email"] = "alice@example.com";
let name = map.get(person, "name", "Unknown");
```

#### Strings
```python
# Python
text = "Hello World"
upper = text.upper()
lower = text.lower()
parts = text.split(" ")
joined = " ".join(parts)
```

```rads
// RADS
let text = "Hello World";
let upper = string.upper(text);
let lower = string.lower(text);
let parts = string.split(text, " ");
let joined = string.join(parts, " ");
```

---

### 3. Standard Library (90% Compatible)

#### File I/O
```python
# Python
with open("data.txt", "r") as f:
    content = f.read()

with open("output.txt", "w") as f:
    f.write("Hello")
```

```rads
// RADS (using defer instead of with)
import io;

let f = io.open("data.txt", "r");
defer io.close(f);
let content = io.read(f);

let f2 = io.open("output.txt", "w");
defer io.close(f2);
io.write(f2, "Hello");
```

#### JSON
```python
# Python
import json

data = {"name": "Alice", "age": 30}
json_str = json.dumps(data)
parsed = json.loads(json_str)
```

```rads
// RADS
import json;

let data = {"name": "Alice", "age": 30};
let json_str = json.encode(data);
let parsed = json.decode(json_str);
```

#### HTTP Requests
```python
# Python
import urllib.request

response = urllib.request.urlopen("https://api.example.com/data")
data = response.read()
```

```rads
// RADS
import net;

let response = net.http_get("https://api.example.com/data");
let data = response.body;
```

---

### 4. Classes (85% Compatible)

```python
# Python
class Person:
    def __init__(self, name, age):
        self.name = name
        self.age = age
    
    def greet(self):
        print(f"Hi, I'm {self.name}")
    
    def birthday(self):
        self.age += 1

person = Person("Alice", 30)
person.greet()
person.birthday()
```

```rads
// RADS
class Person {
    fn init(name, age) {
        this.name = name;
        this.age = age;
    }
    
    fn greet() {
        io.print("Hi, I'm " + this.name);
    }
    
    fn birthday() {
        this.age = this.age + 1;
    }
}

let person = Person("Alice", 30);
person.greet();
person.birthday();
```

---

## ⚠️ WHAT REQUIRES LEARNING

### 1. Async/Await → Constellation (10% of code)

**Python:**
```python
import asyncio

async def fetch_data(url):
    response = await http_get(url)
    return response.json()

async def main():
    results = await asyncio.gather(
        fetch_data("https://api1.com"),
        fetch_data("https://api2.com")
    )
    print(results)

asyncio.run(main())
```

**RADS (requires learning):**
```rads
import constellation;
import net;

@constellation.async
fn fetch_data(url) {
    let response = net.http_get(url);
    return json.decode(response.body);
}

@constellation.parallel
fn main() {
    let results = constellation.gather([
        fetch_data("https://api1.com"),
        fetch_data("https://api2.com")
    ]);
    io.print(results);
}

main();
```

**Learning Required:** Understanding `@constellation.async` and `@constellation.parallel`

---

### 2. Context Managers (`with`) → `defer` (5% of code)

**Python:**
```python
with open("file.txt") as f:
    data = f.read()
    process(data)
# File automatically closed
```

**RADS:**
```rads
let f = io.open("file.txt", "r");
defer io.close(f);  // Executes when scope exits
let data = io.read(f);
process(data);
```

**Learning Required:** `defer` keyword (similar to Go)

---

### 3. Generators (`yield`) → Not Yet Supported (2% of code)

**Python:**
```python
def fibonacci():
    a, b = 0, 1
    while True:
        yield a
        a, b = b, a + b

for num in fibonacci():
    if num > 100:
        break
    print(num)
```

**RADS Workaround:**
```rads
fn fibonacci_up_to(max) {
    let results = [];
    let a = 0;
    let b = 1;
    while (a <= max) {
        array.push(results, a);
        let temp = a;
        a = b;
        b = temp + b;
    }
    return results;
}

for (let num in fibonacci_up_to(100)) {
    io.print(num);
}
```

**Learning Required:** Generators not supported; use arrays or iterators

---

### 4. Decorators → Function Wrappers (3% of code)

**Python:**
```python
def timer(func):
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        print(f"Took {time.time() - start}s")
        return result
    return wrapper

@timer
def slow_function():
    time.sleep(1)
    return "Done"
```

**RADS:**
```rads
import time;

fn timer(func) {
    return fn(...args) {
        let start = time.now();
        let result = func(...args);
        io.print("Took " + (time.now() - start) + "s");
        return result;
    };
}

let slow_function = timer(fn() {
    time.sleep(1);
    return "Done";
});
```

**Learning Required:** Manual decorator application (no `@` syntax yet)

---

## 🔧 CONVERSION EXAMPLES

### Example 1: Simple Script

**Python Input:**
```python
# calculate.py
def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)

def main():
    for i in range(1, 11):
        result = factorial(i)
        print(f"{i}! = {result}")

if __name__ == "__main__":
    main()
```

**Command:**
```bash
rads-mask --from python calculate.py -o calculate.rads
```

**RADS Output:**
```rads
import io;
import array;

fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn main() {
    for (let i in array.range(1, 11)) {
        let result = factorial(i);
        io.print(i + "! = " + result);
    }
}

main();
```

---

### Example 2: HTTP Server

**Python Input:**
```python
# server.py
from http.server import HTTPServer, BaseHTTPRequestHandler

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"<h1>Hello from Python!</h1>")

server = HTTPServer(('localhost', 8080), Handler)
print("Server running on http://localhost:8080")
server.serve_forever()
```

**RADS Output:**
```rads
import net;
import io;

let server = net.http_server(8080);

server.on_request(fn(req, res) {
    res.set_status(200);
    res.set_header("Content-Type", "text/html");
    res.send("<h1>Hello from RADS!</h1>");
});

io.print("Server running on http://localhost:8080");
server.start();
```

---

### Example 3: Data Processing

**Python Input:**
```python
# process.py
import json

def load_data(filename):
    with open(filename) as f:
        return json.load(f)

def process_users(users):
    adults = [u for u in users if u['age'] >= 18]
    names = [u['name'] for u in adults]
    return sorted(names)

def main():
    users = load_data("users.json")
    adult_names = process_users(users)
    print(f"Found {len(adult_names)} adults:")
    for name in adult_names:
        print(f"  - {name}")

if __name__ == "__main__":
    main()
```

**RADS Output:**
```rads
import io;
import json;
import array;

fn load_data(filename) {
    let f = io.open(filename, "r");
    defer io.close(f);
    let content = io.read(f);
    return json.decode(content);
}

fn process_users(users) {
    let adults = array.filter(users, fn(u) {
        return u["age"] >= 18;
    });
    let names = array.map(adults, fn(u) {
        return u["name"];
    });
    return array.sort(names);
}

fn main() {
    let users = load_data("users.json");
    let adult_names = process_users(users);
    io.print("Found " + array.length(adult_names) + " adults:");
    for (let name in adult_names) {
        io.print("  - " + name);
    }
}

main();
```

---

## 🎯 BEST PRACTICES

### 1. Start Simple
Begin with basic Python scripts before attempting complex projects.

### 2. Review Generated Code
Always check the generated RADS code to understand the conversion:
```bash
rads-mask --from python script.py --print
```

### 3. Use `--verbose` for Learning
See what conversions are happening:
```bash
rads-mask --from python script.py -o output.rads --verbose
```

### 4. Incremental Conversion
Convert one module at a time, test, then move to the next.

### 5. Mix Python and RADS
You can gradually introduce native RADS for performance-critical sections:
```python
# mostly_python.py
def slow_part(data):
    # This will be converted to RADS
    return [x * 2 for x in data]

# Import native RADS module for performance
import rads_native  # Hand-written RADS module

def fast_part(data):
    return rads_native.optimized_process(data)
```

---

## 🐛 COMMON ISSUES & SOLUTIONS

### Issue 1: Import Not Found
**Error:**
```
Error: Python module 'requests' has no RADS equivalent
```

**Solution:**
Use RADS's built-in `net` module:
```python
# Instead of:
import requests
response = requests.get("https://api.example.com")

# Use:
import net  # Will convert to RADS net module
response = net.http_get("https://api.example.com")
```

---

### Issue 2: List Comprehension Too Complex
**Error:**
```
Warning: Complex list comprehension may not convert optimally
```

**Solution:**
Simplify or use explicit loops:
```python
# Complex (may not convert well):
result = [x*2 for x in data if x > 0 for y in x.items() if y[1] > 10]

# Better (converts cleanly):
result = []
for x in data:
    if x > 0:
        for y in x.items():
            if y[1] > 10:
                result.append(x * 2)
```

---

### Issue 3: Type Hints Ignored
**Note:**
Python type hints are currently ignored but will be used in future versions:
```python
# Type hints are preserved in comments
def add(a: int, b: int) -> int:
    return a + b
```

```rads
// RADS (future: will use type hints for optimization)
fn add(a, b) {  // TODO: Add type annotations
    return a + b;
}
```

---

## 📊 CONVERSION COMPATIBILITY MATRIX

| Python Feature | RADS Support | Conversion Rate | Notes |
|----------------|--------------|-----------------|-------|
| Variables | ✅ Full | 100% | Direct mapping |
| Functions | ✅ Full | 100% | Including defaults |
| Classes | ✅ Full | 95% | No metaclasses |
| If/Elif/Else | ✅ Full | 100% | Direct mapping |
| For loops | ✅ Full | 100% | Uses `for...in` |
| While loops | ✅ Full | 100% | Direct mapping |
| Lists | ✅ Full | 98% | Negative indexing needs workaround |
| Dicts | ✅ Full | 95% | Most operations supported |
| Strings | ✅ Full | 90% | Most methods supported |
| List comprehensions | ✅ Partial | 85% | Simple ones convert well |
| Lambda functions | ✅ Full | 100% | Direct mapping |
| Decorators | ⚠️ Manual | 50% | Requires manual application |
| Context managers (`with`) | ⚠️ Alternative | 80% | Use `defer` instead |
| Generators (`yield`) | ❌ Not yet | 0% | Use arrays/iterators |
| Async/await | ⚠️ Alternative | 70% | Use Constellation |
| Type hints | ⏳ Future | 0% | Preserved in comments |
| F-strings | ✅ Full | 95% | Converts to concatenation |
| Exception handling | ✅ Full | 90% | Try/catch supported |
| Imports | ✅ Partial | 80% | Stdlib mapped, 3rd-party varies |

**Legend:**
- ✅ Full support, automatic conversion
- ⚠️ Requires learning RADS alternative
- ❌ Not supported yet
- ⏳ Planned for future release

---

## 🚀 NEXT STEPS

### 1. Try the Examples
Clone the examples repository:
```bash
git clone https://github.com/rads-lang/masquerading-examples
cd masquerading-examples/python
rads-mask --from python examples/*.py -o build/
```

### 2. Convert Your Own Code
Start with a small Python script:
```bash
rads-mask --from python your_script.py -o your_script.rads
rads your_script.rads
```

### 3. Learn RADS Extensions
Read about RADS-specific features:
- [Constellation (Distributed Computing)](../CONSTELLATION.md)
- [FFI (C Library Integration)](../FFI.md)
- [Performance Optimization](../OPTIMIZATION.md)

### 4. Join the Community
- Discord: https://discord.gg/rads-lang
- Forum: https://forum.rads-lang.org
- GitHub: https://github.com/rads-lang/rads

---

## 📚 ADDITIONAL RESOURCES

- [Python → RADS Cheat Sheet](./PYTHON_CHEATSHEET.md)
- [Common Python Patterns in RADS](./PYTHON_PATTERNS.md)
- [Migrating Django to RADS](./PYTHON_DJANGO.md)
- [Migrating Flask to RADS](./PYTHON_FLASK.md)
- [Python Stdlib → RADS Stdlib Mapping](./PYTHON_STDLIB.md)

---

**Questions?** Open an issue on GitHub or ask in Discord!

**Found a conversion bug?** Report it: https://github.com/rads-lang/rads/issues/new?template=masquerading-bug
