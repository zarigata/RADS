/**
 * RADS Masquerading - Simple Pattern-Based Transpiler
 *
 * A pragmatic, pattern-matching based transpiler for Python → RADS
 * This is the Ralph Wiggum loop approach: simple, testable, iterative.
 *
 * Version: 0.0.5 "CHAMELEON"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 4096
#define MAX_INDENT 100

// ============================================================================
// Utility Functions
// ============================================================================

static int count_leading_spaces(const char* line) {
    int count = 0;
    while (line[count] == ' ' || line[count] == '\t') {
        if (line[count] == '\t') count += 4;  // Tab = 4 spaces
        else count++;
    }
    return count;
}

static char* trim(char* str) {
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}

static int starts_with(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/*
 * find_assignment() — locate a real Python assignment '=' in a statement.
 * Why: a bare '=' is an assignment, but '==', '!=', '<=', '>=', ':=' and
 * keyword-arguments ('foo(x=1)') must NOT be treated as one. The old code
 * checked the char BEFORE the first '=', which wrongly captured the second
 * '=' of 'a == b' and mangled it into 'turbo a = = b'.
 * How to extend: add more operator exceptions to the skip-conditions, or
 * teach the paren-depth check about brackets/braces for slice defaults.
 */
static char* find_assignment(const char* s) {
    int paren_depth = 0;
    for (const char* p = s; *p; p++) {
        if (*p == '(') paren_depth++;
        else if (*p == ')' && paren_depth > 0) paren_depth--;
        else if (*p == '=' && paren_depth == 0) {
            /* skip ==, !=, <=, >=, := ; guard p[-1] at string start (UB) */
            if (p[1] == '=' ||
                (p > s && (p[-1] == '=' || p[-1] == '!' ||
                           p[-1] == '<' || p[-1] == '>' || p[-1] == ':'))) {
                p++; /* skip the pair so '==' is not re-examined */
                continue;
            }
            return (char*)p;
        }
    }
    return NULL;
}


static void replace_in_line(char* line, const char* from, const char* to) {
    char buffer[MAX_LINE_LENGTH];
    char* pos = line;
    char* temp = buffer;
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);

    while (*pos) {
        if (strncmp(pos, from, from_len) == 0) {
            strcpy(temp, to);
            temp += to_len;
            pos += from_len;
        } else {
            *temp++ = *pos++;
        }
    }
    *temp = '\0';
    strcpy(line, buffer);
}

/*
 * replace_outside_strings() — like replace_in_line, but never touches text
 * inside '...' or "..." literals (run 22). Method names live next to string
 * args; blindly replacing ".upper()" would corrupt "le.upper()" inside a
 * string constant. Handles backslash escapes inside literals.
 */
static void replace_outside_strings(char* line, const char* from, const char* to) {
    char buffer[MAX_LINE_LENGTH * 2];
    char* pos = line;
    char* temp = buffer;
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);
    int in_str = 0;
    char quote = 0;

    while (*pos) {
        if (in_str) {
            if (*pos == '\\' && pos[1]) {
                *temp++ = *pos++; *temp++ = *pos++;
                continue;
            }
            if (*pos == quote) in_str = 0;
            *temp++ = *pos++;
        } else if (*pos == '"' || *pos == '\'') {
            in_str = 1; quote = *pos;
            *temp++ = *pos++;
        } else if (strncmp(pos, from, from_len) == 0) {
            memcpy(temp, to, to_len);
            temp += to_len;
            pos += from_len;
        } else {
            *temp++ = *pos++;
        }
    }
    *temp = '\0';
    strcpy(line, buffer);
}

/*
 * replace_word_outside_strings() — replace_outside_strings plus identifier
 * boundaries: "True" must not match inside "TrueThing" or "myTrue".
 */
