# JavaScript → RADS Masquerading Guide

**Version:** 0.0.5  
**Status:** Draft  
**Last Updated:** 2026-01-13

---

## 🌐 INTRODUCTION

This guide shows JavaScript/TypeScript developers how to use RADS without learning a new syntax. Write JavaScript, run RADS.

**Conversion Rate:** ~90% of JavaScript code converts automatically  
**Learning Required:** ~10% for RADS-specific features

---

## 🚀 QUICK START

### Your First Conversion

```bash
# Write JavaScript as usual
cat > hello.js << EOF
function greet(name) {
    console.log(\`Hello, \${name}!\`);
}

greet("World");
EOF

# Convert to RADS
rads-mask --from javascript hello.js -o hello.rads

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
```javascript
// JavaScript
const x = 10;
let name = "Alice";
var isValid = true;
let data = null;
```

```rads
// RADS
let x = 10;
let name = "Alice";
let is_valid = true;
let data = null;
```

#### Functions
```javascript
// JavaScript
function add(a, b) {
    return a + b;
}

const multiply = (a, b) => a * b;

const greet = (name = "World") => {
    console.log(`Hello, ${name}`);
};
```

```rads
// RADS
fn add(a, b) {
    return a + b;
}

let multiply = fn(a, b) { return a * b; };

let greet = fn(name = "World") {
    io.print("Hello, " + name);
};
```

#### Control Flow
```javascript
// JavaScript
if (x > 10) {
    console.log("Large");
} else if (x > 5) {
    console.log("Medium");
} else {
    console.log("Small");
}

for (let i = 0; i < 10; i++) {
    console.log(i);
}

while (x > 0) {
    x--;
}
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

for (let i = 0; i < 10; i = i + 1) {
    io.print(i);
}

while (x > 0) {
    x = x - 1;
}
```

---

### 2. Data Structures (95% Compatible)

#### Arrays
```javascript
// JavaScript
const numbers = [1, 2, 3, 4, 5];
numbers.push(6);
numbers.pop();
const length = numbers.length;
const first = numbers[0];
const last = numbers[numbers.length - 1];
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

#### Objects
```javascript
// JavaScript
const person = {
    name: "Alice",
    age: 30,
    city: "NYC"
};
person.email = "alice@example.com";
const name = person.name || "Unknown";
```

```rads
// RADS
let person = {
    "name": "Alice",
    "age": 30,
    "city": "NYC"
};
person["email"] = "alice@example.com";
let name = person["name"] || "Unknown";
```

#### Strings
```javascript
// JavaScript
const text = "Hello World";
const upper = text.toUpperCase();
const lower = text.toLowerCase();
const parts = text.split(" ");
const joined = parts.join(" ");
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

### 3. Array Methods (90% Compatible)

```javascript
// JavaScript
const numbers = [1, 2, 3, 4, 5];

const doubled = numbers.map(x => x * 2);
const evens = numbers.filter(x => x % 2 === 0);
const sum = numbers.reduce((acc, x) => acc + x, 0);
const hasLarge = numbers.some(x => x > 3);
const allPositive = numbers.every(x => x > 0);
```

```rads
// RADS
let numbers = [1, 2, 3, 4, 5];

let doubled = array.map(numbers, fn(x) { return x * 2; });
let evens = array.filter(numbers, fn(x) { return x % 2 == 0; });
let sum = array.reduce(numbers, fn(acc, x) { return acc + x; }, 0);
let has_large = array.some(numbers, fn(x) { return x > 3; });
let all_positive = array.every(numbers, fn(x) { return x > 0; });
```

---

### 4. HTTP Server (Node.js → RADS)

```javascript
// JavaScript (Node.js)
const http = require('http');

const server = http.createServer((req, res) => {
    res.writeHead(200, {'Content-Type': 'text/plain'});
    res.end('Hello World\n');
});

server.listen(8080, () => {
    console.log('Server running at http://localhost:8080/');
});
```

```rads
// RADS
import net;
import io;

let server = net.http_server(8080);

server.on_request(fn(req, res) {
    res.set_header("Content-Type", "text/plain");
    res.send("Hello World\n");
});

io.print("Server running at http://localhost:8080/");
server.start();
```

---

## ⚠️ WHAT REQUIRES LEARNING

### 1. Async/Await → Constellation (10% of code)

**JavaScript:**
```javascript
async function fetchData(url) {
    const response = await fetch(url);
    return await response.json();
}

async function main() {
    const results = await Promise.all([
        fetchData("https://api1.com"),
        fetchData("https://api2.com")
    ]);
    console.log(results);
}

main();
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

---

### 2. Promises → Constellation (8% of code)

**JavaScript:**
```javascript
function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

delay(1000).then(() => {
    console.log("Done!");
});
```

**RADS:**
```rads
import time;
import constellation;

fn delay(ms) {
    return constellation.promise(fn(resolve) {
        time.sleep(ms);
        resolve();
    });
}

delay(1000).then(fn() {
    io.print("Done!");
});
```

---

