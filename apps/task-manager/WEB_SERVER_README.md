# 🦋 RADS Task Manager Web Server

**TURBO & RADICAL Web-Based Task Management!**

## 🚀 Quick Start

```bash
cd apps/task-manager
rads web-server.rads
```

Then open your browser to: **http://localhost:5050**

## ✨ Features

This application combines **two major RADS features**:

1. **Web Framework (v0.0.1)** - HTTP server, routing, form handling
2. **SQLite Database (v0.0.3)** - Persistent storage, transactions

### What You Can Do

- ✅ **Add Tasks** - Create new tasks via web form
- ✅ **Mark Complete** - Click to mark tasks as done
- ✅ **Delete Tasks** - Remove tasks you don't need
- ✅ **Persistent Storage** - All data saved to SQLite database
- ✅ **Beautiful UI** - Modern gradient design with animations
- ✅ **REST API** - JSON API at `/api/tasks`

## 📡 API Endpoints

### GET `/`
Main task manager interface with beautiful UI

### GET `/api/tasks`
Returns all tasks as JSON

**Response:**
```json
{
  "tasks": [
    {"id": 1, "description": "Build RADS v0.0.3 Butterfly", "done": 1},
    {"id": 2, "description": "Create web task manager", "done": 0}
  ],
  "count": 2,
  "version": "v0.0.3 Butterfly"
}
```

### POST `/add`
Add a new task

**Form Data:**
- `description` - Task description (required)

### POST `/complete/:id`
Mark a task as complete

**Parameters:**
- `id` - Task ID to complete

### POST `/delete/:id`
Delete a task

**Parameters:**
- `id` - Task ID to delete

## 🎨 User Interface

The web interface features:

- **Purple gradient background** - Modern and eye-catching
- **Card-based layout** - Clean, organized content
- **Smooth animations** - Hover effects and transitions
- **Responsive design** - Works on all screen sizes
- **Task status indicators** - Visual feedback for completed tasks

## 🗄️ Database

**Database File:** `tasks_web.db`

**Schema:**
```sql
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    description TEXT NOT NULL,
    done INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## 🧪 Testing

### Test with curl

**Get all tasks:**
```bash
curl http://localhost:5050/api/tasks
```

**View main page:**
```bash
curl http://localhost:5050
```

### Test with browser

1. Open http://localhost:5050
2. Add a task using the form
3. Click "✓ Complete" to mark it done
4. Click "🗑 Delete" to remove it

## 📊 Sample Data

The server automatically adds sample tasks on first run:

1. ✅ Build RADS v0.0.3 Butterfly (completed)
2. ✅ Create SQLite database driver (completed)
3. ⏳ Build web-based task manager (pending)
4. ⏳ Deploy to localhost:5050 (pending)

## 🔧 Technical Details

**Language:** RADS v0.0.3 "Butterfly"
**Backend:** SQLite3 database
**Frontend:** HTML5 + CSS3 + Vanilla JavaScript
**Server:** libuv-based HTTP server
**Port:** 5050

**Features Used:**
- `net.http_server()` - Create HTTP server
- `net.route()` - Define routes
- `net.serve()` - Start event loop
- `net.form_parse()` - Parse POST data
- `net.param_get()` - Extract route parameters
- `db.open()` - Open database
- `db.execute()` - Execute SQL
- `db.query()` - Query database

## 🛠️ Development

The server runs in an event loop and handles:

- **GET requests** - Serve HTML and JSON
- **POST requests** - Process form submissions
- **Route parameters** - Dynamic URLs like `/complete/:id`
- **Static content** - Inline CSS and JavaScript
- **Database operations** - All CRUD operations

## 📝 Code Structure

```
web-server.rads (220 lines)
├── init_db()                  - Initialize SQLite database
├── home_handler()             - Main page with UI
├── api_tasks_handler()        - JSON API endpoint
├── add_task_handler()         - Add new task
├── complete_task_handler()    - Mark task complete
├── delete_task_handler()      - Delete task
└── main()                     - Server setup and start
```

## 🎯 Use Cases

This demonstrates how to build:

1. **Full-stack web apps** with RADS
2. **RESTful APIs** with JSON responses
3. **Database-backed applications** with SQLite
4. **Form handling** with POST requests
5. **Dynamic routing** with parameters
6. **Modern UIs** with inline CSS

## 🚦 Server Status

**When Running:**
- Server: http://localhost:5050 ✓
- API: http://localhost:5050/api/tasks ✓
- Database: tasks_web.db ✓

**To Stop:**
Press `Ctrl+C` in the terminal

## 🌟 Next Steps

**Enhancements you could add:**

1. **Due dates** - Add deadline tracking
2. **Priorities** - High/medium/low
3. **Categories** - Organize by project
4. **Search** - Filter tasks by keyword
5. **User auth** - Multi-user support
6. **Export** - Download as JSON/CSV
7. **Statistics** - Charts and graphs
8. **Dark mode** - Theme switcher

## 🎓 Learning Points

This application teaches:

- HTTP request handling in RADS
- Database integration patterns
- Form data processing
- Route parameters and dynamic URLs
- JSON API design
- HTML templating in code
- Event-driven server architecture

## 📄 Files

- `web-server.rads` - Main server application
- `tasks_web.db` - SQLite database (auto-created)
- `WEB_SERVER_README.md` - This file

## 💡 Tips

1. **Database persists** - Your tasks are saved between server restarts
2. **Sample data** - Uses `INSERT OR IGNORE` to avoid duplicates
3. **Auto-redirect** - Forms redirect back to home after submission
4. **AJAX loading** - Tasks load dynamically via fetch API
5. **Inline styles** - All CSS embedded for single-file deployment

## 🐛 Troubleshooting

**Port already in use?**
```bash
# Find and kill the process
lsof -ti:5050 | xargs kill
```

**Database locked?**
```bash
# Remove the database and restart
rm tasks_web.db
rads web-server.rads
```

**Server not responding?**
- Check if process is running: `ps aux | grep rads`
- Check server output for errors
- Try accessing: `curl http://localhost:5050`

## 🏆 Achievement Unlocked

You've successfully deployed a **full-stack web application** using RADS that combines:

- ✅ HTTP server
- ✅ SQLite database
- ✅ RESTful API
- ✅ Beautiful UI
- ✅ Form handling
- ✅ Dynamic routing

**All in 220 lines of RADS code!** 🦋

---

*Keep it TURBO, keep it RADICAL!* 🚀✨

**RADS v0.0.3 "Butterfly"**
**January 11, 2026**