static void replace_word_outside_strings(char* line, const char* from, const char* to) {
    char buffer[MAX_LINE_LENGTH * 2];
    char* pos = line;
    char* temp = buffer;
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);
    int in_str = 0;
    char quote = 0;

    while (*pos) {
        if (in_str) {
            if (*pos == '\\' && pos[1]) { *temp++ = *pos++; *temp++ = *pos++; continue; }
            if (*pos == quote) in_str = 0;
            *temp++ = *pos++;
        } else if (*pos == '"' || *pos == '\'') {
            in_str = 1; quote = *pos;
            *temp++ = *pos++;
        } else if (strncmp(pos, from, from_len) == 0) {
            char prev = (pos > line) ? pos[-1] : '\0';
            char next = pos[from_len];
            int prev_ok = prev == '\0' || !(isalnum((unsigned char)prev) || prev == '_');
            int next_ok = next == '\0' || !(isalnum((unsigned char)next) || next == '_');
            if (prev_ok && next_ok) {
                memcpy(temp, to, to_len);
                temp += to_len;
                pos += from_len;
            } else {
                *temp++ = *pos++;
            }
        } else {
            *temp++ = *pos++;
        }
    }
    *temp = '\0';
    strcpy(line, buffer);
}

/*
 * expand_fstring() — turn ONE Python f-string literal `f"...{expr}..."` into
 * a RADS concatenation expression (run 22):
 *   {expr}      → str(expr)          (Python str() semantics)
 *   {{ / }}     → literal { / }
 *   {e:.2f}     → pyfmt.fixed(e, 2)
 *   {e:>10}     → str(e).rjust(10)   {e:<10} → .ljust(10)   {e:^7} → .center(7)
 *   {e:5d}      → str(e).rjust(5)
 * Literal chunks are re-emitted as double-quoted RADS strings with `"`
 * escaped. Returns 1 on success, 0 to keep the line verbatim.
 */
static int expand_fstring(const char* src, char* out, size_t outsz) {
    if (!(src[0] == 'f' && (src[1] == '"' || src[1] == '\''))) return 0;

    const char* p = src + 2;
    char endq = src[1];
    const char* close = strrchr(p, endq);
    if (!close || close == p) return 0;

    char expr[512];
    char lit[1024];
    size_t llen = 0;
    size_t olen = 0;
    int parts = 0;

    /* flush pending literal chunk as a quoted RADS string */
    #define FLUSH_LIT() do { \
        if (llen > 0) { \
            if (parts > 0) { memcpy(out + olen, " + ", 3); olen += 3; } \
            if (olen + llen + 4 >= outsz) return 0; \
            out[olen++] = '"'; \
            memcpy(out + olen, lit, llen); olen += llen; \
            out[olen++] = '"'; out[olen] = '\0'; \
            parts++; llen = 0; \
        } \
    } while (0)

    while (p < close) {
        if (*p == '{' && p[1] == '{') { if (llen + 1 < sizeof lit) lit[llen++] = '{'; p += 2; continue; }
        if (*p == '}' && p[1] == '}') { if (llen + 1 < sizeof lit) lit[llen++] = '}'; p += 2; continue; }
        if (*p == '{') {
            const char* e = p + 1;
            int depth = 1;
            while (*e && depth > 0) {
                if (*e == '{') depth++;
                else if (*e == '}') { depth--; if (depth == 0) break; }
                e++;
            }
            if (depth != 0) return 0; /* unbalanced: keep verbatim */
            size_t elen = (size_t)(e - (p + 1));
            if (elen == 0 || elen >= sizeof expr) return 0;

            memcpy(expr, p + 1, elen);
            expr[elen] = '\0';

            /* format spec: split at the LAST ':' — expressions may contain
             * slice colons, but a spec colon is always the final segment */
            char* spec = strrchr(expr, ':');
            if (spec) *spec++ = '\0';

            char* es = expr;
            while (*es == ' ' || *es == '\t') es++;
            char* ee = es + strlen(es) - 1;
            while (ee > es && (*ee == ' ' || *ee == '\t')) *ee-- = '\0';
            if (*es == '\0') return 0;

            /* parse spec: [align][width][.precision][type] */
            char align = 0;
            const char* sp = spec;
            if (sp && (*sp == '<' || *sp == '>' || *sp == '^')) { align = *sp++; }
            int width = 0, precision = -1;
            const char* dot = sp ? strchr(sp, '.') : NULL;
            if (dot) { precision = atoi(dot + 1); }
            if (sp) { width = atoi(sp); }
            if (width < 0) width = 0;
            if (precision > 60) precision = 60;

            FLUSH_LIT();
            if (parts > 0) { if (olen + 4 >= outsz) return 0; memcpy(out + olen, " + ", 3); olen += 3; }

            /* base conversion */
            if (precision >= 0) {
                olen += (size_t)snprintf(out + olen, outsz - olen,
                                         "pyfmt.fixed(%s, %d)", es, precision);
            } else {
                olen += (size_t)snprintf(out + olen, outsz - olen, "str(%s)", es);
            }
            /* width padding */
            if (width > 0) {
                char fn = (align == '<') ? 'l' : (align == '^') ? 'c' : 'r';
                const char* fname = (fn == 'l') ? "ljust" : (fn == 'c') ? "center" : "rjust";
                olen += (size_t)snprintf(out + olen, outsz - olen, ".%s(%d)", fname, width);
            }
            if (olen >= outsz - 1) return 0;
            parts++;
            p = e + 1;
            continue;
        }
        /* literal char; escape a bare '"' into the double-quoted RADS chunk */
        if (*p == '"' && !(llen > 0 && lit[llen - 1] == '\\')) {
            if (llen + 2 < sizeof lit) { lit[llen++] = '\\'; lit[llen++] = '"'; }
        } else if (llen + 1 < sizeof lit) {
            lit[llen++] = *p;
        }
        p++;
    }
    FLUSH_LIT();
    #undef FLUSH_LIT
    if (parts == 0) return 0; /* plain string, no substitutions */
    out[olen] = '\0';
    return 1;
}

