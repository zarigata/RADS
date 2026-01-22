#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "../core/interpreter.h"
#include "stdlib_db.h"

// ============================================================================
    // RADS v0.0.5 "CHAMELEON" - SQLite Database Driver
// ============================================================================
//
// Provides elegant database connectivity for building real applications.
//
// API:
//   db.open(path) - Open database connection
//   db.query(sql, params) - Execute SELECT query, returns array of rows
//   db.execute(sql, params) - Execute INSERT/UPDATE/DELETE, returns affected rows
//   db.begin() - Start transaction
//   db.commit() - Commit transaction
//   db.rollback() - Rollback transaction
//   db.close() - Close database connection
//
// ============================================================================

// Database connection handle stored as opaque pointer
typedef struct {
    sqlite3* db;
    int in_transaction;
} DBHandle;

// Global database handle (single connection for now)
static DBHandle* current_db = NULL;

// ============================================================================
// Database Error Helper
// ============================================================================
static void print_db_error(const char* message, sqlite3* db) {
    const char* err = sqlite3_errmsg(db);
    fprintf(stderr, "\033[1;31m[DB ERROR]\033[0m %s: %s\n", message, err ? err : "Unknown error");
}

// ============================================================================
// Database Row Object Creation (for future use)
// ============================================================================

static Value __attribute__((unused)) create_row_object(sqlite3_stmt* stmt) {
    // Create a struct-like object representing a database row
    // For now, we'll use a simple approach with a custom type
    // In future, this could be enhanced with proper struct support

    Value row = {0};
    row.type = VAL_NULL; // Placeholder - will implement proper row objects

    // For demonstration, we'll just print the row data
    // TODO: Create proper struct instance with column values

    return row;
 }

// ============================================================================
// db.open(path) - Open database connection
// ============================================================================
// Opens a SQLite database file. Creates the file if it doesn't exist.
// Special path ":memory:" creates an in-memory database.
//
// Examples:
//   turbo db = db.open("app.db");
//   turbo memdb = db.open(":memory:");
// ============================================================================

