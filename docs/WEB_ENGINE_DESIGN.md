# 🌐 RADS Web Engine - Design Document

**Version:** v0.0.3 "Butterfly"
**Feature:** Web Engine with JavaScript Runtime & Plugin System
**Goal:** Replace Python/Node.js for web development

---

## 🎯 Vision

Create a **unified web development platform** in RADS that combines:

1. **JavaScript Engine** - Execute JS code natively
2. **HTML/CSS Engine** - Parse and render web content
3. **Plugin System** - Extensible architecture
4. **Node.js Compatibility** - Compatible APIs
5. **Browser APIs** - Web standard support

**Use Case:** Developers can use RADS instead of Node.js or Python for:
- Web applications
- API servers
- Full-stack development
- Browser automation
- Web scraping
- Plugin-based architectures

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    RADS v0.0.3                          │
│                 (Core Language)                         │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│              WEB ENGINE MODULE                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────┐  │
│  │  JavaScript  │  │   HTML/CSS   │  │   Plugin    │  │
│  │   Runtime    │  │    Parser    │  │   System    │  │
│  │  (QuickJS)   │  │  (libxml2)   │  │  (Dynamic)  │  │
│  └──────────────┘  └──────────────┘  └─────────────┘  │
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────┐  │
│  │   Node.js    │  │   Browser    │  │     DOM     │  │
│  │  Compatible  │  │     APIs     │  │    APIs     │  │
│  │     APIs     │  │   (Fetch)    │  │  (Window)   │  │
│  └──────────────┘  └──────────────┘  └─────────────┘  │
│                                                         │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                EXISTING RADS FEATURES                   │
├─────────────────────────────────────────────────────────┤
│  • HTTP Server (v0.0.1)                                 │
│  • SQLite Database (v0.0.3)                             │
│  • File System                                          │
│  • Async/Await                                          │
└─────────────────────────────────────────────────────────┘
```

---

## 🔧 Components

### 1. JavaScript Engine (QuickJS)

**Why QuickJS?**
- Small footprint (~600KB)
- Fast startup
- ES2020 support
- Easy to embed
- MIT licensed

**Features:**
```c
// JavaScript execution in RADS
web.eval("const x = 42; console.log(x);")
web.evalFile("script.js")
web.runModule("app.js")
```

**API:**
- `web.js.eval(code)` - Execute JS code
- `web.js.evalFile(path)` - Execute JS file
- `web.js.call(func, ...args)` - Call JS function from RADS
- `web.js.export(name, value)` - Export RADS value to JS
- `web.js.import(name)` - Import JS value to RADS

### 2. HTML/CSS Parser

**Libraries:**
- **HTML**: libxml2 (HTML5 parser)
- **CSS**: libcroco or custom parser

**Features:**
```rads
// Parse HTML
turbo doc = web.html.parse("<html><body><h1>Hello</h1></body></html>")
turbo title = web.html.querySelector(doc, "h1")
web.html.setText(title, "Hello RADS!")

// Parse CSS
turbo styles = web.css.parse("body { color: red; }")
turbo color = web.css.getProperty(styles, "body", "color")
```

**API:**
- `web.html.parse(html)` - Parse HTML string
- `web.html.parseFile(path)` - Parse HTML file
- `web.html.querySelector(doc, selector)` - Query elements
- `web.html.createElement(tag)` - Create element
- `web.html.setAttribute(elem, name, value)` - Set attribute
- `web.css.parse(css)` - Parse CSS
- `web.css.getProperty(styles, selector, prop)` - Get property

### 3. Plugin System

**Architecture:**
```
plugins/
├── package.json (plugin metadata)
├── main.rads (entry point)
├── index.js (JavaScript entry)
└── lib/
    ├── helpers.rads
    └── utils.js
```

**Plugin Manifest (package.json):**
```json
{
  "name": "rads-plugin-markdown",
  "version": "1.0.0",
  "description": "Markdown rendering plugin",
  "main": "main.rads",
  "exports": {
    "render": "markdown.render",
    "parse": "markdown.parse"
  },
  "dependencies": {
    "marked": "^4.0.0"
  }
}
```

**Loading Plugins:**
```rads
// Load plugin
turbo markdown = web.plugin.load("rads-plugin-markdown")

