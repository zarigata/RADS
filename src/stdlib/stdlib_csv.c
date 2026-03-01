#include "stdlib_csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// Helper to parse a single CSV field
static char* parse_field(const char** cursor, char delimiter, char quote, char escape) {
    const char* p = *cursor;
    
    // Skip leading whitespace
    while (*p && isspace((unsigned char)*p) && *p != delimiter) {
        p++;
    }
    
    // Check for quoted field
    if (*p == quote) {
        p++; // Skip opening quote
        
        size_t capacity = 64;
        size_t len = 0;
        char* result = malloc(capacity);
        
        while (*p) {
            if (*p == escape && *(p + 1)) {
                // Escaped character
                if (len + 1 >= capacity) {
                    capacity *= 2;
                    result = realloc(result, capacity);
                }
                p++; // Skip escape
                result[len++] = *p++;
            } else if (*p == quote) {
                if (*(p + 1) == quote) {
                    // Double quote = escaped quote
                    if (len + 1 >= capacity) {
                        capacity *= 2;
                        result = realloc(result, capacity);
                    }
                    result[len++] = quote;
                    p += 2;
                } else {
                    // End of quoted field
                    p++; // Skip closing quote
                    break;
                }
            } else {
                if (len + 1 >= capacity) {
                    capacity *= 2;
                    result = realloc(result, capacity);
                }
                result[len++] = *p++;
            }
        }
        
        result[len] = '\0';
        
        // Skip to delimiter or end
        while (*p && *p != delimiter && *p != '\n' && *p != '\r') {
            p++;
        }
        if (*p == delimiter) p++;
        
        *cursor = p;
        return result;
    } else {
        // Unquoted field
        size_t capacity = 64;
        size_t len = 0;
        char* result = malloc(capacity);
        
        while (*p && *p != delimiter && *p != '\n' && *p != '\r') {
            if (len + 1 >= capacity) {
                capacity *= 2;
                result = realloc(result, capacity);
            }
            result[len++] = *p++;
        }
        
        // Trim trailing whitespace
        while (len > 0 && isspace((unsigned char)result[len - 1])) {
            len--;
        }
        
        result[len] = '\0';
        
        if (*p == delimiter) p++;
        
        *cursor = p;
        return result;
    }
}

// Helper to parse a CSV line into fields
static Array* parse_line(const char** cursor, char delimiter, char quote, char escape) {
    const char* p = *cursor;
    
    if (*p == '\0') {
        return NULL;
    }
    
    Array* fields = array_create(8);
    
    while (*p && *p != '\n' && *p != '\r') {
        char* field = parse_field(&p, delimiter, quote, escape);
        if (field) {
            Value v;
            v.type = VAL_STRING;
            v.string_val = field;
            array_push(fields, v);
        }
    }
    
    // Skip newline characters
    if (*p == '\r') p++;
    if (*p == '\n') p++;
    
    *cursor = p;
    return fields;
}

// Helper to write a quoted CSV field
static void write_quoted_field(FILE* fp, const char* field, char quote, char escape) {
    fputc(quote, fp);
    
    for (const char* p = field; *p; p++) {
        if (*p == quote) {
            fputc(quote, fp);  // Double the quote
        } else if (*p == escape) {
            fputc(escape, fp);
        }
        fputc(*p, fp);
    }
    
    fputc(quote, fp);
}

