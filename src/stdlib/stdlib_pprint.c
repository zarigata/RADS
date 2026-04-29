#include "stdlib_pprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_null(void);
extern void register_native(const char* name, NativeFn fn);
extern void value_print(Value* value);

/* Forward declarations from value.h types */
typedef struct FieldValue FieldValue;
typedef struct StructInstance StructInstance;

// ── Internal: format a value into a dynamically-growing string ──

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} Buf;

static void buf_init(Buf* b) {
    b->cap = 256;
    b->data = malloc(b->cap);
    b->len = 0;
    b->data[0] = '\0';
}

static void buf_append(Buf* b, const char* s) {
    size_t slen = strlen(s);
    while (b->len + slen + 1 >= b->cap) {
        b->cap *= 2;
        b->data = realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, slen + 1);
    b->len += slen;
}

static void buf_appendn(Buf* b, const char* s, size_t n) {
    while (b->len + n + 1 >= b->cap) {
        b->cap *= 2;
        b->data = realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void format_value(Buf* b, Value* v, int indent, int depth, int max_depth) {
    if (depth > max_depth) {
        buf_append(b, "...");
        return;
    }

    char tmp[128];

    switch (v->type) {
        case VAL_NULL:
            buf_append(b, "null");
            break;
        case VAL_BOOL:
            buf_append(b, v->bool_val ? "true" : "false");
            break;
        case VAL_INT:
            snprintf(tmp, sizeof(tmp), "%lld", v->int_val);
            buf_append(b, tmp);
            break;
        case VAL_FLOAT:
            snprintf(tmp, sizeof(tmp), "%g", v->float_val);
            buf_append(b, tmp);
            break;
        case VAL_STRING:
            buf_append(b, "\"");
            /* escape special chars */
            for (const char* p = v->string_val; *p; p++) {
                switch (*p) {
                    case '"':  buf_append(b, "\\\""); break;
                    case '\\': buf_append(b, "\\\\"); break;
                    case '\n': buf_append(b, "\\n"); break;
                    case '\r': buf_append(b, "\\r"); break;
                    case '\t': buf_append(b, "\\t"); break;
                    default:
                        if ((unsigned char)*p < 0x20) {
                            snprintf(tmp, sizeof(tmp), "\\x%02x", (unsigned char)*p);
                            buf_append(b, tmp);
                        } else {
                            buf_appendn(b, p, 1);
                        }
                }
            }
            buf_append(b, "\"");
            break;
        case VAL_FUNCTION:
            buf_append(b, "<function>");
            break;
        case VAL_ARRAY: {
            Array* arr = v->array_val;
            if (arr->count == 0) {
                buf_append(b, "[]");
                break;
            }
            /* compact if small and no nested structures */
            int compact = 1;
            if (arr->count > 5) compact = 0;
            if (compact) {
                for (size_t i = 0; i < arr->count; i++) {
                    if (arr->items[i].type == VAL_ARRAY || arr->items[i].type == VAL_STRUCT_INSTANCE) {
                        compact = 0;
                        break;
                    }
                }
            }

            if (compact) {
                buf_append(b, "[");
                for (size_t i = 0; i < arr->count; i++) {
                    if (i > 0) buf_append(b, ", ");
                    format_value(b, &arr->items[i], indent, depth + 1, max_depth);
                }
                buf_append(b, "]");
            } else {
                buf_append(b, "[\n");
                for (size_t i = 0; i < arr->count; i++) {
                    /* indent */
                    for (int s = 0; s < indent + 2; s++) buf_append(b, " ");
                    format_value(b, &arr->items[i], indent + 2, depth + 1, max_depth);
                    if (i + 1 < arr->count) buf_append(b, ",");
                    buf_append(b, "\n");
                }
                for (int s = 0; s < indent; s++) buf_append(b, " ");
                buf_append(b, "]");
            }
            break;
        }
        case VAL_STRUCT_DEF:
            buf_append(b, "<struct_def ");
            buf_append(b, v->struct_def->name ? v->struct_def->name : "anonymous");
            buf_append(b, ">");
            break;
        case VAL_STRUCT_INSTANCE: {
            StructInstance* si = v->struct_instance;
            buf_append(b, "struct ");
            if (si->definition && si->definition->name) {
                buf_append(b, si->definition->name);
            }
            buf_append(b, " {\n");
            FieldValue* field = si->fields;
            while (field) {
                for (int s = 0; s < indent + 2; s++) buf_append(b, " ");
                buf_append(b, field->name);
                buf_append(b, ": ");
                format_value(b, field->value, indent + 2, depth + 1, max_depth);
                buf_append(b, "\n");
                field = field->next;
            }
            for (int s = 0; s < indent; s++) buf_append(b, " ");
            buf_append(b, "}");
            break;
        }
        default:
            buf_append(b, "<unknown>");
    }
}

// ── pprint.format(value, indent=2, max_depth=10) ──
// Returns pretty-printed string representation
static Value pprint_format(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: pprint.format() requires at least one argument\n");
        return make_null();
    }
    int max_depth = 10;
    if (argc >= 2 && args[1].type == VAL_INT) max_depth = (int)args[1].int_val;
    if (argc >= 3 && args[2].type == VAL_INT) max_depth = (int)args[2].int_val;

    Buf b;
    buf_init(&b);
    format_value(&b, &args[0], 0, 0, max_depth);

    Value result;
    result.type = VAL_STRING;
    result.string_val = b.data;
    return result;
}

// ── pprint.print(value, indent=2, max_depth=10) ──
// Pretty-print to stdout
static Value pprint_print(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    Value formatted = pprint_format(interp, argc, args);
    if (formatted.type == VAL_STRING) {
        printf("%s\n", formatted.string_val);
        free(formatted.string_val);
    }
    return make_null();
}

// ── pprint.table(headers, rows) ──
// Print data as a text table. headers: array of strings, rows: array of arrays.
static Value pprint_table(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) {
        fprintf(stderr, "Error: pprint.table(headers, rows) requires two arrays\n");
        return make_null();
    }

    Array* headers = args[0].array_val;
    Array* rows = args[1].array_val;
    int ncols = (int)headers->count;
    if (ncols == 0) return make_null();

    /* compute column widths */
    int* widths = calloc(ncols, sizeof(int));
    char tmp[256];

    for (int c = 0; c < ncols; c++) {
        if (headers->items[c].type == VAL_STRING)
            widths[c] = (int)strlen(headers->items[c].string_val);
    }

    for (size_t r = 0; r < rows->count; r++) {
        if (rows->items[r].type != VAL_ARRAY) continue;
        Array* row = rows->items[r].array_val;
        for (int c = 0; c < ncols && c < (int)row->count; c++) {
            int w = 0;
            switch (row->items[c].type) {
                case VAL_STRING: w = (int)strlen(row->items[c].string_val); break;
                case VAL_INT: snprintf(tmp, sizeof(tmp), "%lld", row->items[c].int_val); w = (int)strlen(tmp); break;
                case VAL_FLOAT: snprintf(tmp, sizeof(tmp), "%g", row->items[c].float_val); w = (int)strlen(tmp); break;
                case VAL_BOOL: w = row->items[c].bool_val ? 4 : 5; break;
                case VAL_NULL: w = 4; break;
                default: w = 8; break;
            }
            if (w > widths[c]) widths[c] = w;
        }
    }

    /* print header */
    for (int c = 0; c < ncols; c++) {
        const char* h = (headers->items[c].type == VAL_STRING) ? headers->items[c].string_val : "?";
        printf("%-*s  ", widths[c], h);
    }
    printf("\n");

    /* print separator */
    for (int c = 0; c < ncols; c++) {
        for (int w = 0; w < widths[c]; w++) printf("-");
        printf("  ");
    }
    printf("\n");

    /* print rows */
    for (size_t r = 0; r < rows->count; r++) {
        if (rows->items[r].type != VAL_ARRAY) continue;
        Array* row = rows->items[r].array_val;
        for (int c = 0; c < ncols; c++) {
            if (c < (int)row->count) {
                switch (row->items[c].type) {
                    case VAL_STRING:
                        printf("%-*s  ", widths[c], row->items[c].string_val);
                        break;
                    case VAL_INT:
                        snprintf(tmp, sizeof(tmp), "%lld", row->items[c].int_val);
                        printf("%-*s  ", widths[c], tmp);
                        break;
                    case VAL_FLOAT:
                        snprintf(tmp, sizeof(tmp), "%g", row->items[c].float_val);
                        printf("%-*s  ", widths[c], tmp);
                        break;
                    case VAL_BOOL:
                        printf("%-*s  ", widths[c], row->items[c].bool_val ? "true" : "false");
                        break;
                    case VAL_NULL:
                        printf("%-*s  ", widths[c], "null");
                        break;
                    default:
                        printf("%-*s  ", widths[c], "<value>");
                }
            } else {
                printf("%-*s  ", widths[c], "");
            }
        }
        printf("\n");
    }

    free(widths);
    return make_null();
}

