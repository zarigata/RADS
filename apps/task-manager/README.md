# 📋 RADS Task Manager

A beautiful command-line task management application built with **RADS v0.0.3 "Butterfly"** and SQLite.

## Features

✅ **Add Tasks** - Create new tasks with descriptions
✅ **List Tasks** - View all pending and completed tasks
✅ **Mark Complete** - Check off finished tasks
✅ **Delete Tasks** - Remove tasks you don't need
✅ **Persistent Storage** - SQLite database saves everything
✅ **Transactions** - Atomic bulk operations
✅ **Beautiful Output** - Colored terminal interface

## Installation

Make sure RADS v0.0.3 or later is installed:

```bash
$ rads --version
RADS v0.0.3 "Butterfly" 🦋
```

## Usage

### Run the Examples

**Simple Example (In-Memory Database):**
```bash
$ cd apps/task-manager
$ rads simple.rads
```

**Full Demo (Persistent Database):**
```bash
$ cd apps/task-manager
$ rads demo.rads
```

This will:
1. Create a `tasks.db` SQLite database
2. Add sample tasks
3. Demonstrate all features (queries, updates, transactions)
4. Show the final task list

### Important: main() Function Required

All RADS script files must have a `main()` function to execute:

```rads
blast main() {
    // Your code here
    echo("Hello, RADS!");
    db.open("mydb.db");
    // ...
}
```

**Note:** The REPL mode executes statements immediately and doesn't require a `main()` function.

### Interactive Mode (Coming Soon)

```bash
$ rads tasks-interactive.rads
```

## Example Output

```
  ╔════════════════════════════════════════════╗
  ║  📋 RADS Task Manager v1.0           ║
  ║  Keep it TURBO, keep it RADICAL!  ║
  ╚════════════════════════════════════════════╝

🦋 Initializing Task Manager Database...
✓ Database ready!

Demo Mode - Adding sample tasks:

✅ Added: Build RADS v0.0.3 Butterfly
✅ Added: Implement SQLite database driver
✅ Added: Create task manager app
✅ Added: Test all features
✅ Added: Deploy to production

Marking some tasks as complete:

✓ Task 1 marked as complete!
✓ Task 2 marked as complete!
✓ Task 3 marked as complete!

📋 Your Tasks:

id              description                              done
------------------------------------------------------------------------------
4               Test all features                        0
5               Deploy to production                     0
1               Build RADS v0.0.3 Butterfly              1
2               Implement SQLite database driver         1
3               Create task manager app                  1

🦋 Task Manager Demo Complete!
Database saved to: tasks.db
```

## Database Schema

```sql
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    description TEXT NOT NULL,
    done INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## API Functions

### `init_database()`
Initializes the SQLite database and creates the tasks table.

### `add_task(description)`
Adds a new task with the given description.

```rads
add_task("Write documentation");
```

### `list_tasks()`
Displays all tasks in a formatted table.

```rads
list_tasks();
```

### `mark_done(task_id)`
Marks a task as complete.

```rads
mark_done(1);
```

### `delete_task(task_id)`
Permanently deletes a task.

```rads
delete_task(1);
```

## Transaction Example

```rads
// Start transaction
db.begin();

// Add multiple tasks
add_task("Task 1");
add_task("Task 2");
add_task("Task 3");

// Commit all at once
db.commit();
```

## File Structure

```
apps/task-manager/
├── tasks.rads          - Main demo application
├── tasks-interactive.rads  - Interactive CLI (coming soon)
├── README.md           - This file
└── tasks.db            - SQLite database (created on first run)
```

## Requirements

- RADS v0.0.3 "Butterfly" or later
- SQLite3 library (automatically included)

## Technical Details

**Language**: RADS
**Database**: SQLite3
**Lines of Code**: ~150
**Features**: Full CRUD operations, transactions, colored output

## Future Enhancements

- [ ] Interactive CLI with user input
- [ ] Task priorities (high, medium, low)
- [ ] Due dates and reminders
- [ ] Categories/tags
- [ ] Search functionality
- [ ] Export to JSON/CSV
- [ ] Task statistics

## License

Part of the RADS programming language examples.

---

*Keep it TURBO, keep it RADICAL!* 🚀🦋
