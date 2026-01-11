# 🦋 RADS Applications

Real-world applications built with **RADS v0.0.3 "Butterfly"** to demonstrate the language's capabilities.

## 📦 Available Applications

### 1. Task Manager 📋
**Location**: `task-manager/`
**Features**: SQLite database, CRUD operations, transactions
**Complexity**: Beginner-Friendly

A command-line task management system with persistent storage.

```bash
cd task-manager
rads tasks.rads
```

### 2. Blog System (Coming Soon) 📝
**Location**: `blog/`
**Features**: Posts, comments, users, authentication
**Complexity**: Intermediate

A complete blogging platform with user management.

### 3. API Server (Coming Soon) 🌐
**Location**: `api-server/`
**Features**: REST API, JSON, rate limiting
**Complexity**: Advanced

A production-ready REST API server.

## 🎯 Purpose

These applications are:

- ✅ **Production Examples** - Real, usable software
- ✅ **Learning Resources** - Learn RADS by example
- ✅ **Feature Demonstrations** - Show what RADS can do
- ✅ **Testing Grounds** - Validate v0.0.3 features

## 📁 Structure

```
apps/
├── README.md            # This file
├── task-manager/        # Task management app
│   ├── tasks.rads
│   ├── README.md
│   └── tasks.db         # Created on first run
├── blog/               # Blog platform (coming soon)
└── api-server/         # REST API server (coming soon)
```

## 🚀 Getting Started

1. **Install RADS v0.0.3**:
   ```bash
   rads --version  # Should show v0.0.3 "Butterfly"
   ```

2. **Choose an app** and navigate to its folder:
   ```bash
   cd apps/task-manager
   ```

3. **Run it**:
   ```bash
   rads tasks.rads
   ```

## 📚 What You'll Learn

### Task Manager
- SQLite database integration
- CRUD operations (Create, Read, Update, Delete)
- Database transactions
- Colored terminal output
- Function definitions
- String concatenation

### Blog (Coming Soon)
- Multi-table relationships
- User authentication
- Session management
- Complex queries
- Data validation

### API Server (Coming Soon)
- HTTP server setup
- RESTful routing
- JSON parsing/generation
- Middleware
- Rate limiting

## 🛠️ Development Guidelines

**When creating new apps:**

1. **Separate Folder** - Each app gets its own directory
2. **README Required** - Document how to use it
3. **Real Use Cases** - Build actually useful applications
4. **Clean Code** - Follow RADS best practices
5. **Error Handling** - Handle edge cases gracefully

## 📊 Statistics

| Application | Lines of Code | Database Tables | Features |
|-------------|---------------|-----------------|----------|
| Task Manager | ~150 | 1 | CRUD, Transactions |
| Blog | TBD | 3-5 | Auth, Posts, Comments |
| API Server | TBD | 2-3 | REST, JSON, Middleware |

## 🎓 Learning Path

**Recommended order:**

1. **Start with Task Manager** - Learn the basics
2. **Move to Blog** - Understand relationships
3. **Build API Server** - Master async and networking

## 🤝 Contributing

Want to add your own RADS application?

1. Create a new folder in `apps/`
2. Build something awesome
3. Add a comprehensive README
4. Test it thoroughly
5. Share it!

## 📝 Template

Use this template for new apps:

```
apps/your-app/
├── README.md           # Usage instructions
├── main.rads           # Main application file
├── lib/               # Optional: shared functions
└── data/              # Optional: data files
```

## ⚡ Quick Examples

**Task Manager**:
```rads
db.open("tasks.db");
add_task("Learn RADS");
list_tasks();
db.close();
```

**Future Blog Example**:
```rads
db.open("blog.db");
turbo post = create_post("My First Post", "Hello World!");
publish_post(post.id);
db.close();
```

## 🔧 Requirements

- **RADS**: v0.0.3 "Butterfly" or later
- **SQLite3**: Included with RADS
- **Terminal**: Modern terminal with ANSI color support

## 📖 Documentation

Each application has its own README with:
- Installation instructions
- Usage examples
- API documentation
- Database schema
- Feature list

## 🎯 Goals

These applications demonstrate that RADS is:

- **Production-Ready** - Build real software
- **Easy to Learn** - Clear, readable syntax
- **Powerful** - Full database and networking support
- **Fun** - Enjoy the TURBO & RADICAL development experience!

---

**Keep it TURBO, keep it RADICAL!** 🚀🦋

*Built with RADS v0.0.3 "Butterfly"*