// ── pprint.json(value, indent=2) ──
// Format as JSON string
static void format_json_value(Buf* b, Value* v, int indent, int depth, int max_depth);

static void format_json_indent(Buf* b, int indent) {
    for (int i = 0; i < indent; i++) buf_append(b, " ");
}

static void format_json_value(Buf* b, Value* v, int indent, int depth, int max_depth) {
    char tmp[128];
    if (depth > max_depth) { buf_append(b, "null"); return; }

    switch (v->type) {
        case VAL_NULL: buf_append(b, "null"); break;
        case VAL_BOOL: buf_append(b, v->bool_val ? "true" : "false"); break;
        case VAL_INT:
            snprintf(tmp, sizeof(tmp), "%lld", v->int_val);
            buf_append(b, tmp);
            break;
        case VAL_FLOAT:
            snprintf(tmp, sizeof(tmp), "%g", v->float_val);
            buf_append(b, tmp);
            break;
        case VAL_STRING:
            buf_append(b, "\"");
            for (const char* p = v->string_val; *p; p++) {
                switch (*p) {
                    case '"': buf_append(b, "\\\""); break;
                    case '\\': buf_append(b, "\\\\"); break;
                    case '\n': buf_append(b, "\\n"); break;
                    case '\r': buf_append(b, "\\r"); break;
                    case '\t': buf_append(b, "\\t"); break;
                    default: buf_appendn(b, p, 1);
                }
            }
            buf_append(b, "\"");
            break;
        case VAL_ARRAY: {
            Array* arr = v->array_val;
            if (arr->count == 0) { buf_append(b, "[]"); break; }
            buf_append(b, "[\n");
            for (size_t i = 0; i < arr->count; i++) {
                format_json_indent(b, (depth + 1) * indent);
                format_json_value(b, &arr->items[i], indent, depth + 1, max_depth);
                if (i + 1 < arr->count) buf_append(b, ",");
                buf_append(b, "\n");
            }
            format_json_indent(b, depth * indent);
            buf_append(b, "]");
            break;
        }
        case VAL_STRUCT_INSTANCE: {
            StructInstance* si = v->struct_instance;
            /* count fields */
            int nfields = 0;
            FieldValue* f = si->fields;
            while (f) { nfields++; f = f->next; }
            if (nfields == 0) { buf_append(b, "{}"); break; }
            buf_append(b, "{\n");
            f = si->fields;
            int idx = 0;
            while (f) {
                format_json_indent(b, (depth + 1) * indent);
                buf_append(b, "\"");
                buf_append(b, f->name);
                buf_append(b, "\": ");
                format_json_value(b, f->value, indent, depth + 1, max_depth);
                if (idx + 1 < nfields) buf_append(b, ",");
                buf_append(b, "\n");
                f = f->next;
                idx++;
            }
            format_json_indent(b, depth * indent);
            buf_append(b, "}");
            break;
        }
        default:
            buf_append(b, "null");
    }
}

static Value pprint_json(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) {
        fprintf(stderr, "Error: pprint.json() requires at least one argument\n");
        return make_null();
    }
    int indent = 2;
    if (argc >= 2 && args[1].type == VAL_INT) indent = (int)args[1].int_val;
    if (indent < 1) indent = 2;

    Buf b;
    buf_init(&b);
    format_json_value(&b, &args[0], indent, 0, 20);

    Value result;
    result.type = VAL_STRING;
    result.string_val = b.data;
    return result;
}

void stdlib_pprint_register(void) {
    register_native("pprint.format", pprint_format);
    register_native("pprint.print", pprint_print);
    register_native("pprint.table", pprint_table);
    register_native("pprint.json", pprint_json);
}