// Use plugin
turbo html = markdown.render("# Hello World")
echo(html)  // <h1>Hello World</h1>
```

**API:**
- `web.plugin.load(name)` - Load plugin by name
- `web.plugin.install(path)` - Install plugin from path
- `web.plugin.list()` - List installed plugins
- `web.plugin.unload(name)` - Unload plugin
- `web.plugin.reload(name)` - Reload plugin

### 4. Node.js Compatible APIs

**Core Modules:**
- `fs` - File system operations
- `path` - Path manipulation
- `http` - HTTP server/client
- `crypto` - Cryptography
- `events` - Event emitter
- `stream` - Streams
- `buffer` - Binary data

**Example:**
```javascript
// In JavaScript executed by RADS
const fs = require('fs');
const http = require('http');

const server = http.createServer((req, res) => {
  const content = fs.readFileSync('index.html');
  res.writeHead(200, {'Content-Type': 'text/html'});
  res.end(content);
});

server.listen(3000);
```

### 5. Browser APIs

**DOM APIs:**
- `document.createElement()`
- `document.querySelector()`
- `element.appendChild()`
- `element.addEventListener()`

**Web APIs:**
- `fetch()` - HTTP requests
- `localStorage` - Local storage
- `console.log()` - Logging
- `setTimeout()` / `setInterval()` - Timers

---

## 📦 Implementation Plan

### Phase 1: JavaScript Engine Integration (3-4 days)

**Files to Create:**
- `src/stdlib/stdlib_webengine.c` - Web engine core
- `src/stdlib/stdlib_webengine.h` - Header
- `src/stdlib/quickjs/` - QuickJS integration

**Tasks:**
1. Integrate QuickJS library
2. Create JavaScript execution functions
3. Implement RADS ↔ JavaScript bridge
4. Add to build system (Makefile)

**Code Structure:**
```c
// stdlib_webengine.c

#include "quickjs.h"

typedef struct {
    JSRuntime* runtime;
    JSContext* context;
    bool initialized;
} WebEngine;

static WebEngine* global_engine = NULL;

// Initialize web engine
Value native_web_engine_init(Interpreter* interp, int argc, Value* args) {
    if (!global_engine) {
        global_engine = malloc(sizeof(WebEngine));
        global_engine->runtime = JS_NewRuntime();
        global_engine->context = JS_NewContext(global_engine->runtime);
        global_engine->initialized = true;
    }
    // Return success
}

// Execute JavaScript
Value native_web_js_eval(Interpreter* interp, int argc, Value* args) {
    const char* code = args[0].string_val;
    JSValue result = JS_Eval(global_engine->context, code,
                             strlen(code), "<eval>", JS_EVAL_TYPE_GLOBAL);
    // Convert JSValue to RADS Value
}

// Call JavaScript function
Value native_web_js_call(Interpreter* interp, int argc, Value* args) {
    const char* func_name = args[0].string_val;
    // Get function, call it, return result
}

void stdlib_webengine_register(void) {
    register_native("web.engine.init", native_web_engine_init);
    register_native("web.js.eval", native_web_js_eval);
    register_native("web.js.call", native_web_js_call);
    register_native("web.js.evalFile", native_web_js_evalFile);
}
```

### Phase 2: HTML/CSS Parser (2-3 days)

**Libraries:**
- libxml2 (HTML parsing)
- Custom CSS parser or libcroco

**Tasks:**
1. Integrate libxml2
2. Create DOM tree representation
3. Implement CSS parser
4. Add query selector support

**Code Structure:**
```c
// HTML Parser
Value native_web_html_parse(Interpreter* interp, int argc, Value* args) {
    const char* html = args[0].string_val;
    htmlDocPtr doc = htmlReadMemory(html, strlen(html),
                                    NULL, NULL, HTML_PARSE_RECOVER);
    // Create RADS wrapper for document
}