### 3. Classes → RADS Classes (5% of code)

**JavaScript:**
```javascript
class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }
    
    greet() {
        console.log(`Hi, I'm ${this.name}`);
    }
    
    birthday() {
        this.age++;
    }
}

const person = new Person("Alice", 30);
person.greet();
```

**RADS:**
```rads
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
```

---

### 4. Import/Export → RADS Modules (3% of code)

**JavaScript:**
```javascript
// math.js
export function add(a, b) {
    return a + b;
}

export const PI = 3.14159;

// main.js
import { add, PI } from './math.js';
console.log(add(2, 3));
console.log(PI);
```

**RADS:**
```rads
// math.rads
export fn add(a, b) {
    return a + b;
}

export let PI = 3.14159;

// main.rads
import math;
io.print(math.add(2, 3));
io.print(math.PI);
```

---

## 🔧 CONVERSION EXAMPLES

### Example 1: Express.js Server → RADS

**JavaScript Input:**
```javascript
// server.js
const express = require('express');
const app = express();

app.use(express.json());

app.get('/', (req, res) => {
    res.json({ message: 'Hello World' });
});

app.post('/users', (req, res) => {
    const user = req.body;
    res.status(201).json({ id: 1, ...user });
});

app.listen(3000, () => {
    console.log('Server running on port 3000');
});
```

**RADS Output:**
```rads
import net;
import json;
import io;

let app = net.http_server(3000);

app.get("/", fn(req, res) {
    res.json({"message": "Hello World"});
});

app.post("/users", fn(req, res) {
    let user = json.decode(req.body);
    res.set_status(201);
    res.json({"id": 1, ...user});
});

io.print("Server running on port 3000");
app.start();
```

---

### Example 2: Data Processing

**JavaScript Input:**
```javascript
// process.js
const fs = require('fs');

function loadData(filename) {
    const content = fs.readFileSync(filename, 'utf8');
    return JSON.parse(content);
}

function processUsers(users) {
    const adults = users.filter(u => u.age >= 18);
    const names = adults.map(u => u.name);
    return names.sort();
}

function main() {
    const users = loadData('users.json');
    const adultNames = processUsers(users);
    console.log(`Found ${adultNames.length} adults:`);
    adultNames.forEach(name => console.log(`  - ${name}`));
}

main();
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
    array.for_each(adult_names, fn(name) {
        io.print("  - " + name);
    });
}

main();
```

---

## 📊 CONVERSION COMPATIBILITY MATRIX

| JavaScript Feature | RADS Support | Conversion Rate | Notes |
|-------------------|--------------|-----------------|-------|
| Variables (let/const/var) | ✅ Full | 100% | All become `let` |
| Functions | ✅ Full | 100% | Including arrow functions |
| Classes | ✅ Full | 95% | No private fields yet |
| If/Else | ✅ Full | 100% | Direct mapping |
| For loops | ✅ Full | 100% | All variants supported |
| While loops | ✅ Full | 100% | Direct mapping |
| Arrays | ✅ Full | 98% | Most methods supported |
| Objects | ✅ Full | 95% | Most operations supported |
| Strings | ✅ Full | 90% | Most methods supported |
| Template literals | ✅ Full | 95% | Converts to concatenation |
| Arrow functions | ✅ Full | 100% | Direct mapping |
| Destructuring | ⚠️ Partial | 70% | Simple cases work |
| Spread operator | ⚠️ Partial | 75% | Basic support |
| Async/await | ⚠️ Alternative | 70% | Use Constellation |
| Promises | ⚠️ Alternative | 70% | Use Constellation |
| Generators | ❌ Not yet | 0% | Use arrays/iterators |
| Symbols | ❌ Not yet | 0% | Not supported |
| Proxy/Reflect | ❌ Not yet | 0% | Not supported |
| WeakMap/WeakSet | ❌ Not yet | 0% | Not supported |

---

## 🎯 BEST PRACTICES

### 1. Use Modern JavaScript
Modern ES6+ syntax converts better than old ES5:
```javascript
// Good (ES6+)
const add = (a, b) => a + b;

// Also fine (ES5)
function add(a, b) {
    return a + b;
}
```

### 2. Avoid Complex Destructuring
Simple destructuring works, complex patterns may not:
```javascript
// Good (converts well)
const { name, age } = person;

// Avoid (may not convert well)
const { name: userName, address: { city } } = person;
```

### 3. Prefer Explicit Over Implicit
Be explicit for better conversion:
```javascript
// Good
if (value !== null && value !== undefined) { ... }

// Avoid
if (value) { ... }  // Truthy check may convert differently
```

---

## 🚀 NEXT STEPS

1. **Try Examples**: `git clone https://github.com/rads-lang/masquerading-examples`
2. **Convert Your Code**: `rads-mask --from javascript your_app.js -o your_app.rads`
3. **Learn RADS Extensions**: Read about Constellation, FFI, and optimization
4. **Join Community**: Discord, Forum, GitHub

---

**Questions?** Open an issue or ask in Discord!
