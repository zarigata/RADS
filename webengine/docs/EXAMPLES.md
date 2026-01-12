# 📚 RADS Web Engine - Examples Cookbook

Collection of practical examples for RADS Web Engine.

---

## 🎯 Basic Examples

### Example 1: Hello World

```rads
blast main() {
    web_init();
    echo("Web engine ready!");
}
```

**Run:** `rads webengine/examples/01-basic/hello.rads`

---

### Example 2: JavaScript Console

```rads
blast main() {
    web_init();

    js_eval("console.log('Hello from JavaScript!')");
    js_eval("console.log('RADS is TURBO!')");
}
```

**Run:** `rads webengine/examples/02-javascript/js_demo.rads`

---

### Example 3: HTML Parsing

```rads
blast main() {
    web_init();

    turbo html = "<html><body><h1>Hello!</h1></body></html>";
    turbo doc = html_parse(html);

    turbo h1 = html_querySelector(doc, "h1");
    echo("Found: " + h1);
}
```

**Run:** `rads webengine/examples/03-html-css/html_demo.rads`

---

### Example 4: Plugin System

```rads
blast main() {
    web_init();

    turbo md = pkg_load("rads-plugin-markdown");
    turbo tpl = pkg_load("rads-plugin-template");

    pkg_list();
}
```

**Run:** `rads webengine/examples/04-plugins/plugin_demo.rads`

---

## 🌐 Web Server Examples

### Simple HTTP Server

```rads
blast home_handler(path, method, body, query, params, headers, cookies) {
    turbo html = "<html><body><h1>Hello RADS!</h1></body></html>";
    return [200, html, "text/html"];
}

blast main() {
    turbo server = net.http_server("localhost", 3000);
    net.route(server, "/", home_handler, "GET");

    echo("Server: http://localhost:3000");
    net.serve();
}
```

---

### JSON API Server

```rads
blast api_handler(path, method, body, query, params, headers, cookies) {
    turbo json = "{\"status\": \"ok\", \"message\": \"Hello from RADS!\"}";
    return [200, json, "application/json"];
}

blast main() {
    turbo server = net.http_server("localhost", 8080);
    net.route(server, "/api", api_handler, "GET");

    net.serve();
}
```

---

### Dynamic Routes

```rads
blast user_handler(path, method, body, query, params, headers, cookies) {
    turbo id = net.param_get(params, "id");

    turbo html = "<html><body>";
    html = html + "<h1>User Profile</h1>";
    html = html + "<p>User ID: " + id + "</p>";
    html = html + "</body></html>";

    return [200, html, "text/html"];
}

blast main() {
    turbo server = net.http_server("localhost", 3000);
    net.route(server, "/user/:id", user_handler, "GET");

    net.serve();
}
```

---

## 💾 Database Integration

### Simple CRUD

```rads
blast main() {
    web_init();
    db.open("myapp.db");

    // Create
    db.execute("CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY,
        name TEXT
    )");

    // Insert
    db.execute("INSERT INTO users VALUES (1, 'Alice')");
    db.execute("INSERT INTO users VALUES (2, 'Bob')");

    // Read
    db.query("SELECT * FROM users");

    // Update
    db.execute("UPDATE users SET name = 'Charlie' WHERE id = 1");

    // Delete
    db.execute("DELETE FROM users WHERE id = 2");

    db.close();
}
```

---

## 🎨 HTML + CSS Examples

### Styled Page

```rads
blast page_handler(path, method, body, query, params, headers, cookies) {
    turbo html = "<!DOCTYPE html><html><head>";

    // CSS
    html = html + "<style>";
    html = html + "body {";
    html = html + "  background: linear-gradient(135deg, #667eea, #764ba2);";
    html = html + "  color: white;";
    html = html + "  font-family: Arial, sans-serif;";
    html = html + "  padding: 50px;";
    html = html + "}";
    html = html + "h1 { font-size: 3em; }";
    html = html + "</style>";

    html = html + "</head><body>";
    html = html + "<h1>🦋 RADS Web Engine</h1>";
    html = html + "<p>Beautiful gradient background!</p>";
    html = html + "</body></html>";

    return [200, html, "text/html"];
}

blast main() {
    turbo server = net.http_server("localhost", 3000);
    net.route(server, "/", page_handler, "GET");

    net.serve();
}
```

---

### Form Handling

```rads
blast form_get(path, method, body, query, params, headers, cookies) {
    turbo html = "<html><body>";
    html = html + "<form action='/submit' method='POST'>";
    html = html + "  <input type='text' name='name' placeholder='Your name'>";
    html = html + "  <button type='submit'>Submit</button>";
    html = html + "</form>";
    html = html + "</body></html>";

    return [200, html, "text/html"];
}

blast form_post(path, method, body, query, params, headers, cookies) {
    turbo form_data = net.form_parse(body);
    turbo name = net.form_get(form_data, "name");

    turbo html = "<html><body>";
    html = html + "<h1>Hello, " + name + "!</h1>";
    html = html + "<a href='/'>Back</a>";
    html = html + "</body></html>";

    return [200, html, "text/html"];
}

blast main() {
    turbo server = net.http_server("localhost", 3000);
    net.route(server, "/", form_get, "GET");
    net.route(server, "/submit", form_post, "POST");

    net.serve();
}
```

---

## 🔌 Plugin Examples

### Markdown Blog

```rads
blast main() {
    web_init();
    db.open("blog.db");

    turbo markdown = pkg_load("rads-plugin-markdown");

    // In full implementation:
    // turbo content = "# My Blog Post";
    // turbo html = markdown.render(content);

    echo("Blog system initialized!");
}
```

---

### Template Engine

```rads
blast main() {
    web_init();

    turbo template = pkg_load("rads-plugin-template");

    // In full implementation:
    // turbo html = template.render("page.html", {
    //     "title": "My Site",
    //     "content": "Welcome!"
    // });

    echo("Template engine loaded!");
}
```

---

## 🚀 Full Application Example

See: `webengine/examples/05-task-manager/task_manager.rads`

Complete task manager with:
- ✅ Pure RADS + HTML + CSS
- ✅ SQLite database
- ✅ Beautiful UI with animations
- ✅ CRUD operations
- ✅ RESTful API
- ✅ Form handling

**Run:**
```bash
rads webengine/examples/05-task-manager/task_manager.rads
```

**Visit:** http://localhost:5050

---

## 📖 More Examples

- Basic: `webengine/examples/01-basic/`
- JavaScript: `webengine/examples/02-javascript/`
- HTML/CSS: `webengine/examples/03-html-css/`
- Plugins: `webengine/examples/04-plugins/`
- Task Manager: `webengine/examples/05-task-manager/`

---

*RADS Web Engine Examples - v0.0.3 "Butterfly"* 🦋