Value native_db_open(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; // Unused for now

    if (argc != 1) {
        fprintf(stderr, "Error: db.open() requires 1 argument (path)\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: db.open() path must be a string\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* path = args[0].string_val;

    // Close existing connection if any
    if (current_db != NULL) {
        sqlite3_close(current_db->db);
        free(current_db);
    }

    // Allocate new handle
    current_db = malloc(sizeof(DBHandle));
    current_db->in_transaction = 0;

    // Open database
    int rc = sqlite3_open(path, &current_db->db);
    if (rc != SQLITE_OK) {
        print_db_error("Failed to open database", current_db->db);
        free(current_db);
        current_db = NULL;
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    printf("\033[1;32m✓\033[0m Database opened: %s\n", path);

    // Return success (boolean true)
    Value result = {0};
    result.type = VAL_BOOL;
    result.bool_val = 1;
    return result;
}

// ============================================================================
// db.execute(sql, params) - Execute non-SELECT query
// ============================================================================
// Executes INSERT, UPDATE, DELETE, or other SQL statements.
// Returns the number of affected rows.
//
// Examples:
//   db.execute("CREATE TABLE users (id INTEGER, name TEXT)");
//   db.execute("INSERT INTO users VALUES (?, ?)", [1, "Alice"]);
//   turbo rows = db.execute("UPDATE users SET name = ? WHERE id = ?", ["Bob", 1]);
// ============================================================================

Value native_db_execute(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; // Unused

    if (current_db == NULL) {
        fprintf(stderr, "Error: No database connection. Call db.open() first.\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    if (argc < 1 || argc > 2) {
        fprintf(stderr, "Error: db.execute() requires 1-2 arguments (sql, params?)\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: db.execute() sql must be a string\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* sql = args[0].string_val;

    // For now, simple exec without parameter binding
    // TODO: Add parameter binding from args[1] array

    char* err_msg = NULL;
    int rc = sqlite3_exec(current_db->db, sql, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "\033[1;31mSQL Error:\033[0m %s\n", err_msg);
        sqlite3_free(err_msg);
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    // Return number of affected rows
    int affected = sqlite3_changes(current_db->db);

    Value result = {0};
    result.type = VAL_INT;
    result.int_val = affected;
    return result;
}

// ============================================================================
// db.query(sql, params) - Execute SELECT query
// ============================================================================
// Executes a SELECT query and returns an array of row objects.
// Each row is a struct with column names as fields.
//
// Examples:
//   turbo users = db.query("SELECT * FROM users");
//   turbo adults = db.query("SELECT * FROM users WHERE age > ?", [18]);
// ============================================================================

Value native_db_query(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; // Unused

    if (current_db == NULL) {
        fprintf(stderr, "Error: No database connection. Call db.open() first.\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    if (argc < 1 || argc > 2) {
        fprintf(stderr, "Error: db.query() requires 1-2 arguments (sql, params?)\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: db.query() sql must be a string\n");
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    const char* sql = args[0].string_val;

    // Prepare statement
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(current_db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "\033[1;31mSQL Error:\033[0m %s\n", sqlite3_errmsg(current_db->db));
        Value v = {0};
        v.type = VAL_NULL;
        return v;
    }

    // TODO: Bind parameters from args[1] if provided

    // For now, print results directly (will return proper array later)
    int col_count = sqlite3_column_count(stmt);
    int row_count = 0;

    // Print header
    printf("\n");
    for (int i = 0; i < col_count; i++) {
        printf("%-15s ", sqlite3_column_name(stmt, i));
    }
    printf("\n");
    for (int i = 0; i < col_count * 16; i++) {
        printf("-");
    }
    printf("\n");

    // Print rows
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (int i = 0; i < col_count; i++) {
            const char* val = (const char*)sqlite3_column_text(stmt, i);
            printf("%-15s ", val ? val : "NULL");
        }
        printf("\n");
        row_count++;
    }
    printf("\n");

    sqlite3_finalize(stmt);

    // Return row count for now
    // TODO: Return actual array of row objects
    Value result = {0};
    result.type = VAL_INT;
    result.int_val = row_count;
    return result;
}

// ============================================================================
// db.begin() - Start transaction
// ============================================================================

Value native_db_begin(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;

    Value result = {0};
    result.type = VAL_BOOL;

    if (current_db == NULL) {
        fprintf(stderr, "Error: No database connection\n");
        result.type = VAL_NULL;
        return result;
    }

    if (current_db->in_transaction) {
        fprintf(stderr, "Warning: Already in transaction\n");
        result.bool_val = 0;
        return result;
    }

    char* err_msg = NULL;
    int rc = sqlite3_exec(current_db->db, "BEGIN TRANSACTION", NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "\033[1;31mTransaction Error:\033[0m %s\n", err_msg);
        sqlite3_free(err_msg);
        result.bool_val = 0;
        return result;
    }

    current_db->in_transaction = 1;
    printf("\033[1;36m→\033[0m Transaction started\n");
    result.bool_val = 1;
    return result;
}

// ============================================================================
// db.commit() - Commit transaction
// ============================================================================

Value native_db_commit(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;

    Value result = {0};
    result.type = VAL_BOOL;

    if (current_db == NULL) {
        fprintf(stderr, "Error: No database connection\n");
        result.type = VAL_NULL;
        return result;
    }

    if (!current_db->in_transaction) {
        fprintf(stderr, "Warning: No active transaction\n");
        result.bool_val = 0;
        return result;
    }

    char* err_msg = NULL;
    int rc = sqlite3_exec(current_db->db, "COMMIT", NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "\033[1;31mCommit Error:\033[0m %s\n", err_msg);
        sqlite3_free(err_msg);
        result.bool_val = 0;
        return result;
    }

    current_db->in_transaction = 0;
    printf("\033[1;32m✓\033[0m Transaction committed\n");
    result.bool_val = 1;
    return result;
}

// ============================================================================
// db.rollback() - Rollback transaction
// ============================================================================

Value native_db_rollback(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;

    Value result = {0};
    result.type = VAL_BOOL;

    if (current_db == NULL) {
        fprintf(stderr, "Error: No database connection\n");
        result.type = VAL_NULL;
        return result;
    }

    if (!current_db->in_transaction) {
        fprintf(stderr, "Warning: No active transaction\n");
        result.bool_val = 0;
        return result;
    }

    char* err_msg = NULL;
    int rc = sqlite3_exec(current_db->db, "ROLLBACK", NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "\033[1;31mRollback Error:\033[0m %s\n", err_msg);
        sqlite3_free(err_msg);
        result.bool_val = 0;
        return result;
    }

    current_db->in_transaction = 0;
    printf("\033[1;33m↺\033[0m Transaction rolled back\n");
    result.bool_val = 1;
    return result;
}

// ============================================================================
// db.close() - Close database connection
// ============================================================================

Value native_db_close(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;

    Value result = {0};
    result.type = VAL_BOOL;

    if (current_db == NULL) {
        fprintf(stderr, "Warning: No database connection to close\n");
        result.bool_val = 0;
        return result;
    }

    sqlite3_close(current_db->db);
    free(current_db);
    current_db = NULL;

    printf("\033[1;34m✓\033[0m Database closed\n");
    result.bool_val = 1;
    return result;
}

// ============================================================================
// Registration
// ============================================================================

void stdlib_db_register(void) {
    // Register all database functions
    register_native("db.open", native_db_open);
    register_native("db.query", native_db_query);
    register_native("db.execute", native_db_execute);
    register_native("db.begin", native_db_begin);
    register_native("db.commit", native_db_commit);
    register_native("db.rollback", native_db_rollback);
    register_native("db.close", native_db_close);

    printf("\033[1;36m[DB]\033[0m stdlib_db_register\n");
}