/*
 * expand_fstrings_in_line() — find every f-string literal in the statement
 * (outside other string literals) and replace it with the expanded
 * parenthesized concatenation. Runs before all statement rules.
 */
static void expand_fstrings_in_line(char* line) {
    char out[MAX_LINE_LENGTH * 2];
    size_t r = 0, w = 0;
    int in_str = 0;
    char quote = 0;

    while (line[r]) {
        if (in_str) {
            if (line[r] == '\\' && line[r + 1]) { out[w++] = line[r++]; out[w++] = line[r++]; continue; }
            if (line[r] == quote) in_str = 0;
            out[w++] = line[r++];
            continue;
        }
        if (line[r] == '"' || line[r] == '\'') {
            in_str = 1; quote = line[r];
            out[w++] = line[r++];
            continue;
        }
        /* 'f' immediately followed by a quote = f-string start */
        if (line[r] == 'f' && (line[r + 1] == '"' || line[r + 1] == '\'')) {
            /* find the closing quote of this f-string (respect escapes) */
            char q = line[r + 1];
            size_t e = r + 2;
            while (line[e]) {
                if (line[e] == '\\' && line[e + 1]) { e += 2; continue; }
                if (line[e] == q) break;
                e++;
            }
            if (!line[e]) { out[w++] = line[r++]; continue; } /* unterminated: leave */
            char expanded[MAX_LINE_LENGTH * 2];
            /* temporarily NUL-terminate the slice for expand_fstring */
            char saved = line[e + 1];
            line[e + 1] = '\0';
            int ok = expand_fstring(line + r, expanded, sizeof expanded);
            line[e + 1] = saved;
            if (ok) {
                w += (size_t)snprintf(out + w, sizeof(out) - w, "(%s)", expanded);
                r = e + 1;
                continue;
            }
            /* expansion failed: pass the literal through untouched */
            while (r <= e) out[w++] = line[r++];
            continue;
        }
        out[w++] = line[r++];
    }
    out[w] = '\0';
    if (w < sizeof(line)) { /* fits: copy back */ }
    snprintf(line, MAX_LINE_LENGTH, "%s", out);
}

// ============================================================================
// Transpilation Rules
// ============================================================================

