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

static StructInstance* create_row_instance(sqlite3_stmt* stmt) {
    StructInstance* inst = calloc(1, sizeof(StructInstance));
    if (!inst) return NULL;

    int col_count = sqlite3_column_count(stmt);
    FieldValue* tail = NULL;

    for (int i = 0; i < col_count; i++) {
        FieldValue* fv = calloc(1, sizeof(FieldValue));
        if (!fv) continue;
        fv->name = strdup(sqlite3_column_name(stmt, i));

        Value* val = calloc(1, sizeof(Value));
        int type = sqlite3_column_type(stmt, i);

        switch (type) {
            case SQLITE_INTEGER:
                val->type = VAL_INT;
                val->int_val = sqlite3_column_int64(stmt, i);
                break;
            case SQLITE_FLOAT:
                val->type = VAL_FLOAT;
                val->float_val = sqlite3_column_double(stmt, i);
                break;
            case SQLITE_TEXT: {
                const char* text = (const char*)sqlite3_column_text(stmt, i);
                val->type = VAL_STRING;
                val->string_val = text ? strdup(text) : strdup("");
                break;
            }
            case SQLITE_NULL:
            default:
                val->type = VAL_NULL;
                break;
        }

        fv->value = val;

        if (tail) {
            tail->next = fv;
        } else {
            inst->fields = fv;
        }
        tail = fv;
    }

    return inst;
}

// Bind parameters from a RADS array (args[1]) to a prepared statement
static int bind_params(sqlite3_stmt* stmt, Value* params_arr) {
    if (!params_arr || params_arr->type != VAL_ARRAY) return 0;
    Array* arr = params_arr->array_val;
    for (size_t i = 0; i < arr->count; i++) {
        int idx = (int)i + 1; // SQLite params are 1-indexed
        Value* v = &arr->items[i];
        int rc;
        switch (v->type) {
            case VAL_INT:
                rc = sqlite3_bind_int64(stmt, idx, v->int_val);
                break;
            case VAL_FLOAT:
                rc = sqlite3_bind_double(stmt, idx, v->float_val);
                break;
            case VAL_STRING:
                rc = sqlite3_bind_text(stmt, idx, v->string_val, -1, SQLITE_TRANSIENT);
                break;
            case VAL_NULL:
                rc = sqlite3_bind_null(stmt, idx);
                break;
            case VAL_BOOL:
                rc = sqlite3_bind_int(stmt, idx, v->bool_val ? 1 : 0);
                break;
            default:
                rc = sqlite3_bind_text(stmt, idx, "", -1, SQLITE_TRANSIENT);
                break;
        }
        if (rc != SQLITE_OK) return rc;
    }
    return SQLITE_OK;
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

    // If params provided, use prepared statement for proper binding
    if (argc >= 2 && args[1].type == VAL_ARRAY) {
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(current_db->db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "\033[1;31mSQL Error:\033[0m %s\n", sqlite3_errmsg(current_db->db));
            Value v = {0}; v.type = VAL_NULL; return v;
        }
        rc = bind_params(stmt, &args[1]);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "\033[1;31mBind Error:\033[0m %s\n", sqlite3_errmsg(current_db->db));
            sqlite3_finalize(stmt);
            Value v = {0}; v.type = VAL_NULL; return v;
        }
        sqlite3_step(stmt);
        int affected = sqlite3_changes(current_db->db);
        sqlite3_finalize(stmt);
        Value result = {0}; result.type = VAL_INT; result.int_val = affected;
        return result;
    }

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

    // Bind parameters if provided
    if (argc >= 2 && args[1].type == VAL_ARRAY) {
        int brc = bind_params(stmt, &args[1]);
        if (brc != SQLITE_OK) {
            fprintf(stderr, "\033[1;31mBind Error:\033[0m %s\n", sqlite3_errmsg(current_db->db));
            sqlite3_finalize(stmt);
            Value v = {0}; v.type = VAL_NULL; return v;
        }
    }

    // Build result array of struct instances
    Array* rows = array_create(16);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StructInstance* inst = create_row_instance(stmt);
        if (inst) {
            Value row = {0};
            row.type = VAL_STRUCT_INSTANCE;
            row.struct_instance = inst;
            array_push(rows, row);
        }
    }

    sqlite3_finalize(stmt);

    Value result = {0};
    result.type = VAL_ARRAY;
    result.array_val = rows;
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
}