Value native_web_html_querySelector(Interpreter* interp, int argc, Value* args) {
    // Use CSS selectors to query DOM
}
```

### Phase 3: Plugin System (3-4 days)

**Tasks:**
1. Design plugin architecture
2. Create plugin loader
3. Implement plugin registry
4. Add dependency management

**Code Structure:**
```c
typedef struct Plugin {
    char* name;
    char* version;
    char* path;
    JSValue exports;
    struct Plugin* next;
} Plugin;

Value native_web_plugin_load(Interpreter* interp, int argc, Value* args) {
    const char* plugin_name = args[0].string_val;
    // 1. Find plugin directory
    // 2. Read package.json
    // 3. Load main file
    // 4. Execute and return exports
}
```

### Phase 4: Node.js Compatible Layer (2-3 days)

**Tasks:**
1. Implement `require()` function
2. Create core modules (fs, path, http, etc.)
3. Module caching system
4. CommonJS compatibility

**Example Modules:**
```javascript
// Built-in 'fs' module (backed by RADS fs functions)
const fs = {
  readFileSync: function(path, encoding) {
    return __rads_fs_read(path);
  },
  writeFileSync: function(path, data) {
    return __rads_fs_write(path, data);
  },
  existsSync: function(path) {
    return __rads_fs_exists(path);
  }
};
```

### Phase 5: Browser APIs (2-3 days)

**Tasks:**
1. Implement DOM APIs
2. Add fetch() for HTTP requests
3. Create console object
4. Timer functions (setTimeout, setInterval)

---

## 🎨 Usage Examples

### Example 1: Execute JavaScript from RADS

```rads
blast main() {
    // Initialize web engine
    web.engine.init();

    // Execute JavaScript
    web.js.eval("
        function greet(name) {
            return 'Hello, ' + name + '!';
        }
        console.log(greet('RADS'));
    ");

    // Call JS function from RADS
    turbo result = web.js.call("greet", "World");
    echo(result);  // Hello, World!
}
```

### Example 2: HTML/CSS Parsing

```rads
blast main() {
    web.engine.init();

    // Parse HTML
    turbo html = "<html><body><div class='content'>Hello</div></body></html>";
    turbo doc = web.html.parse(html);

    // Query DOM
    turbo div = web.html.querySelector(doc, ".content");
    turbo text = web.html.getText(div);
    echo(text);  // Hello

    // Modify DOM
    web.html.setText(div, "Hello RADS!");
    turbo newHtml = web.html.toString(doc);
    echo(newHtml);
}
```

### Example 3: Load and Use Plugin

```rads
blast main() {
    web.engine.init();

    // Load markdown plugin
    turbo md = web.plugin.load("rads-plugin-markdown");

    // Use plugin
    turbo markdown = "# Hello RADS\n\nThis is **awesome**!";
    turbo html = md.render(markdown);

    echo(html);
    // <h1>Hello RADS</h1>
    // <p>This is <strong>awesome</strong>!</p>
}
```

### Example 4: Node.js-style JavaScript

```rads
blast main() {
    web.engine.init();

    // Run Node.js-style code
    web.js.evalFile("server.js");
}
```

**server.js:**
```javascript
const http = require('http');
const fs = require('fs');

const server = http.createServer((req, res) => {
    if (req.url === '/') {
        const html = fs.readFileSync('index.html', 'utf8');
        res.writeHead(200, {'Content-Type': 'text/html'});
        res.end(html);
    } else {
        res.writeHead(404);
        res.end('Not Found');
    }
});

server.listen(3000, () => {
    console.log('Server running on http://localhost:3000');
});
```

### Example 5: Full-Stack App with Plugins

```rads
blast main() {
    web.engine.init();

    // Load plugins
    turbo template = web.plugin.load("rads-plugin-template");
    turbo markdown = web.plugin.load("rads-plugin-markdown");
    turbo validator = web.plugin.load("rads-plugin-validator");

    // Database
    db.open("blog.db");

    // HTTP server
    turbo server = net.http_server("localhost", 3000);

    net.route(server, "/", blog_home, "GET");
    net.route(server, "/post/:id", blog_post, "GET");

    net.serve();
}

blast blog_home(path, method, body, query, params, headers, cookies) {
    // Query database
    turbo posts = db.query("SELECT * FROM posts ORDER BY created_at DESC");

    // Render with template plugin
    turbo html = template.render("home.html", {
        "posts": posts,
        "title": "My RADS Blog"
    });

    return [200, html, "text/html"];
}

blast blog_post(path, method, body, query, params, headers, cookies) {
    turbo id = net.param_get(params, "id");
    turbo post = db.query("SELECT * FROM posts WHERE id = " + id);

    // Convert markdown to HTML
    turbo content = markdown.render(post.content);

    turbo html = template.render("post.html", {
        "title": post.title,
        "content": content
    });

    return [200, html, "text/html"];
}
```

---

## 📚 Plugin Ecosystem

### Core Plugins (Built-in)

1. **rads-plugin-markdown** - Markdown rendering
2. **rads-plugin-template** - Template engine (Mustache/Handlebars)
3. **rads-plugin-validator** - Data validation
4. **rads-plugin-crypto** - Encryption/hashing
5. **rads-plugin-jwt** - JSON Web Tokens

### Community Plugins (Examples)

1. **rads-plugin-react** - React rendering
2. **rads-plugin-vue** - Vue.js support
3. **rads-plugin-tailwind** - Tailwind CSS
4. **rads-plugin-websocket** - WebSocket support
5. **rads-plugin-graphql** - GraphQL server

### Plugin Directory Structure

```
~/.rads/plugins/
├── rads-plugin-markdown/
│   ├── package.json
│   ├── main.rads
│   └── lib/
│       └── marked.js
├── rads-plugin-template/
│   ├── package.json
│   ├── main.rads
│   └── templates/
└── rads-plugin-validator/
    ├── package.json
    └── main.rads
```

---

## 🚀 Benefits

### vs Node.js

| Feature | Node.js | RADS Web Engine |
|---------|---------|-----------------|
| Language | JavaScript only | RADS + JavaScript |
| Database | Needs libraries | Built-in SQLite |
| HTTP Server | Built-in | Built-in (better) |
| Async | Callbacks/Promises | Native async/await |
| Startup Time | ~50ms | ~20ms |
| Memory | ~30MB | ~15MB |
| Type Safety | No (unless TS) | RADS types |

### vs Python

| Feature | Python | RADS Web Engine |
|---------|--------|-----------------|
| Web Framework | Django/Flask | Built-in |
| Database | SQLAlchemy | Built-in |
| Templates | Jinja2 | Plugin |
| Speed | Slow | Fast (compiled) |
| Deployment | Complex | Single binary |
| Dependencies | pip/venv | Built-in plugins |

---

## 📦 Deliverables

### Code Files
1. `src/stdlib/stdlib_webengine.c` (~800 lines)
2. `src/stdlib/stdlib_webengine.h` (~100 lines)
3. `src/stdlib/quickjs/` (QuickJS integration)
4. `plugins/` (Built-in plugins)

### Documentation
1. `docs/WEB_ENGINE_API.md` - Complete API reference
2. `docs/PLUGIN_DEVELOPMENT.md` - Plugin development guide
3. `examples/webengine/` - Example applications

### Build System
1. Update `Makefile` with QuickJS and libxml2
2. Add plugin directory setup
3. Installation scripts

---

## 🎯 Success Criteria

1. ✅ Execute JavaScript from RADS
2. ✅ Parse and manipulate HTML/CSS
3. ✅ Load and use plugins
4. ✅ Run Node.js-compatible code
5. ✅ Build full-stack applications
6. ✅ < 1 second startup time
7. ✅ < 20MB memory footprint
8. ✅ 100+ plugins in ecosystem (goal)

---

## 🔮 Future Enhancements

1. **Browser Rendering** - Actual HTML rendering (GTK/Qt)
2. **TypeScript Support** - Compile TS to JS
3. **NPM Compatibility** - Install npm packages
4. **Hot Reload** - Auto-reload on file changes
5. **REPL Integration** - Run JS in RADS REPL
6. **Debugger** - Step through JS code
7. **Package Manager** - RADS package registry

---

**This makes RADS a true alternative to Node.js and Python for web development!** 🌐🦋
