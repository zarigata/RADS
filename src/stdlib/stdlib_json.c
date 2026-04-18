#define _POSIX_C_SOURCE 200809L
#include "stdlib_json.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ═══════════════════════════════════════════
   Recursive-descent JSON parser
   ═══════════════════════════════════════════ */

typedef struct {
    const char* src;
    size_t pos;
    size_t len;
} JsonParser;

static void jp_skip_ws(JsonParser* p) {
    while (p->pos < p->len) {
        char c = p->src[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') p->pos++;
        else break;
    }
}

static char jp_peek(JsonParser* p) {
    jp_skip_ws(p);
    return (p->pos < p->len) ? p->src[p->pos] : '\0';
}

static char jp_next(JsonParser* p) {
    jp_skip_ws(p);
    return (p->pos < p->len) ? p->src[p->pos++] : '\0';
}

/* Forward declaration */
static Value jp_parse_value(JsonParser* p);

static Value jp_parse_string(JsonParser* p) {
    if (jp_next(p) != '"') return make_null();

    size_t cap = 256;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) return make_null();

    while (p->pos < p->len) {
        char c = p->src[p->pos++];
        if (c == '"') {
            buf[len] = '\0';
            Value v; v.type = VAL_STRING; v.string_val = buf;
            return v;
        }
        if (c == '\\') {
            if (p->pos >= p->len) break;
            char e = p->src[p->pos++];
            switch (e) {
                case '"':  c = '"';  break;
                case '\\': c = '\\'; break;
                case '/':  c = '/';  break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case 'u': {
                    /* Parse 4 hex digits, output as UTF-8 bytes */
                    unsigned int cp = 0;
                    for (int i = 0; i < 4 && p->pos < p->len; i++) {
                        char h = p->src[p->pos++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= (h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                    }
                    /* Surrogate pair handling */
                    if (cp >= 0xD800 && cp <= 0xDBFF && p->pos + 5 < p->len &&
                        p->src[p->pos] == '\\' && p->src[p->pos+1] == 'u') {
                        p->pos += 2;
                        unsigned int cp2 = 0;
                        for (int i = 0; i < 4 && p->pos < p->len; i++) {
                            char h = p->src[p->pos++];
                            cp2 <<= 4;
                            if (h >= '0' && h <= '9') cp2 |= (h - '0');
                            else if (h >= 'a' && h <= 'f') cp2 |= (h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp2 |= (h - 'A' + 10);
                        }
                        if (cp2 >= 0xDC00 && cp2 <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
                        }
                    }
                    /* Encode as UTF-8 */
                    if (cp < 0x80) {
                        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                        buf[len++] = (char)cp;
                    } else if (cp < 0x800) {
                        if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                        buf[len++] = (char)(0xC0 | (cp >> 6));
                        buf[len++] = (char)(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        if (len + 3 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                        buf[len++] = (char)(0xE0 | (cp >> 12));
                        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        buf[len++] = (char)(0x80 | (cp & 0x3F));
                    } else {
                        if (len + 4 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                        buf[len++] = (char)(0xF0 | (cp >> 18));
                        buf[len++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        buf[len++] = (char)(0x80 | (cp & 0x3F));
                    }
                    continue; /* skip the normal append below */
                }
                default: c = e; break;
            }
        }
        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[len++] = c;
    }
    free(buf);
    return make_null();
}

static Value jp_parse_number(JsonParser* p) {
    jp_skip_ws(p);
    size_t start = p->pos;
    int is_float = 0;

    if (p->pos < p->len && p->src[p->pos] == '-') p->pos++;
    while (p->pos < p->len && p->src[p->pos] >= '0' && p->src[p->pos] <= '9') p->pos++;
    if (p->pos < p->len && p->src[p->pos] == '.') {
        is_float = 1;
        p->pos++;
        while (p->pos < p->len && p->src[p->pos] >= '0' && p->src[p->pos] <= '9') p->pos++;
    }
    if (p->pos < p->len && (p->src[p->pos] == 'e' || p->src[p->pos] == 'E')) {
        is_float = 1;
        p->pos++;
        if (p->pos < p->len && (p->src[p->pos] == '+' || p->src[p->pos] == '-')) p->pos++;
        while (p->pos < p->len && p->src[p->pos] >= '0' && p->src[p->pos] <= '9') p->pos++;
    }

    size_t nlen = p->pos - start;
    char* tmp = malloc(nlen + 1);
    if (!tmp) return make_null();
    memcpy(tmp, p->src + start, nlen);
    tmp[nlen] = '\0';

    Value v;
    if (is_float) {
        v.type = VAL_FLOAT;
        v.float_val = strtod(tmp, NULL);
    } else {
        v.type = VAL_INT;
        v.int_val = strtoll(tmp, NULL, 10);
    }
    free(tmp);
    return v;
}

/* Parse a RADS struct instance from a JSON object */
static Value jp_parse_object(JsonParser* p) {
    if (jp_next(p) != '{') return make_null();

    /* We store as an array of alternating [key1, val1, key2, val2, ...] */
    Array* entries = array_create(8);

    if (jp_peek(p) == '}') {
        p->pos++;
        /* Return as array of pairs */
        Value v; v.type = VAL_ARRAY; v.array_val = entries;
        return v;
    }

    while (1) {
        jp_skip_ws(p);
        Value key = jp_parse_string(p);
        if (key.type != VAL_STRING) { break; }
        array_push(entries, key);

        jp_skip_ws(p);
        if (jp_peek(p) != ':') break;
        p->pos++; /* skip : */

        Value val = jp_parse_value(p);
        array_push(entries, val);

        jp_skip_ws(p);
        if (jp_peek(p) == ',') { p->pos++; continue; }
        if (jp_peek(p) == '}') { p->pos++; break; }
        break;
    }

    Value v; v.type = VAL_ARRAY; v.array_val = entries;
    return v;
}

static Value jp_parse_array(JsonParser* p) {
    if (jp_next(p) != '[') return make_null();

    Array* arr = array_create(8);

    if (jp_peek(p) == ']') {
        p->pos++;
        Value v; v.type = VAL_ARRAY; v.array_val = arr;
        return v;
    }

    while (1) {
        Value val = jp_parse_value(p);
        array_push(arr, val);

        jp_skip_ws(p);
        if (jp_peek(p) == ',') { p->pos++; continue; }
        if (jp_peek(p) == ']') { p->pos++; break; }
        break;
    }

    Value v; v.type = VAL_ARRAY; v.array_val = arr;
    return v;
}

static Value jp_parse_value(JsonParser* p) {
    jp_skip_ws(p);
    if (p->pos >= p->len) return make_null();

    char c = p->src[p->pos];

    if (c == '"') return jp_parse_string(p);
    if (c == '{') return jp_parse_object(p);
    if (c == '[') return jp_parse_array(p);

    /* true / false / null */
    if (strncmp(p->src + p->pos, "true", 4) == 0) {
        p->pos += 4;
        return make_bool(true);
    }
    if (strncmp(p->src + p->pos, "false", 5) == 0) {
        p->pos += 5;
        return make_bool(false);
    }
    if (strncmp(p->src + p->pos, "null", 4) == 0) {
        p->pos += 4;
        return make_null();
    }

    /* Number */
    if (c == '-' || (c >= '0' && c <= '9')) {
        return jp_parse_number(p);
    }

    return make_null();
}

/* ═══════════════════════════════════════════
   JSON serializer (Value → JSON string)
   ═══════════════════════════════════════════ */

typedef struct {
    char* buf;
    size_t len;
    size_t cap;
} JsonWriter;

static void jw_init(JsonWriter* w) {
    w->cap = 256;
    w->len = 0;
    w->buf = malloc(w->cap);
    if (w->buf) w->buf[0] = '\0';
}

static void jw_ensure(JsonWriter* w, size_t extra) {
    while (w->len + extra + 1 >= w->cap) {
        w->cap *= 2;
        w->buf = realloc(w->buf, w->cap);
    }
}

static void jw_putc(JsonWriter* w, char c) {
    jw_ensure(w, 1);
    w->buf[w->len++] = c;
}

static void jw_puts(JsonWriter* w, const char* s) {
    size_t slen = strlen(s);
    jw_ensure(w, slen);
    memcpy(w->buf + w->len, s, slen);
    w->len += slen;
}

static void jw_write_string(JsonWriter* w, const char* s) {
    jw_putc(w, '"');
    for (const char* p = s; *p; p++) {
        switch (*p) {
            case '"':  jw_puts(w, "\\\""); break;
            case '\\': jw_puts(w, "\\\\"); break;
            case '\b': jw_puts(w, "\\b");  break;
            case '\f': jw_puts(w, "\\f");  break;
            case '\n': jw_puts(w, "\\n");  break;
            case '\r': jw_puts(w, "\\r");  break;
            case '\t': jw_puts(w, "\\t");  break;
            default:
                if ((unsigned char)*p < 0x20) {
                    char ubuf[8];
                    snprintf(ubuf, sizeof(ubuf), "\\u%04x", (unsigned char)*p);
                    jw_puts(w, ubuf);
                } else {
                    jw_putc(w, *p);
                }
                break;
        }
    }
    jw_putc(w, '"');
}

/* Forward declaration */
static void jw_write_value(JsonWriter* w, Value* v, int indent, int pretty);

static void jw_write_indent(JsonWriter* w, int indent) {
    for (int i = 0; i < indent; i++) jw_putc(w, ' ');
}

static void jw_write_value(JsonWriter* w, Value* v, int indent, int pretty) {
    switch (v->type) {
        case VAL_NULL:
            jw_puts(w, "null");
            break;
        case VAL_BOOL:
            jw_puts(w, v->bool_val ? "true" : "false");
            break;
        case VAL_INT: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%lld", v->int_val);
            jw_puts(w, buf);
            break;
        }
        case VAL_FLOAT: {
            char buf[64];
            /* Use shortest representation that round-trips */
            snprintf(buf, sizeof(buf), "%.17g", v->float_val);
            /* Ensure it looks like a float (has . or e) */
            if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E')) {
                strcat(buf, ".0");
            }
            jw_puts(w, buf);
            break;
        }
        case VAL_STRING:
            jw_write_string(w, v->string_val);
            break;
        case VAL_ARRAY: {
            Array* arr = v->array_val;
            if (!arr || arr->count == 0) {
                jw_puts(w, "[]");
                break;
            }
            /* Heuristic: if first element is a string with even count, might be an object */
            /* We check if this looks like key-value pairs */
            int is_object = 0;
            if (arr->count >= 2 && arr->count % 2 == 0 && arr->items[0].type == VAL_STRING) {
                is_object = 1;
                /* Verify: even-indexed elements should be strings (keys) */
                for (size_t i = 0; i < arr->count; i += 2) {
                    if (arr->items[i].type != VAL_STRING) { is_object = 0; break; }
                }
            }

            if (is_object) {
                jw_putc(w, '{');
                if (pretty) jw_putc(w, '\n');
                for (size_t i = 0; i < arr->count; i += 2) {
                    if (i > 0) {
                        jw_putc(w, ',');
                        if (pretty) jw_putc(w, '\n');
                    }
                    if (pretty) jw_write_indent(w, indent + 2);
                    jw_write_string(w, arr->items[i].string_val);
                    jw_putc(w, ':');
                    if (pretty) jw_putc(w, ' ');
                    jw_write_value(w, &arr->items[i + 1], indent + 2, pretty);
                }
                if (pretty) { jw_putc(w, '\n'); jw_write_indent(w, indent); }
                jw_putc(w, '}');
            } else {
                jw_putc(w, '[');
                if (pretty) jw_putc(w, '\n');
                for (size_t i = 0; i < arr->count; i++) {
                    if (i > 0) {
                        jw_putc(w, ',');
                        if (pretty) jw_putc(w, '\n');
                    }
                    if (pretty) jw_write_indent(w, indent + 2);
                    jw_write_value(w, &arr->items[i], indent + 2, pretty);
                }
                if (pretty) { jw_putc(w, '\n'); jw_write_indent(w, indent); }
                jw_putc(w, ']');
            }
            break;
        }
        case VAL_FUNCTION:
            jw_puts(w, "null");
            break;
        case VAL_STRUCT_INSTANCE: {
            /* Serialize struct fields as object */
            StructInstance* si = v->struct_instance;
            jw_putc(w, '{');
            if (pretty) jw_putc(w, '\n');
            FieldValue* f = si->fields;
            int first = 1;
            while (f) {
                if (!first) {
                    jw_putc(w, ',');
                    if (pretty) jw_putc(w, '\n');
                }
                first = 0;
                if (pretty) jw_write_indent(w, indent + 2);
                jw_write_string(w, f->name);
                jw_putc(w, ':');
                if (pretty) jw_putc(w, ' ');
                jw_write_value(w, f->value, indent + 2, pretty);
                f = f->next;
            }
            if (pretty) { jw_putc(w, '\n'); jw_write_indent(w, indent); }
            jw_putc(w, '}');
            break;
        }
        default:
            jw_puts(w, "null");
            break;
    }
}

/* ═══════════════════════════════════════════
   Public native functions
   ═══════════════════════════════════════════ */

/* json.parse(json_string) → Value */
Value native_json_parse(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    JsonParser p;
    p.src = args[0].string_val;
    p.len = strlen(p.src);
    p.pos = 0;

    Value result = jp_parse_value(&p);
    return result;
}

/* json.stringify(value) → compact JSON string */
Value native_json_stringify(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("null");

    JsonWriter w;
    jw_init(&w);
    if (!w.buf) return make_null();

    jw_write_value(&w, &args[0], 0, 0);
    w.buf[w.len] = '\0';

    Value v; v.type = VAL_STRING; v.string_val = w.buf;
    return v;
}

/* json.stringify_pretty(value) → indented JSON string */
Value native_json_stringify_pretty(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("null");

    JsonWriter w;
    jw_init(&w);
    if (!w.buf) return make_null();

    jw_write_value(&w, &args[0], 0, 1);
    w.buf[w.len] = '\0';

    Value v; v.type = VAL_STRING; v.string_val = w.buf;
    return v;
}

/* ── Legacy helpers (kept for backward compat) ── */

static void trim_spaces(const char** p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

Value native_json_get_string(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_null();
    const char* json = args[0].string_val;
    const char* key = args[1].string_val;
    if (!json || !key) return make_null();
    size_t keylen = strlen(key);
    const char* p = json;
    while ((p = strstr(p, key)) != NULL) {
        if (p > json && p[-1] != '"') { p += keylen; continue; }
        const char* after = p + keylen;
        if (*after != '"') { p += keylen; continue; }
        const char* colon = strchr(after, ':');
        if (!colon) return make_null();
        colon++;
        trim_spaces(&colon);
        if (*colon == '"') {
            colon++;
            const char* end = strchr(colon, '"');
            if (!end) return make_null();
            size_t len = (size_t)(end - colon);
            char* out = malloc(len + 1);
            if (!out) return make_null();
            memcpy(out, colon, len);
            out[len] = '\0';
            Value v; v.type = VAL_STRING; v.string_val = out; return v;
        }
        return make_null();
    }
    return make_null();
}

Value native_json_get_number(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_null();
    const char* json = args[0].string_val;
    const char* key = args[1].string_val;
    size_t keylen = strlen(key);
    const char* p = json;
    while ((p = strstr(p, key)) != NULL) {
        if (p > json && p[-1] != '"') { p += keylen; continue; }
        const char* after = p + keylen;
        if (*after != '"') { p += keylen; continue; }
        const char* colon = strchr(after, ':');
        if (!colon) return make_null();
        colon++;
        trim_spaces(&colon);
        char* endptr = NULL;
        double val = strtod(colon, &endptr);
        if (endptr == colon) return make_null();
        Value v;
        if (val == (long long)val && !strchr(colon, '.')) {
            v.type = VAL_INT; v.int_val = (long long)val;
        } else {
            v.type = VAL_FLOAT; v.float_val = val;
        }
        return v;
    }
    return make_null();
}

Value native_json_get_bool(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_null();
    const char* json = args[0].string_val;
    const char* key = args[1].string_val;
    size_t keylen = strlen(key);
    const char* p = json;
    while ((p = strstr(p, key)) != NULL) {
        if (p > json && p[-1] != '"') { p += keylen; continue; }
        const char* after = p + keylen;
        if (*after != '"') { p += keylen; continue; }
        const char* colon = strchr(after, ':');
        if (!colon) return make_null();
        colon++;
        trim_spaces(&colon);
        if (strncmp(colon, "true", 4) == 0) { Value v; v.type = VAL_BOOL; v.bool_val = true; return v; }
        if (strncmp(colon, "false", 5) == 0) { Value v; v.type = VAL_BOOL; v.bool_val = false; return v; }
        return make_null();
    }
    return make_null();
}

Value native_json_stringify_kv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return make_null();
    JsonWriter w; jw_init(&w);
    if (!w.buf) return make_null();
    jw_putc(&w, '{');
    jw_write_string(&w, args[0].string_val);
    jw_putc(&w, ':');
    jw_write_string(&w, args[1].string_val);
    jw_putc(&w, '}');
    w.buf[w.len] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = w.buf; return v;
}

Value native_json_escape(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    JsonWriter w; jw_init(&w);
    if (!w.buf) return make_null();
    jw_write_string(&w, args[0].string_val);
    /* Strip surrounding quotes */
    if (w.len >= 2) {
        memmove(w.buf, w.buf + 1, w.len - 2);
        w.len -= 2;
    }
    w.buf[w.len] = '\0';
    Value v; v.type = VAL_STRING; v.string_val = w.buf; return v;
}

/* json.get(path) - get nested value by dot-path from parsed JSON */
Value native_json_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) return make_null();

    Value current = args[0];
    const char* path = args[1].string_val;

    char* path_copy = strdup(path);
    char* token = strtok(path_copy, ".");

    while (token) {
        if (current.type == VAL_ARRAY) {
            /* Check if this is an object (even-count, string keys) or array */
            Array* arr = current.array_val;
            int is_obj = (arr->count >= 2 && arr->count % 2 == 0 && arr->items[0].type == VAL_STRING);
            if (is_obj) {
                /* Look up key */
                int found = 0;
                for (size_t i = 0; i < arr->count; i += 2) {
                    if (arr->items[i].type == VAL_STRING &&
                        strcmp(arr->items[i].string_val, token) == 0) {
                        current = arr->items[i + 1];
                        found = 1;
                        break;
                    }
                }
                if (!found) { free(path_copy); return make_null(); }
            } else {
                /* Array index */
                char* end;
                long idx = strtol(token, &end, 10);
                if (*end != '\0' || idx < 0 || (size_t)idx >= arr->count) {
                    free(path_copy);
                    return make_null();
                }
                current = arr->items[(size_t)idx];
            }
        } else {
            free(path_copy);
            return make_null();
        }
        token = strtok(NULL, ".");
    }

    free(path_copy);
    return current;
}

/* json.keys(parsed_object) → array of key strings */
Value native_json_keys(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    Array* keys = array_create(arr->count / 2 + 1);
    for (size_t i = 0; i < arr->count; i += 2) {
        if (arr->items[i].type == VAL_STRING) {
            array_push(keys, arr->items[i]);
        }
    }
    Value v; v.type = VAL_ARRAY; v.array_val = keys;
    return v;
}

/* json.values(parsed_object) → array of values */
Value native_json_values(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    Array* vals = array_create(arr->count / 2 + 1);
    for (size_t i = 1; i < arr->count; i += 2) {
        array_push(vals, arr->items[i]);
    }
    Value v; v.type = VAL_ARRAY; v.array_val = vals;
    return v;
}

/* json.has_key(parsed_object, key) → bool */
Value native_json_has_key(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_STRING) {
        Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
    }
    Array* arr = args[0].array_val;
    const char* key = args[1].string_val;
    for (size_t i = 0; i + 1 < arr->count; i += 2) {
        if (arr->items[i].type == VAL_STRING && strcmp(arr->items[i].string_val, key) == 0) {
            Value v; v.type = VAL_BOOL; v.bool_val = true; return v;
        }
    }
    Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
}

/* json.type_of(value) → string describing type */
Value native_json_type_of(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_string("null");
    switch (args[0].type) {
        case VAL_NULL:            return make_string("null");
        case VAL_BOOL:            return make_string("bool");
        case VAL_INT:             return make_string("int");
        case VAL_FLOAT:           return make_string("float");
        case VAL_STRING:          return make_string("string");
        case VAL_FUNCTION:        return make_string("function");
        case VAL_ARRAY:           return make_string("array");
        case VAL_STRUCT_DEF:      return make_string("struct_def");
        case VAL_STRUCT_INSTANCE: return make_string("struct");
        default:                  return make_string("unknown");
    }
}

void stdlib_json_register(void) {
    register_native("json.parse",           native_json_parse);
    register_native("json.stringify",       native_json_stringify);
    register_native("json.stringify_pretty",native_json_stringify_pretty);
    register_native("json.get",             native_json_get);
    register_native("json.keys",            native_json_keys);
    register_native("json.values",          native_json_values);
    register_native("json.has_key",         native_json_has_key);
    register_native("json.type_of",         native_json_type_of);
    /* Legacy */
    register_native("json.get_string",      native_json_get_string);
    register_native("json.get_number",      native_json_get_number);
    register_native("json.get_bool",        native_json_get_bool);
    register_native("json.stringify_kv",    native_json_stringify_kv);
    register_native("json.escape",          native_json_escape);
}