static void transpile_line(const char* input, char* output, int* indent_level) {
    char line[MAX_LINE_LENGTH];
    strncpy(line, input, MAX_LINE_LENGTH - 1);
    line[MAX_LINE_LENGTH - 1] = '\0';

    // Calculate indentation
    int leading_spaces = count_leading_spaces(line);
    int new_indent = leading_spaces / 4;

    // Skip to non-whitespace
    const char* trimmed = line + leading_spaces;

    // Skip empty lines, comments, and pass statements
    if (*trimmed == '\0' || *trimmed == '#' || strcmp(trimmed, "pass") == 0) {
        output[0] = '\0';
        return;
    }

    // Start building output with proper indentation
    output[0] = '\0';
    for (int i = 0; i < new_indent; i++) {
        strcat(output, "    ");
    }

    // Rule 0a (run 22): expand f-string literals IN PLACE before `converted`
    // is copied from this same buffer — rules below must see the expansion.
    expand_fstrings_in_line(line);

    char converted[MAX_LINE_LENGTH];
    strcpy(converted, trimmed);

    // Rule 0b (run 22): Python literal names → RADS, string-safe + word-boundary.
    replace_word_outside_strings(converted, "True", "true");
    replace_word_outside_strings(converted, "False", "false");
    replace_word_outside_strings(converted, "None", "null");

    // Rule 1: def function_name(...): → blast function_name(...) {
    if (starts_with(converted, "def ")) {
        char* colon = strrchr(converted, ':');
        if (colon) {
            *colon = '\0';
            sprintf(output + strlen(output), "blast %s {", converted + 4);
            *indent_level = new_indent + 1;
            return;
        }
    }

    // Rule 2: Variable assignment with type inference
    // Python: x = 42  → RADS: turbo x = 42;
    // Uses find_assignment() so comparisons (==) and kwargs are not mangled.
    // 'return x = ...' can't happen in Python, but 'return x == y' must not
    // be captured here — Rule 8 runs first (moved below its check).
    if (!starts_with(converted, "return")) {
        char* equals = find_assignment(converted);
        if (equals) {
            char var_name[256];
            char* start = converted;
            while (isspace((unsigned char)*start)) start++;
            char* end = equals - 1;
            while (end > start && isspace((unsigned char)*end)) end--;
            int len = (int)(end - start) + 1;
            if (len > 0 && len < (int)sizeof(var_name)) {
                strncpy(var_name, start, len);
                var_name[len] = '\0';

                char* value = equals + 1;
                while (isspace((unsigned char)*value)) value++;

                /* strip trailing newline carriage leftovers before formatting */
                trim(converted);

                // Always use turbo for variable declarations
                sprintf(output + strlen(output), "turbo %s = %s;", var_name, value);
                trim(output);
                return;
            }
        }
    }

    // Rule 3: if condition: → if (condition) {
    if (starts_with(converted, "if ")) {
        char* colon = strrchr(converted, ':');
        if (colon) {
            *colon = '\0';
            char condition[MAX_LINE_LENGTH];
            strcpy(condition, converted + 3);
            trim(condition);

            // Convert Python booleans to RADS
            replace_in_line(condition, "True", "true");
            replace_in_line(condition, "False", "false");
            replace_in_line(condition, "None", "null");
            replace_in_line(condition, " and ", " && ");
            replace_in_line(condition, " or ", " || ");
            replace_in_line(condition, " not ", " ! ");

            sprintf(output + strlen(output), "if (%s) {", condition);
            *indent_level = new_indent + 1;
            return;
        }
    }

    // Rule 4: elif condition: → } elif (condition) {
    if (starts_with(converted, "elif ")) {
        char* colon = strrchr(converted, ':');
        if (colon) {
            *colon = '\0';
            char condition[MAX_LINE_LENGTH];
            strcpy(condition, converted + 5);
            trim(condition);

            // Convert Python booleans to RADS
            replace_in_line(condition, "True", "true");
            replace_in_line(condition, "False", "false");

            // Reduce indent for closing brace — align with the opening 'if'
            output[0] = '\0';
            for (int i = 0; i < new_indent; i++) {
                strcat(output, "    ");
            }
            sprintf(output + strlen(output), "} elif (%s) {", condition);
            *indent_level = new_indent;
            return;
        }
    }

    // Rule 5: else: → } else {
    if (starts_with(converted, "else:") || strcmp(converted, "else:") == 0) {
        output[0] = '\0';
        for (int i = 0; i < new_indent; i++) {
            strcat(output, "    ");
        }
        strcat(output, "} else {");
        *indent_level = new_indent + 1;
        return;
    }

    // Rule 6: while condition: → loop (condition) {
    if (starts_with(converted, "while ")) {
        char* colon = strrchr(converted, ':');
        if (colon) {
            *colon = '\0';
            char condition[MAX_LINE_LENGTH];
            strcpy(condition, converted + 6);
            trim(condition);

            replace_in_line(condition, "True", "true");
            replace_in_line(condition, "False", "false");

            sprintf(output + strlen(output), "loop (%s) {", condition);
            *indent_level = new_indent + 1;
            return;
        }
    }

    // Rule 7: for item in items: → cruise (items) { turbo item
    if (starts_with(converted, "for ")) {
        char* in_pos = strstr(converted, " in ");
        if (in_pos) {
            char* colon = strrchr(converted, ':');
            if (colon) {
                *colon = '\0';
                *in_pos = '\0';

                char var[256];
                strcpy(var, converted + 4);
                trim(var);

                char collection[MAX_LINE_LENGTH];
                strcpy(collection, in_pos + 4);
                trim(collection);

                /* Python range(...) → RADS native `lo..hi` half-open range.
                 * (array.range never existed — that mapping emitted a call
                 * to an undefined function.)
                 * Supported: range(n) → 0..n, range(a, b) → a..b.
                 * range(a, b, step) has no RADS equivalent yet → left as-is
                 * so the interpreter's "Undefined function 'range'" names it. */
                if (starts_with(collection, "range(")) {
                    char* rp = collection + 6;
                    char* rend = strrchr(rp, ')');
                    if (rend) {
                        *rend = '\0';
                        /* count top-level commas to detect the 3-arg form */
                        int commas = 0, depth = 0, in_str = 0;
                        for (char* q = rp; *q; q++) {
                            if (in_str) { if (*q == '"') in_str = 0; }
                            else if (*q == '"') in_str = 1;
                            else if (*q == '(' || *q == '[') depth++;
                            else if (*q == ')' || *q == ']') depth--;
                            else if (*q == ',' && depth == 0) commas++;
                        }
                        char range_buf[MAX_LINE_LENGTH]; // scratch: rp aliases collection
                        if (commas == 0) {
                            snprintf(range_buf, sizeof(range_buf), "0..%s", rp);
                            strcpy(collection, range_buf);
                        } else if (commas == 1) {
                            char* c1 = strchr(rp, ',');
                            *c1 = '\0';
                            char* lo = rp;
                            char* hi = c1 + 1;
                            while (*lo == ' ') lo++;
                            while (*hi == ' ') hi++;
                            char* he = hi + strlen(hi) - 1;
                            while (he > hi && *he == ' ') *he-- = '\0';
                            snprintf(range_buf, sizeof(range_buf), "%s..%s", lo, hi);
                            strcpy(collection, range_buf);
                        }
                        /* 3-arg range: leave as `range(...)` text; the parse
                         * error will name it explicitly rather than emit a
                         * call to the nonexistent array.range */
                    }
                }

                // NOTE: no `turbo var;` here — cruise binds the loop variable
                // itself; redeclaring with turbo SHADOWS it with null.
                snprintf(output + strlen(output),
                         MAX_LINE_LENGTH - strlen(output),
                         "cruise %s in %s {", var, collection);
                *indent_level = new_indent + 1;
                return;
            }
        }
    }

    // Rule 8: return value → return value;
    if (starts_with(converted, "return")) {
        /* Python literal names differ in RADS — convert them so
         * 'return True' doesn't leak Python keywords into RADS code. */
        replace_in_line(converted, "True", "true");
        replace_in_line(converted, "False", "false");
        replace_in_line(converted, "None", "null");
        sprintf(output + strlen(output), "%s;", converted);
        trim(output);
        return;
    }

    // Rule 9: break / continue
    if (strcmp(converted, "break") == 0 || strcmp(converted, "continue") == 0) {
        sprintf(output + strlen(output), "%s;", converted);
        return;
    }

    // Rule 10: print(...) → echo(...)
    if (starts_with(converted, "print(")) {
        char* rest = converted + 5;  // Skip "print"
        replace_in_line(rest, "True", "true");
        replace_in_line(rest, "False", "false");
        replace_in_line(rest, "None", "null");

        // Rule 10b: multi-arg print folds into string concatenation.
        // Python `print(a, b, c)` space-separates; RADS echo takes ONE expr.
        // → echo(str(a) + " " + str(b) + " " + str(c));
        // Only fires when rest is exactly "(...)" — anything else (trailing
        // comment, inline statement) keeps the legacy single-echo path.
        size_t rlen = strlen(rest);
        if (rlen >= 2 && rest[0] == '(' && rest[rlen - 1] == ')' && strchr(rest, ',')) {
            char args_buf[MAX_LINE_LENGTH];
            strncpy(args_buf, rest + 1, rlen - 2);
            args_buf[rlen - 2] = '\0';

            // Split on top-level commas (paren/bracket/brace depth + quotes)
            char* argv[64];
            int ac = 0, depth = 0, in_str = 0;
            char quote = 0;
            char* start = args_buf;
            char* p;
            for (p = args_buf; ; p++) {
                if (*p == '\0') { argv[ac++] = start; break; }
                if (in_str) {
                    if (*p == quote) in_str = 0;
                } else if (*p == '"' || *p == '\'') {
                    in_str = 1; quote = *p;
                } else if (*p == '(' || *p == '[' || *p == '{') {
                    depth++;
                } else if (*p == ')' || *p == ']' || *p == '}') {
                    depth--;
                } else if (*p == ',' && depth == 0) {
                    *p = '\0';
                    argv[ac++] = start;
                    start = p + 1;
                    if (ac >= 64) { ac = -1; break; } // too many args: bail out
                }
            }

            if (ac > 1) {
                char joined[MAX_LINE_LENGTH * 2];
                size_t jlen = 0, jcap = sizeof(joined);
                int overflow = 0;
                jlen += (size_t)snprintf(joined, jcap, "echo(");
                for (int i = 0; i < ac && !overflow; i++) {
                    // trim the arg (leading spaces; trailing cut by ',' NUL)
                    while (*argv[i] == ' ' || *argv[i] == '\t') argv[i]++;
                    char* endp = argv[i] + strlen(argv[i]) - 1;
                    while (endp > argv[i] && (*endp == ' ' || *endp == '\t')) *endp-- = '\0';
                    if (i > 0)
                        jlen += (size_t)snprintf(joined + jlen, jcap - jlen, " + \" \" + ");
                    jlen += (size_t)snprintf(joined + jlen, jcap - jlen, "str(%s)", argv[i]);
                    if (jlen >= jcap - 1) overflow = 1; // truncated: bail to legacy
                }
                if (!overflow) {
                    snprintf(joined + jlen, jcap - jlen, ");");
                    // bounded concat: joined can be ~2x MAX_LINE_LENGTH, so
                    // plain sprintf into output could overflow it
                    size_t clen = strlen(output);
                    if (clen + strlen(joined) + 1 <= MAX_LINE_LENGTH) {
                        strcpy(output + clen, joined);
                        trim(output);
                        return;
                    }
                    // didn't fit: fall through to legacy echo below
                }
                // overflow or weird arg count: fall through to legacy echo
            }
        }

        // Rule 10c (run 22): single-arg print(x) → echo(str(x)) so output
        // goes through the PYTHON serializer (float repr, True/False, None),
        // not native echo's %f format. Only when rest is exactly "(X)" with
        // no top-level comma (multi-arg is Rule 10b's job).
        rlen = strlen(rest);
        if (rlen >= 2 && rest[0] == '(' && rest[rlen - 1] == ')') {
            int tl_comma = 0, sdepth = 0, s_str = 0; char squote = 0;
            for (const char* q = rest + 1; q < rest + rlen - 1; q++) {
                if (s_str) { if (*q == squote) s_str = 0; }
                else if (*q == '"' || *q == '\'') { s_str = 1; squote = *q; }
                else if (*q == '(' || *q == '[' || *q == '{') sdepth++;
                else if (*q == ')' || *q == ']' || *q == '}') sdepth--;
                else if (*q == ',' && sdepth == 0) { tl_comma = 1; break; }
            }
            if (!tl_comma && rlen > 2) {
                char inner[MAX_LINE_LENGTH];
                strncpy(inner, rest + 1, rlen - 2);
                inner[rlen - 2] = '\0';
                /* trim so str() sees the expr, not padding */
                char* is = inner; while (*is == ' ' || *is == '\t') is++;
                char* ie = is + strlen(is) - 1;
                while (ie > is && (*ie == ' ' || *ie == '\t')) *ie-- = '\0';
                if (strlen(output) + strlen(is) + 32 <= (size_t)MAX_LINE_LENGTH) {
                    sprintf(output + strlen(output), "echo(str(%s));", is);
                    trim(output);
                    return;
                }
            }
            /* print() with nothing: empty line, like Python */
            if (rlen == 2) {
                sprintf(output + strlen(output), "echo(\"\");");
                return;
            }
        }

        sprintf(output + strlen(output), "echo%s;", rest);
        trim(output);
        return;
    }

    // Rule 11: Function calls and expressions
    if (strchr(converted, '(') && !strchr(converted, '=')) {
        sprintf(output + strlen(output), "%s;", converted);
        trim(output);
        return;
    }

    // Rule 12: import module → dropped entirely (run 22).
    // Python imports pull NAMES into scope; the old `import "module.rads"`
    // emitted a load of a file that does not exist on disk — a landmine for
    // every transpiled program. RADS stdlib natives are always available, so
    // the transpiled code just uses them directly.
    if (starts_with(converted, "import ") || starts_with(converted, "from ")) {
        output[0] = '\0';
        return;
    }

    // Default: output as-is with semicolon (if not already there)
    sprintf(output + strlen(output), "%s", converted);
    trim(output);
    if (output[strlen(output) - 1] != ';' && output[strlen(output) - 1] != '{') {
        strcat(output, ";");
    }
}

