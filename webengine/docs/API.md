# 🌐 RADS Web Engine - Complete API Reference

**Version:** v0.0.3 "Butterfly"

---

## 📖 Table of Contents

1. [Initialization](#initialization)
2. [JavaScript API](#javascript-api)
3. [HTML API](#html-api)
4. [CSS API](#css-api)
5. [Plugin API](#plugin-api)
6. [Examples](#examples)

---

## Initialization

### `web_init()`

Initializes the web engine (JavaScript runtime, HTML parser, CSS parser, plugin system).

**Returns:** `Boolean` - true on success

**Example:**
```rads
blast main() {
    web_init();
    echo("Ready!");
}
```

**Output:**
```
[WEB] JavaScript engine initialized
Ready!
```

---

## JavaScript API

### `js_eval(code)`

Executes JavaScript code.

**Parameters:**
- `code` (String) - JavaScript code to execute

**Returns:** `Boolean` - true on success

**Supported Features:**
- `console.log()` - Logging
- Variables and expressions
- Basic JavaScript syntax

**Examples:**
```rads
// Simple logging
js_eval("console.log('Hello!')");

// Variables
js_eval("const x = 42");
js_eval("console.log('x = ' + x)");

// Math
js_eval("console.log('2 + 2 = ' + (2 + 2))");
```

---

### `js_evalFile(path)`

Executes a JavaScript file.

**Parameters:**
- `path` (String) - Path to JavaScript file

**Returns:** `Boolean` - true on success

**Example:**
```rads
js_evalFile("script.js");
```

**script.js:**
```javascript
console.log('Running from file!');
const result = 42 + 58;
console.log('Result: ' + result);
```

---

## HTML API

### `html_parse(html)`

Parses HTML string into a document object.

**Parameters:**
- `html` (String) - HTML markup

**Returns:** `String` - Document reference

**Example:**
```rads
turbo html = "<html><body><h1>Test</h1></body></html>";
turbo doc = html_parse(html);
echo("Document: " + doc);
```

**Output:**
```
[HTML] Parsed HTML document (45 bytes)
Document: <HTMLDocument:0x12345678>
```

---

### `html_querySelector(doc, selector)`

Queries DOM for elements matching CSS selector.

**Parameters:**
- `doc` (String) - Document reference from `html_parse()`
- `selector` (String) - CSS selector

**Returns:** `String` - Element reference

**Example:**
```rads
turbo html = "<div class='content'>Text</div>";
turbo doc = html_parse(html);

turbo div = html_querySelector(doc, ".content");
turbo h1 = html_querySelector(doc, "h1");
```

---

## CSS API

### `css_parse(css)`

Parses CSS stylesheet.

**Parameters:**
- `css` (String) - CSS code

**Returns:** `String` - Stylesheet reference

**Example:**
```rads
turbo css = "
    body {
        color: purple;
        background: linear-gradient(135deg, #667eea, #764ba2);
    }
";

turbo styles = css_parse(css);
```

---

## Plugin API

### `pkg_load(name)`

Loads a plugin by name.

**Parameters:**
- `name` (String) - Plugin name

**Returns:** `String` - Plugin reference

**Example:**
```rads
turbo markdown = pkg_load("rads-plugin-markdown");
turbo template = pkg_load("rads-plugin-template");
```

**Output:**
```
[PLUGIN] Loaded: rads-plugin-markdown v1.0.0
[PLUGIN] Loaded: rads-plugin-template v1.0.0
```

---

### `pkg_list()`

Lists all installed plugins.

**Returns:** `Integer` - Number of plugins

**Example:**
```rads
turbo count = pkg_list();
echo("Total: " + count);
```

**Output:**
```
[PLUGIN] Installed plugins:
  • rads-plugin-markdown v1.0.0
  • rads-plugin-template v1.0.0
Total: 2
```

---

### `pkg_install(path)`

Installs a plugin from directory.

**Parameters:**
- `path` (String) - Path to plugin directory

**Returns:** `Boolean` - true on success

**Example:**
```rads
pkg_install("./my-custom-plugin");
```

---

## Examples

### Full Web Application

```rads
blast home_handler(path, method, body, query, params, headers, cookies) {
    turbo html = "<!DOCTYPE html>";
    html = html + "<html><head>";
    html = html + "<style>";
    html = html + "body { background: linear-gradient(135deg, #667eea, #764ba2); }";
    html = html + "</style>";
    html = html + "</head><body>";
    html = html + "<h1>RADS Web Engine</h1>";
    html = html + "</body></html>";

    return [200, html, "text/html"];
}

blast main() {
    web_init();
    db.open("app.db");

    turbo server = net.http_server("localhost", 3000);
    net.route(server, "/", home_handler, "GET");

    net.serve();
}
```

---

### Plugin-Based Application

```rads
blast main() {
    web_init();

    // Load plugins
    turbo md = pkg_load("rads-plugin-markdown");
    turbo tpl = pkg_load("rads-plugin-template");
    turbo val = pkg_load("rads-plugin-validator");

    // Use plugins (in full implementation)
    // turbo html = md.render("# Hello");
    // turbo page = tpl.render("index.html", {"title": "RADS"});

    pkg_list();
}
```

---

*Complete API documentation for RADS Web Engine v0.0.3 "Butterfly"* 🦋