// csv.parse(str) or csv.parse(str, opts) - Parse CSV string into array of structs
Value stdlib_csv_parse(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: csv.parse() requires a string argument\n");
        return make_null();
    }
    
    // Default options
    char delimiter = ',';
    bool has_header = true;
    char quote = '"';
    char escape = '\\';
    
    // Parse options if provided
    if (argc >= 2 && args[1].type == VAL_STRUCT_INSTANCE) {
        StructInstance* opts = args[1].struct_instance;
        FieldValue* field = opts->fields;
        
        while (field) {
            if (strcmp(field->name, "delimiter") == 0 && field->value->type == VAL_STRING) {
                delimiter = field->value->string_val[0];
            } else if (strcmp(field->name, "header") == 0 && field->value->type == VAL_BOOL) {
                has_header = field->value->bool_val;
            } else if (strcmp(field->name, "quote") == 0 && field->value->type == VAL_STRING) {
                quote = field->value->string_val[0];
            } else if (strcmp(field->name, "escape") == 0 && field->value->type == VAL_STRING) {
                escape = field->value->string_val[0];
            }
            field = field->next;
        }
    }
    
    const char* cursor = args[0].string_val;
    
    // Parse header row if present
    Array* headers = NULL;
    if (has_header) {
        headers = parse_line(&cursor, delimiter, quote, escape);
    }
    
    // Create result array
    Array* result = array_create(16);
    
    // Parse data rows
    while (*cursor) {
        Array* row = parse_line(&cursor, delimiter, quote, escape);
        if (!row) break;
        
        // Create struct instance for this row
        StructInstance* instance = malloc(sizeof(StructInstance));
        instance->fields = NULL;
        FieldValue* last_field = NULL;
        
        for (size_t i = 0; i < row->count; i++) {
            FieldValue* field = malloc(sizeof(FieldValue));
            field->value = malloc(sizeof(Value));
            field->value->type = VAL_STRING;
            field->value->string_val = strdup(row->items[i].string_val);
            field->next = NULL;
            
            if (headers && i < headers->count) {
                field->name = strdup(headers->items[i].string_val);
            } else {
                // Generate field name
                char name[32];
                snprintf(name, sizeof(name), "field_%zu", i);
                field->name = strdup(name);
            }
            
            if (instance->fields == NULL) {
                instance->fields = field;
            } else {
                last_field->next = field;
            }
            last_field = field;
        }
        
        // Free row array
        for (size_t i = 0; i < row->count; i++) {
            free(row->items[i].string_val);
        }
        free(row->items);
        free(row);
        
        // Add struct to result
        Value item;
        item.type = VAL_STRUCT_INSTANCE;
        item.struct_instance = instance;
        array_push(result, item);
    }
    
    // Free headers if allocated
    if (headers) {
        for (size_t i = 0; i < headers->count; i++) {
            free(headers->items[i].string_val);
        }
        free(headers->items);
        free(headers);
    }
    
    Value val;
    val.type = VAL_ARRAY;
    val.array_val = result;
    return val;
}

// csv.stringify(data) or csv.stringify(data, opts) - Convert array of structs to CSV
Value stdlib_csv_stringify(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: csv.stringify() requires an array argument\n");
        return make_null();
    }
    
    // Default options
    char delimiter = ',';
    bool has_header = true;
    char quote = '"';
    char escape = '\\';
    
    // Parse options if provided
    if (argc >= 2 && args[1].type == VAL_STRUCT_INSTANCE) {
        StructInstance* opts = args[1].struct_instance;
        FieldValue* field = opts->fields;
        
        while (field) {
            if (strcmp(field->name, "delimiter") == 0 && field->value->type == VAL_STRING) {
                delimiter = field->value->string_val[0];
            } else if (strcmp(field->name, "header") == 0 && field->value->type == VAL_BOOL) {
                has_header = field->value->bool_val;
            } else if (strcmp(field->name, "quote") == 0 && field->value->type == VAL_STRING) {
                quote = field->value->string_val[0];
            } else if (strcmp(field->name, "escape") == 0 && field->value->type == VAL_STRING) {
                escape = field->value->string_val[0];
            }
            field = field->next;
        }
    }
    
    Array* data = args[0].array_val;
    if (data->count == 0) {
        return make_string("");
    }
    
    // Collect all field names from first struct
    Array* field_names = array_create(16);
    if (data->items[0].type == VAL_STRUCT_INSTANCE) {
        StructInstance* first = data->items[0].struct_instance;
        FieldValue* field = first->fields;
        
        // Count and collect field names in order
        // Note: struct fields are stored as linked list, we preserve insertion order
        while (field) {
            Value name;
            name.type = VAL_STRING;
            name.string_val = strdup(field->name);
            array_push(field_names, name);
            field = field->next;
        }
    }
    
    // Build result string
    size_t capacity = 1024;
    size_t len = 0;
    char* result = malloc(capacity);
    result[0] = '\0';
    
    // Write header if needed
    if (has_header && field_names->count > 0) {
        for (size_t i = 0; i < field_names->count; i++) {
            if (i > 0) {
                if (len + 1 >= capacity) {
                    capacity *= 2;
                    result = realloc(result, capacity);
                }
                result[len++] = delimiter;
            }
            const char* name = field_names->items[i].string_val;
            size_t name_len = strlen(name);
            if (len + name_len + 1 >= capacity) {
                capacity = len + name_len + 1024;
                result = realloc(result, capacity);
            }
            memcpy(result + len, name, name_len);
            len += name_len;
        }
        if (len + 1 >= capacity) {
            capacity *= 2;
            result = realloc(result, capacity);
        }
        result[len++] = '\n';
    }
    
    // Write data rows
    for (size_t i = 0; i < data->count; i++) {
        if (data->items[i].type != VAL_STRUCT_INSTANCE) {
            continue;
        }
        
        StructInstance* row = data->items[i].struct_instance;
        
        // Build lookup for this row's fields
        // We need to output fields in the same order as field_names
        
        for (size_t j = 0; j < field_names->count; j++) {
            if (j > 0) {
                if (len + 1 >= capacity) {
                    capacity *= 2;
                    result = realloc(result, capacity);
                }
                result[len++] = delimiter;
            }
            
            const char* field_name = field_names->items[j].string_val;
            
            // Find field value in struct
            const char* field_value = NULL;
            FieldValue* f = row->fields;
            while (f) {
                if (strcmp(f->name, field_name) == 0) {
                    if (f->value->type == VAL_STRING) {
                        field_value = f->value->string_val;
                    } else if (f->value->type == VAL_INT) {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%lld", f->value->int_val);
                        field_value = strdup(buf);
                    } else if (f->value->type == VAL_FLOAT) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%g", f->value->float_val);
                        field_value = strdup(buf);
                    } else if (f->value->type == VAL_BOOL) {
                        field_value = f->value->bool_val ? "true" : "false";
                    }
                    break;
                }
                f = f->next;
            }
            
            if (field_value) {
                // Check if field needs quoting
                bool needs_quote = strchr(field_value, delimiter) != NULL ||
                                   strchr(field_value, quote) != NULL ||
                                   strchr(field_value, '\n') != NULL;
                
                if (needs_quote) {
                    // Calculate quoted size
                    size_t val_len = strlen(field_value);
                    size_t quoted_size = val_len + 4; // quotes + some escapes
                    
                    if (len + quoted_size + 1 >= capacity) {
                        capacity = len + quoted_size + 1024;
                        result = realloc(result, capacity);
                    }
                    
                    // Write quoted
                    char* dest = result + len;
                    *dest++ = quote;
                    for (const char* p = field_value; *p; p++) {
                        if (*p == quote) {
                            *dest++ = quote;
                            if (dest - result >= (ptrdiff_t)capacity - 2) {
                                capacity *= 2;
                                result = realloc(result, capacity);
                                dest = result + len + (dest - (result + len));
                            }
                        }
                        *dest++ = *p;
                    }
                    *dest++ = quote;
                    len = dest - result;
                } else {
                    size_t val_len = strlen(field_value);
                    if (len + val_len + 1 >= capacity) {
                        capacity = len + val_len + 1024;
                        result = realloc(result, capacity);
                    }
                    memcpy(result + len, field_value, val_len);
                    len += val_len;
                }
            }
        }
        
        if (len + 1 >= capacity) {
            capacity *= 2;
            result = realloc(result, capacity);
        }
        result[len++] = '\n';
    }
    
    result[len] = '\0';
    
    // Free field names
    for (size_t i = 0; i < field_names->count; i++) {
        free(field_names->items[i].string_val);
    }
    free(field_names->items);
    free(field_names);
    
    Value val;
    val.type = VAL_STRING;
    val.string_val = result;
    return val;
}