// ============================================================================
// Main Transpilation Function
// ============================================================================

int transpile_python_to_rads(FILE* input, FILE* output) {
    char line[MAX_LINE_LENGTH];
    char converted[MAX_LINE_LENGTH];
    int indent_level = 0;
    int prev_indent = 0;
    int line_number = 0;

    // Header
    fprintf(output, "// Auto-generated by rads-mask v0.0.5 \"CHAMELEON\"\n");
    fprintf(output, "// Transpiled from Python to RADS\n\n");

    while (fgets(line, sizeof(line), input)) {
        line_number++;

        /* strip trailing newline/CR so rules never embed '\n' mid-statement
         * (this was the cause of stray lone ';' lines after echo/return) */
        size_t llen = strlen(line);
        while (llen > 0 && (line[llen - 1] == '\n' || line[llen - 1] == '\r')) {
            line[--llen] = '\0';
        }

        int current_indent = count_leading_spaces(line) / 4;

        /* 'elif'/'else' emit their own leading '}' (RADS: "} elif (...) {"),
         * so the generic dedent closer must not run for them — running both
         * produced a double '}' before every elif/else chain link. */
        const char* first_token = line + count_leading_spaces(line);
        int continues_chain = starts_with(first_token, "elif ") ||
                              strcmp(first_token, "else:") == 0;

        // Handle dedents (closing braces).
        // 'elif'/'else' lines print their own leading '}' which closes the
        // immediately-enclosing body, so the generic closer must emit only
        // (dedent_count - 1) braces for them — none when the chain continues
        // at the same nesting level, extra ones when dedenting out of nested
        // blocks (e.g. inner 'else:' followed by outer 'elif:').
        if (current_indent < prev_indent) {
            int dedent_count = prev_indent - current_indent;
            if (continues_chain) dedent_count--;
            for (int i = 0; i < dedent_count; i++) {
                for (int j = 0; j < prev_indent - i - 1; j++) {
                    fprintf(output, "    ");
                }
                fprintf(output, "}\n");
            }
        }

        // Transpile the line
        transpile_line(line, converted, &indent_level);

        // Output non-empty lines
        if (converted[0] != '\0') {
            fprintf(output, "%s\n", converted);
        }

        prev_indent = current_indent;
    }

    // Close any remaining open blocks
    while (prev_indent > 0) {
        prev_indent--;
        for (int i = 0; i < prev_indent; i++) {
            fprintf(output, "    ");
        }
        fprintf(output, "}\n");
    }

    return 0;
}

// ============================================================================
// CLI Entry Point
// ============================================================================

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("RADS Masquerading Tool v0.0.5 \"CHAMELEON\"\n");
        printf("Usage: %s <input.py> [-o <output.rads>]\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s hello.py                 # Output to stdout\n", argv[0]);
        printf("  %s hello.py -o hello.rads  # Output to file\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = NULL;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            i++;
        }
    }

    // Open input file
    FILE* input = fopen(input_file, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }

    // Open output file or use stdout
    FILE* output = stdout;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
            fclose(input);
            return 1;
        }
    }

    // Perform transpilation
    int result = transpile_python_to_rads(input, output);

    // Cleanup
    fclose(input);
    if (output != stdout) {
        fclose(output);
        printf("✓ Successfully transpiled '%s' → '%s'\n", input_file, output_file);
    }

    return result;
}
