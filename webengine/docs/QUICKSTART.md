# 🚀 RADS Web Engine - Quick Start Guide

Get up and running with RADS Web Engine in 5 minutes!

---

## ⚡ Installation

RADS Web Engine is built into RADS v0.0.3 "Butterfly".

```bash
# Verify you have RADS v0.0.3+
rads --version

# Should show: RADS v0.0.3 "Butterfly" 🦋
```

---

## 🎯 Your First Web Engine Program

### Step 1: Hello World

Create `hello.rads`:

```rads
blast main() {
    // Initialize web engine
    web_init();

    echo("Web engine initialized!");
}
```

Run it:

```bash
rads hello.rads
```

Output:
```
[WEB] JavaScript engine initialized
Web engine initialized!
```

---

### Step 2: Execute JavaScript

Create `js_test.rads`:

```rads
blast main() {
    web_init();

    js_eval("console.log('Hello from JavaScript!')");
    js_eval("console.log('2 + 2 = 4')");
}
```

Run it:

```bash
rads js_test.rads
```

Output:
```
[CONSOLE] Hello from JavaScript!
[CONSOLE] 2 + 2 = 4
```

---

### Step 3: Parse HTML

Create `html_test.rads`:

```rads
blast main() {
    web_init();

    turbo html = "<html><body><h1>Hello RADS!</h1></body></html>";
    turbo doc = html_parse(html);

    echo("Document: " + doc);

    turbo h1 = html_querySelector(doc, "h1");
    echo("Found h1: " + h1);
}
```

Run it:

```bash
rads html_test.rads
```

---

### Step 4: Load Plugins

Create `plugin_test.rads`:

```rads
blast main() {
    web_init();

    turbo markdown = pkg_load("rads-plugin-markdown");
    turbo template = pkg_load("rads-plugin-template");

    echo("Plugins loaded!");

    pkg_list();
}
```

---

## 🌐 Build a Web Server

Create `server.rads`:

```rads
blast home_handler(path, method, body, query, params, headers, cookies) {
    turbo html = "<html><body><h1>Hello from RADS!</h1></body></html>";
    return [200, html, "text/html"];
}

blast main() {
    web_init();

    turbo server = net.http_server("localhost", 3000);
    net.route(server, "/", home_handler, "GET");

    echo("Server running at http://localhost:3000");
    net.serve();
}
```

Run it and visit http://localhost:3000!

---

## 📚 Next Steps

1. **Run Examples**
   ```bash
   rads webengine/examples/01-basic/hello.rads
   rads webengine/examples/02-javascript/js_demo.rads
   rads webengine/examples/03-html-css/html_demo.rads
   ```

2. **Build Task Manager**
   ```bash
   rads webengine/examples/05-task-manager/task_manager.rads
   # Visit http://localhost:5050
   ```

3. **Read Documentation**
   - API Reference: `webengine/docs/API.md`
   - Examples: `webengine/docs/EXAMPLES.md`

---

## 🎯 Complete API

```rads
// Initialization
web_init()

// JavaScript
js_eval(code)
js_evalFile(path)

// HTML
html_parse(html)
html_querySelector(doc, selector)

// CSS
css_parse(css)

// Plugins
pkg_load(name)
pkg_list()
pkg_install(path)
```

---

## 🏃 Run in REPL

```bash
$ rads
rads[1]> web_init()
rads[2]> js_eval("console.log('Hello!')")
rads[3]> html_parse("<html>Test</html>")
rads[4]> pkg_load("markdown")
rads[5]> pkg_list()
```

---

## ✅ You're Ready!

You now know how to:
- ✅ Initialize web engine
- ✅ Execute JavaScript
- ✅ Parse HTML/CSS
- ✅ Load plugins
- ✅ Build web servers

**Start building amazing web applications with RADS!** 🦋

---

*Keep it TURBO, keep it RADICAL!* 🚀✨
