#include "stdlib_textwrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);
extern void register_native(const char* name, NativeFn fn);

// ── helpers ──

/* Duplicate exactly len chars starting at s */
static char* str_ndup(const char* s, size_t len) {
    char* d = malloc(len + 1);
    memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

/* Count leading spaces (0 or positive) */
static int leading_spaces(const char* s) {
    int n = 0;
    while (*s == ' ') { n++; s++; }
    return n;
}

/* Strip trailing whitespace in-place */
static void rtrim(char* s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

/* Return newly allocated string with common leading whitespace removed from every line */
static Value textwrap_dedup_impl(const char* text) {
    /* First pass: find minimum indentation among non-empty lines */
    size_t text_len = strlen(text);
    int min_indent = -1;
    size_t i = 0;
    while (i < text_len) {
        /* skip blank lines for indent calculation */
        if (text[i] == '\n') { i++; continue; }
        int spaces = 0;
        while (i < text_len && text[i] == ' ') { spaces++; i++; }
        if (i < text_len && text[i] != '\n') {
            if (min_indent == -1 || spaces < min_indent) min_indent = spaces;
        }
        while (i < text_len && text[i] != '\n') i++;
        if (i < text_len) i++; /* skip \n */
    }
    if (min_indent < 0) min_indent = 0;

    /* Second pass: build output stripping min_indent leading spaces */
    /* Output size <= input size */
    char* out = malloc(text_len + 1);
    size_t j = 0;
    i = 0;
    while (i < text_len) {
        if (text[i] == '\n') {
            out[j++] = '\n';
            i++;
            continue;
        }
        /* skip min_indent spaces */
        int skip = min_indent;
        while (skip > 0 && i < text_len && text[i] == ' ') { i++; skip--; }
        /* copy rest of line */
        while (i < text_len && text[i] != '\n') out[j++] = text[i++];
    }
    out[j] = '\0';
    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.wrap(text, width=70) ──
// Returns array of lines, each <= width characters
static Value textwrap_wrap(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: textwrap.wrap() requires a string argument\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int width = 70;
    if (argc >= 2 && args[1].type == VAL_INT) width = (int)args[1].int_val;
    if (width < 1) width = 1;

    Array* arr = array_create(16);
    size_t len = strlen(text);
    size_t pos = 0;

    while (pos < len) {
        /* skip leading whitespace */
        while (pos < len && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n' || text[pos] == '\r'))
            pos++;
        if (pos >= len) break;

        /* find end of line or width boundary */
        size_t last_space = 0;
        int has_space = 0;
        size_t end = pos;

        while (end < len && (end - pos) < (size_t)width && text[end] != '\n') {
            if (text[end] == ' ') {
                last_space = end;
                has_space = 1;
            }
            end++;
        }

        if (end < len && text[end] != '\n' && (end - pos) >= (size_t)width) {
            if (has_space) {
                end = last_space;
            }
        }

        /* skip trailing spaces on the line */
        size_t line_end = end;
        while (line_end > pos && text[line_end - 1] == ' ') line_end--;

        char* line = str_ndup(text + pos, line_end - pos);
        Value lv;
        lv.type = VAL_STRING;
        lv.string_val = line;
        array_push(arr, lv);

        pos = end;
        /* skip spaces after break point */
        while (pos < len && text[pos] == ' ') pos++;
        if (pos < len && text[pos] == '\n') pos++;
    }

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// ── textwrap.fill(text, width=70) ──
// Like wrap() but returns a single string with newlines
static Value textwrap_fill(Interpreter* interp, int argc, Value* args) {
    Value wrapped = textwrap_wrap(interp, argc, args);
    if (wrapped.type != VAL_ARRAY) return make_null();

    Array* arr = wrapped.array_val;
    if (arr->count == 0) {
        /* free array */
        Value result;
        result.type = VAL_STRING;
        result.string_val = strdup("");
        return result;
    }

    /* calculate total length */
    size_t total = 0;
    for (size_t i = 0; i < arr->count; i++) {
        total += strlen(arr->items[i].string_val);
        if (i > 0) total++; /* for \n */
    }

    char* out = malloc(total + 1);
    out[0] = '\0';
    for (size_t i = 0; i < arr->count; i++) {
        if (i > 0) strcat(out, "\n");
        strcat(out, arr->items[i].string_val);
        free(arr->items[i].string_val);
    }
    free(arr->items);
    free(arr);

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.indent(text, prefix) ──
// Prefix every non-empty line with the given prefix
static Value textwrap_indent(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: textwrap.indent(text, prefix) requires two string arguments\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    const char* prefix = args[1].string_val;
    size_t prefix_len = strlen(prefix);
    size_t text_len = strlen(text);

    /* worst case: every char becomes a line break + prefix */
    char* out = malloc(text_len + (text_len + 1) * prefix_len + 1);
    size_t j = 0;

    /* first line */
    if (text_len > 0) {
        memcpy(out + j, prefix, prefix_len); j += prefix_len;
    }
    for (size_t i = 0; i < text_len; i++) {
        out[j++] = text[i];
        if (text[i] == '\n' && i + 1 < text_len) {
            memcpy(out + j, prefix, prefix_len); j += prefix_len;
        }
    }
    out[j] = '\0';

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.dedent(text) ──
// Remove common leading whitespace from every line
static Value textwrap_dedent(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: textwrap.dedent() requires a string argument\n");
        return make_null();
    }
    return textwrap_dedup_impl(args[0].string_val);
}

// ── textwrap.shorten(text, width=70, placeholder="...") ──
// Collapse whitespace and truncate to width
static Value textwrap_shorten(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: textwrap.shorten() requires a string argument\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int width = 70;
    const char* placeholder = "...";
    if (argc >= 2 && args[1].type == VAL_INT) width = (int)args[1].int_val;
    if (argc >= 3 && args[2].type == VAL_STRING) placeholder = args[2].string_val;

    size_t len = strlen(text);
    /* collapse whitespace */
    char* collapsed = malloc(len + 1);
    size_t j = 0;
    int in_space = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r') {
            if (!in_space && j > 0) { collapsed[j++] = ' '; in_space = 1; }
        } else {
            collapsed[j++] = text[i];
            in_space = 0;
        }
    }
    collapsed[j] = '\0';

    size_t plen = strlen(placeholder);
    if (j <= (size_t)width) {
        Value result;
        result.type = VAL_STRING;
        result.string_val = collapsed;
        return result;
    }

    /* truncate and append placeholder */
    size_t keep = width > (int)plen ? width - plen : 0;
    if (keep > j) keep = j;
    /* don't break in middle of a word — back up to last space */
    while (keep > 0 && collapsed[keep] != ' ') keep--;
    if (keep == 0) keep = width > (int)plen ? width - plen : 0;

    char* out = malloc(keep + plen + 1);
    memcpy(out, collapsed, keep);
    memcpy(out + keep, placeholder, plen);
    out[keep + plen] = '\0';
    free(collapsed);

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.truncate(text, width, placeholder="...") ──
// Simple truncation without word awareness
static Value textwrap_truncate(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        fprintf(stderr, "Error: textwrap.truncate(text, width) requires string and int\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int width = (int)args[1].int_val;
    const char* placeholder = "...";
    if (argc >= 3 && args[2].type == VAL_STRING) placeholder = args[2].string_val;

    size_t len = strlen(text);
    size_t plen = strlen(placeholder);
    if (len <= (size_t)width) {
        return make_string(text);
    }
    size_t keep = width > (int)plen ? width - plen : 0;
    char* out = malloc(keep + plen + 1);
    memcpy(out, text, keep);
    memcpy(out + keep, placeholder, plen);
    out[keep + plen] = '\0';

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.center(text, width, fillchar=" ") ──
static Value textwrap_center(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        fprintf(stderr, "Error: textwrap.center(text, width) requires string and int\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int width = (int)args[1].int_val;
    char fill = ' ';
    if (argc >= 3 && args[2].type == VAL_STRING && strlen(args[2].string_val) > 0)
        fill = args[2].string_val[0];

    size_t len = strlen(text);
    if ((int)len >= width) return make_string(text);

    int total_pad = width - (int)len;
    int left = total_pad / 2;
    int right = total_pad - left;

    char* out = malloc(width + 1);
    memset(out, fill, left);
    memcpy(out + left, text, len);
    memset(out + left + len, fill, right);
    out[width] = '\0';

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.ljust(text, width, fillchar=" ") ──
static Value textwrap_ljust(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        fprintf(stderr, "Error: textwrap.ljust(text, width) requires string and int\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int width = (int)args[1].int_val;
    char fill = ' ';
    if (argc >= 3 && args[2].type == VAL_STRING && strlen(args[2].string_val) > 0)
        fill = args[2].string_val[0];

    size_t len = strlen(text);
    if ((int)len >= width) return make_string(text);

    char* out = malloc(width + 1);
    memcpy(out, text, len);
    memset(out + len, fill, width - len);
    out[width] = '\0';

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.rjust(text, width, fillchar=" ") ──
static Value textwrap_rjust(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        fprintf(stderr, "Error: textwrap.rjust(text, width) requires string and int\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int width = (int)args[1].int_val;
    char fill = ' ';
    if (argc >= 3 && args[2].type == VAL_STRING && strlen(args[2].string_val) > 0)
        fill = args[2].string_val[0];

    size_t len = strlen(text);
    if ((int)len >= width) return make_string(text);

    int pad = width - (int)len;
    char* out = malloc(width + 1);
    memset(out, fill, pad);
    memcpy(out + pad, text, len);
    out[width] = '\0';

    Value result;
    result.type = VAL_STRING;
    result.string_val = out;
    return result;
}

// ── textwrap.word_count(text) ──
static Value textwrap_word_count(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: textwrap.word_count() requires a string argument\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    int count = 0;
    int in_word = 0;
    for (size_t i = 0; text[i]; i++) {
        if (isspace((unsigned char)text[i])) {
            in_word = 0;
        } else {
            if (!in_word) count++;
            in_word = 1;
        }
    }
    return make_int(count);
}

// ── textwrap.line_count(text) ──
static Value textwrap_line_count(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: textwrap.line_count() requires a string argument\n");
        return make_null();
    }
    const char* text = args[0].string_val;
    if (strlen(text) == 0) return make_int(0);
    int count = 1;
    for (size_t i = 0; text[i]; i++) {
        if (text[i] == '\n') count++;
    }
    return make_int(count);
}

// ── registration ──
void stdlib_textwrap_register(void) {
    register_native("textwrap.wrap", textwrap_wrap);
    register_native("textwrap.fill", textwrap_fill);
    register_native("textwrap.indent", textwrap_indent);
    register_native("textwrap.dedent", textwrap_dedent);
    register_native("textwrap.shorten", textwrap_shorten);
    register_native("textwrap.truncate", textwrap_truncate);
    register_native("textwrap.center", textwrap_center);
    register_native("textwrap.ljust", textwrap_ljust);
    register_native("textwrap.rjust", textwrap_rjust);
    register_native("textwrap.word_count", textwrap_word_count);
    register_native("textwrap.line_count", textwrap_line_count);
}