// csv.read(path) or csv.read(path, opts) - Read CSV file
Value stdlib_csv_read(Interpreter* interp, int argc, Value* args) {
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: csv.read() requires a file path argument\n");
        return make_null();
    }
    
    const char* path = args[0].string_val;
    
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error: csv.read() cannot open file: %s\n", path);
        return make_null();
    }
    
    // Read entire file
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* content = malloc(file_size + 1);
    size_t read_size = fread(content, 1, file_size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    // Create string value and call parse
    Value content_val;
    content_val.type = VAL_STRING;
    content_val.string_val = content;
    
    Value result;
    if (argc >= 2) {
        Value parse_args[2] = { content_val, args[1] };
        result = stdlib_csv_parse(interp, 2, parse_args);
    } else {
        result = stdlib_csv_parse(interp, 1, &content_val);
    }
    
    free(content);
    return result;
}

// csv.write(path, data) or csv.write(path, data, opts) - Write CSV file
Value stdlib_csv_write(Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_ARRAY) {
        fprintf(stderr, "Error: csv.write() requires a file path and data array\n");
        return make_bool(false);
    }
    
    const char* path = args[0].string_val;
    
    // Get CSV string
    Value csv_string;
    if (argc >= 3) {
        Value stringify_args[2] = { args[1], args[2] };
        csv_string = stdlib_csv_stringify(interp, 2, stringify_args);
    } else {
        csv_string = stdlib_csv_stringify(interp, 1, &args[1]);
    }
    
    if (csv_string.type != VAL_STRING) {
        return make_bool(false);
    }
    
    FILE* fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Error: csv.write() cannot open file for writing: %s\n", path);
        free(csv_string.string_val);
        return make_bool(false);
    }
    
    fprintf(fp, "%s", csv_string.string_val);
    fclose(fp);
    
    free(csv_string.string_val);
    return make_bool(true);
}

void stdlib_csv_register(void) {
    register_native("csv.parse", stdlib_csv_parse);
    register_native("csv.stringify", stdlib_csv_stringify);
    register_native("csv.read", stdlib_csv_read);
    register_native("csv.write", stdlib_csv_write);
}