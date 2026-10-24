#include "stdlib_regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// Maximum number of cached compiled patterns
#define MAX_REGEX_CACHE 32

typedef struct {
    char* pattern;      // original pattern as the user wrote it (cache key)
    char* ere;          // translated POSIX ERE (named groups become plain groups)
    char** names;       // named group names, declaration order
    int* name_group;    // name_group[i] = ERE capture index for names[i]
    int name_count;     // how many named groups the pattern declared
    regex_t compiled;
    bool valid;
} CachedRegex;

static CachedRegex regex_cache[MAX_REGEX_CACHE];
static int cache_count = 0;

// Shared struct definition so named-group dicts can be printed safely
// ("MatchGroups {year: 2026}") without dereferencing a garbage definition.
// Allocated once, never freed -- value_release() treats definitions as
// registry-owned, so a static singleton is the leak-free choice.
static StructDef* match_groups_def(void) {
    static StructDef def;
    static bool init = false;
    if (!init) {
        def.name = strdup("MatchGroups");
        def.ast_node = NULL;
        init = true;
    }
    return &def;
}

// True if c can appear in a group name (Python identifier rules, loose check:
// letter/underscore first is enforced by the caller, alnum/underscore here).
static bool is_name_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

// Translate a Python-style pattern into POSIX ERE, because regcomp(3) has no
// named groups: "(?P<year>...)" and "(?<year>...)" become "(...)" while the
// name -> capture-index mapping is recorded for named_groups()/backrefs.
// "(?:...)" becomes a plain capture group (POSIX has no non-capturing group;
// numbering of *named* groups stays correct because we count what we emit).
// Lookarounds are rejected with a clear error -- POSIX ERE cannot express them.
// Returns true on success; on failure *err gets a static message.
static bool translate_pattern(const char* pat, char** ere_out, char*** names_out,
                              int** idx_out, int* name_count_out, const char** err) {
    size_t plen = strlen(pat);
    char* ere = malloc(plen + 1);      // translated is never longer than input
    char** names = NULL;
    int* idx = NULL;
    int nsub = 0, name_count = 0;
    size_t o = 0;
    bool in_class = false;

    for (size_t i = 0; i < plen; i++) {
        char c = pat[i];
        if (c == '\\' && i + 1 < plen) {           // escaped char: copy both
            ere[o++] = c;
            ere[o++] = pat[++i];
            continue;
        }
        if (in_class) {
            ere[o++] = c;
            if (c == ']') in_class = false;        // ']' closes unless first (kept literal)
            continue;
        }
        if (c == '[') {
            ere[o++] = c;
            in_class = true;
            // "[]" and "[^" make the next ']' a literal member: pre-copy it so
            // the in_class branch above does not close early.
            if (i + 1 < plen && (pat[i+1] == '^' ? (i + 2 < plen && pat[i+2] == ']')
                                                 : pat[i+1] == ']')) {
                ere[o++] = pat[++i];
                if (pat[i] == '^' && i + 1 < plen && pat[i+1] == ']') ere[o++] = pat[++i];
            }
            continue;
        }
        if (c == '(') {
            // Named group: (?P<name>...) Python style or (?<name>...) .NET style
            const char* p = NULL;
            size_t ni = 0;
            if (strncmp(pat + i, "(?P<", 4) == 0) { p = pat + i + 4; ni = i + 4; }
            else if (strncmp(pat + i, "(?<", 3) == 0 && pat[i+2] != '=') { p = pat + i + 3; ni = i + 3; }
            if (p) {
                size_t j = ni;
                if (j >= plen || !(isalpha((unsigned char)pat[j]) || pat[j] == '_')) {
                    free(ere); free(names); free(idx);
                    *err = "named group needs a name starting with a letter or underscore";
                    return false;
                }
                while (j < plen && is_name_char(pat[j])) j++;
                if (j >= plen || pat[j] != '>') {
                    free(ere); free(names); free(idx);
                    *err = "named group is missing its closing '>'";
                    return false;
                }
                nsub++;
                // record name -> capture index (duplicate names rejected, like Python)
                for (int k = 0; k < name_count; k++) {
                    size_t nl = j - ni;
                    if (strlen(names[k]) == nl && strncmp(names[k], pat + ni, nl) == 0) {
                        free(ere); free(names); free(idx);
                        *err = "duplicate group name in pattern";
                        return false;
                    }
                }
                char** nn = realloc(names, (size_t)(name_count + 1) * sizeof(char*));
                if (!nn) { free(ere); free(idx); free(names); *err = "out of memory"; return false; }
                names = nn;
                int* nx = realloc(idx, (size_t)(name_count + 1) * sizeof(int));
                if (!nx) { free(ere); free(names); *err = "out of memory"; return false; }
                idx = nx;
                size_t nl = j - ni;
                names[name_count] = malloc(nl + 1);
                memcpy(names[name_count], pat + ni, nl);
                names[name_count][nl] = '\0';
                idx[name_count] = nsub;
                name_count++;
                ere[o++] = '(';
                i = j;                    // skip past "...name>" -- loop's i++ lands on group body
                continue;
            }
            if (strncmp(pat + i, "(?:", 3) == 0) {  // non-capturing -> plain capture
                nsub++;
                ere[o++] = '(';
                i += 2;
                continue;
            }
            if (strncmp(pat + i, "(?=", 3) == 0 || strncmp(pat + i, "(?!", 3) == 0) {
                free(ere); free(names); free(idx);
                *err = "lookahead assertions are not supported by the regex engine";
                return false;
            }
            nsub++;                                // plain capture group
            ere[o++] = c;
            continue;
        }
        ere[o++] = c;
    }
    ere[o] = '\0';
    if (in_class) { free(ere); free(names); free(idx); *err = "unterminated character class"; return false; }

    *ere_out = ere;
    *names_out = names;
    *idx_out = idx;
    *name_count_out = name_count;
    return true;
}

// Helper: Get or compile a regex pattern (translated to ERE on first sight).
// NOTE: the returned regex_t* lives inside the cache; compiling another
// pattern may evict entries, so never hold this pointer across another
// get_compiled_regex() call.
static regex_t* get_compiled_regex(const char* pattern, CachedRegex** entry_out) {
    // Check cache first
    for (int i = 0; i < cache_count; i++) {
        if (regex_cache[i].valid && strcmp(regex_cache[i].pattern, pattern) == 0) {
            if (entry_out) *entry_out = &regex_cache[i];
            return &regex_cache[i].compiled;
        }
    }

    // Compile new pattern
    if (cache_count >= MAX_REGEX_CACHE) {
        // Clear oldest cache entry
        regfree(&regex_cache[0].compiled);
        free(regex_cache[0].pattern);
        free(regex_cache[0].ere);
        for (int k = 0; k < regex_cache[0].name_count; k++) free(regex_cache[0].names[k]);
        free(regex_cache[0].names);
        free(regex_cache[0].name_group);
        memmove(&regex_cache[0], &regex_cache[1], (MAX_REGEX_CACHE - 1) * sizeof(CachedRegex));
        cache_count--;
    }

    CachedRegex* slot = &regex_cache[cache_count];
    const char* err = NULL;
    if (!translate_pattern(pattern, &slot->ere, &slot->names, &slot->name_group,
                           &slot->name_count, &err)) {
        fprintf(stderr, "Regex error: %s (pattern: %s)\n", err ? err : "bad pattern", pattern);
        slot->valid = false;
        return NULL;
    }

    int result = regcomp(&slot->compiled, slot->ere, REG_EXTENDED);
    if (result != 0) {
        char error_buf[256];
        regerror(result, &slot->compiled, error_buf, sizeof(error_buf));
        fprintf(stderr, "Regex error: %s (pattern: %s)\n", error_buf, pattern);
        free(slot->ere);
        for (int k = 0; k < slot->name_count; k++) free(slot->names[k]);
        free(slot->names);
        free(slot->name_group);
        slot->valid = false;
        return NULL;
    }

    slot->pattern = strdup(pattern);
    slot->valid = true;
    cache_count++;

    if (entry_out) *entry_out = &regex_cache[cache_count - 1];
    return &regex_cache[cache_count - 1].compiled;
}

// regex.compile(pattern) - Compile and cache a regex pattern, returns pattern string as handle
Value stdlib_regex_compile(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 1) {
        fprintf(stderr, "Error: regex.compile() requires 1 argument (pattern)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.compile() argument must be a string\n");
        return make_null();
    }

    regex_t* regex = get_compiled_regex(args[0].string_val, NULL);
    if (regex == NULL) {
        return make_null();
    }

    // Return the pattern string as a handle
    return make_string(args[0].string_val);
}

// regex.match(pattern, string) - Boolean match check
Value stdlib_regex_match(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 2) {
        fprintf(stderr, "Error: regex.match() requires 2 arguments (pattern, string)\n");
        return make_bool(false);
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.match() arguments must be strings\n");
        return make_bool(false);
    }

    regex_t* regex = get_compiled_regex(args[0].string_val, NULL);
    if (regex == NULL) {
        return make_bool(false);
    }

    int result = regexec(regex, args[1].string_val, 0, NULL, 0);
    return make_bool(result == 0);
}

// regex.findall(pattern, string) - Find all matches, returns array of strings
Value stdlib_regex_findall(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 2) {
        fprintf(stderr, "Error: regex.findall() requires 2 arguments (pattern, string)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.findall() arguments must be strings\n");
        return make_null();
    }

    regex_t* regex = get_compiled_regex(args[0].string_val, NULL);
    if (regex == NULL) {
        return make_null();
    }

    const char* str = args[1].string_val;
    Array* result_arr = array_create(8);

    // Find maximum number of groups
    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    const char* cursor = str;
    while (*cursor != '\0') {
        int result = regexec(regex, cursor, n_groups, matches, 0);

        if (result != 0) {
            break;  // No more matches
        }

        // Get the full match (group 0)
        int start = matches[0].rm_so;
        int end = matches[0].rm_eo;

        if (start == -1 || end == -1) {
            break;
        }

        // Extract match string
        int len = end - start;
        char* match_str = malloc(len + 1);
        memcpy(match_str, cursor + start, len);
        match_str[len] = '\0';

        // Add to result array
        Value item;
        item.type = VAL_STRING;
        item.string_val = match_str;
        array_push(result_arr, item);

        // Move cursor past this match
        if (end == 0) {
            cursor++;  // Prevent infinite loop on empty match
        } else {
            cursor += end;
        }
    }

    free(matches);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

// Helper: append a string field to a struct instance under construction.
// Fields are a linked list built in reverse; order is irrelevant for lookup.
static bool add_string_field(StructInstance* instance, const char* name, const char* value) {
    FieldValue* field = malloc(sizeof(FieldValue));
    if (!field) return false;
    field->name = strdup(name);
    field->value = malloc(sizeof(Value));
    if (!field->name || !field->value) { free(field->name); free(field->value); free(field); return false; }
    field->value->type = VAL_STRING;
    field->value->string_val = strdup(value);
    field->next = instance->fields;
    instance->fields = field;
    return true;
}

// Helper: append an int field to a struct instance under construction.
static bool add_int_field(StructInstance* instance, const char* name, long long value) {
    FieldValue* field = malloc(sizeof(FieldValue));
    if (!field) return false;
    field->name = strdup(name);
    field->value = malloc(sizeof(Value));
    if (!field->name || !field->value) { free(field->name); free(field->value); free(field); return false; }
    field->value->type = VAL_INT;
    field->value->int_val = value;
    field->next = instance->fields;
    instance->fields = field;
    return true;
}

// regex.search(pattern, string) - Find first match with position info
// Returns struct { match: string, start: int, end: int } or null if not found
Value stdlib_regex_search(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 2) {
        fprintf(stderr, "Error: regex.search() requires 2 arguments (pattern, string)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.search() arguments must be strings\n");
        return make_null();
    }

    regex_t* regex = get_compiled_regex(args[0].string_val, NULL);
    if (regex == NULL) {
        return make_null();
    }

    const char* str = args[1].string_val;
    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    int result = regexec(regex, str, n_groups, matches, 0);

    if (result != 0) {
        free(matches);
        return make_null();  // No match
    }

    // Extract match info
    int start = matches[0].rm_so;
    int end = matches[0].rm_eo;
    int len = end - start;

    char* match_str = malloc(len + 1);
    memcpy(match_str, str + start, len);
    match_str[len] = '\0';

    free(matches);

    // Create result struct (definition set so echo() can print it safely)
    StructInstance* instance = malloc(sizeof(StructInstance));
    if (!instance) {
        free(match_str);
        return make_null();
    }

    instance->definition = NULL;
    instance->fields = NULL;

    if (!add_string_field(instance, "match", match_str) ||
        !add_int_field(instance, "start", start) ||
        !add_int_field(instance, "end", end)) {
        free(match_str);
        return make_null();
    }
    free(match_str);

    Value ret;
    ret.type = VAL_STRUCT_INSTANCE;
    ret.struct_instance = instance;
    return ret;
}

// ---- Growable buffer: regex.replace used to size its output as
// 2*len(str)+2*len(repl), which overflows the heap whenever many short
// matches each expand into a long replacement (ASan-confirmed). This is
// the structural fix: grow on demand, never guess. ----
typedef struct {
    char* data;
    size_t len;
    size_t cap;
} GrowBuf;

// Ensure room for `extra` more bytes (plus the NUL we always keep).
static bool gb_reserve(GrowBuf* b, size_t extra) {
    if (b->len + extra + 1 <= b->cap) return true;
    size_t want = b->cap ? b->cap : 64;
    while (want < b->len + extra + 1) want *= 2;
    char* p = realloc(b->data, want);
    if (!p) return false;
    b->data = p;
    b->cap = want;
    return true;
}

// Append n bytes from s (n may be 0; s may be NULL then).
static bool gb_append(GrowBuf* b, const char* s, size_t n) {
    if (n && !s) return false;
    if (!gb_reserve(b, n)) return false;
    if (n) memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return true;
}

// Expand one replacement template against a match: supports \\g<name>,
// \\g<digits> and \\1..\\9 backreferences plus \\\\ for a literal backslash.
// `base` is the string regexec matched against (group offsets are relative
// to it). Returns true on success; unknown names/groups append nothing and
// report to stderr (visible, not silent).
static bool expand_replacement(const char* repl, const CachedRegex* entry,
                               const regmatch_t* matches, const char* base, GrowBuf* out) {
    size_t rlen = strlen(repl);
    for (size_t i = 0; i < rlen; i++) {
        if (repl[i] != '\\') {
            if (!gb_append(out, &repl[i], 1)) return false;
            continue;
        }
        if (i + 1 >= rlen) {                     // trailing lone backslash: literal
            if (!gb_append(out, "\\", 1)) return false;
            break;
        }
        char next = repl[i + 1];
        if (next == '\\') {                      // escaped backslash
            if (!gb_append(out, "\\", 1)) return false;
            i++;
            continue;
        }
        if (next == 'g' && i + 2 < rlen && repl[i + 2] == '<') {
            size_t j = i + 3;
            size_t nm_start = j;
            while (j < rlen && repl[j] != '>') j++;
            if (j >= rlen) {
                fprintf(stderr, "Regex replace: \\g<...> missing '>' in replacement\n");
                continue;
            }
            size_t nlen = j - nm_start;
            int group = -1;
            if (nlen > 0) {
                bool all_digit = true;
                for (size_t k = nm_start; k < j; k++)
                    if (!isdigit((unsigned char)repl[k])) { all_digit = false; break; }
                if (all_digit) group = atoi(repl + nm_start);
                else {
                    for (int k = 0; k < entry->name_count; k++) {
                        if (strlen(entry->names[k]) == nlen &&
                            strncmp(entry->names[k], repl + nm_start, nlen) == 0) {
                            group = entry->name_group[k];
                            break;
                        }
                    }
                }
            }
            if (group < 0 || (size_t)group > entry->compiled.re_nsub) {
                fprintf(stderr, "Regex replace: unknown group \\g<%.*s> in replacement\n",
                        (int)nlen, repl + nm_start);
            } else if (matches[group].rm_so != -1) {
                if (!gb_append(out, base + matches[group].rm_so,
                               (size_t)(matches[group].rm_eo - matches[group].rm_so))) return false;
            }
            i = j;
            continue;
        }
        if (next >= '0' && next <= '9') {
            int group = next - '0';
            if ((size_t)group <= entry->compiled.re_nsub && matches[group].rm_so != -1) {
                if (!gb_append(out, base + matches[group].rm_so,
                               (size_t)(matches[group].rm_eo - matches[group].rm_so))) return false;
            }
            i++;
            continue;
        }
        // any other escape: emit backslash + char as-is
        if (!gb_append(out, &repl[i], 2)) return false;
        i++;
    }
    return true;
}

// regex.replace(pattern, string, replacement) - Replace matches.
// Replacement understands \\g<name>, \\g<1>, \\1 backreferences.
Value stdlib_regex_replace(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 3) {
        fprintf(stderr, "Error: regex.replace() requires 3 arguments (pattern, string, replacement)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING || args[2].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.replace() arguments must be strings\n");
        return make_null();
    }

    CachedRegex* entry = NULL;
    regex_t* regex = get_compiled_regex(args[0].string_val, &entry);
    if (regex == NULL || entry == NULL) {
        return make_null();
    }

    const char* str = args[1].string_val;
    const char* replacement = args[2].string_val;

    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    GrowBuf out = {0};
    if (!gb_append(&out, "", 0)) { free(matches); return make_null(); }

    const char* cursor = str;
    bool replaced = false;

    while (*cursor != '\0') {
        int reg_result = regexec(regex, cursor, n_groups, matches, 0);

        if (reg_result != 0) {
            // No more matches, copy rest of string
            if (!gb_append(&out, cursor, strlen(cursor))) { free(out.data); free(matches); return make_null(); }
            break;
        }

        int start = matches[0].rm_so;
        int end = matches[0].rm_eo;

        if (start == -1 || end == -1) {
            if (!gb_append(&out, cursor, strlen(cursor))) { free(out.data); free(matches); return make_null(); }
            break;
        }

        // Copy text before match
        if (!gb_append(&out, cursor, (size_t)start)) { free(out.data); free(matches); return make_null(); }

        // Copy replacement (with backreference expansion)
        if (!expand_replacement(replacement, entry, matches, cursor, &out)) {
            free(out.data); free(matches); return make_null();
        }

        replaced = true;

        // Move cursor past match
        if (end == 0) {
            if (!gb_append(&out, cursor, 1)) { free(out.data); free(matches); return make_null(); }
            cursor++;
        } else {
            cursor += end;
        }
    }

    free(matches);

    // If no replacement happened, return original string
    if (!replaced) {
        free(out.data);
        return make_string(args[1].string_val);
    }

    Value ret = make_string(out.data);
    free(out.data);
    return ret;
}

// regex.split(pattern, string) - Split string by pattern
Value stdlib_regex_split(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 2) {
        fprintf(stderr, "Error: regex.split() requires 2 arguments (pattern, string)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.split() arguments must be strings\n");
        return make_null();
    }

    regex_t* regex = get_compiled_regex(args[0].string_val, NULL);
    if (regex == NULL) {
        return make_null();
    }

    const char* str = args[1].string_val;
    Array* result_arr = array_create(8);

    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    const char* cursor = str;
    const char* last_end = str;

    while (*cursor != '\0') {
        int result = regexec(regex, cursor, n_groups, matches, 0);

        if (result != 0) {
            // No more matches, add rest of string
            if (*cursor != '\0') {
                char* part = strdup(cursor);
                Value item;
                item.type = VAL_STRING;
                item.string_val = part;
                array_push(result_arr, item);
            }
            break;
        }

        int start = matches[0].rm_so;
        int end = matches[0].rm_eo;

        if (start == -1 || end == -1) {
            break;
        }

        // Extract text before match
        int len = (int)((cursor + start) - last_end);
        char* part = malloc(len + 1);
        memcpy(part, last_end, len);
        part[len] = '\0';

        Value item;
        item.type = VAL_STRING;
        item.string_val = part;
        array_push(result_arr, item);

        // Move past match
        last_end = cursor + end;
        if (end == 0) {
            cursor++;
        } else {
            cursor += end;
        }
    }

    free(matches);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = result_arr;
    return result;
}

// regex.groups(pattern, string) - Extract capture groups
// Returns array of captured groups or null if no match
Value stdlib_regex_groups(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 2) {
        fprintf(stderr, "Error: regex.groups() requires 2 arguments (pattern, string)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.groups() arguments must be strings\n");
        return make_null();
    }

    regex_t* regex = get_compiled_regex(args[0].string_val, NULL);
    if (regex == NULL) {
        return make_null();
    }

    const char* str = args[1].string_val;
    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    int result = regexec(regex, str, n_groups, matches, 0);

    if (result != 0) {
        free(matches);
        return make_null();  // No match
    }

    Array* result_arr = array_create(n_groups);

    // Extract all groups (skip group 0 which is the full match)
    for (size_t i = 1; i < n_groups; i++) {
        if (matches[i].rm_so != -1 && matches[i].rm_eo != -1) {
            int len = matches[i].rm_eo - matches[i].rm_so;
            char* group_str = malloc(len + 1);
            memcpy(group_str, str + matches[i].rm_so, len);
            group_str[len] = '\0';

            Value item;
            item.type = VAL_STRING;
            item.string_val = group_str;
            array_push(result_arr, item);
        } else {
            // No match for this group, add empty string
            Value item;
            item.type = VAL_STRING;
            item.string_val = strdup("");
            array_push(result_arr, item);
        }
    }

    free(matches);

    Value ret;
    ret.type = VAL_ARRAY;
    ret.array_val = result_arr;
    return ret;
}

// regex.named_groups(pattern, string) - Python's match.groupdict()
// Returns a struct with one field per named group ("(?P<year>...)" gives
// .year), unmatched optional groups become "", or null when the pattern
// does not match at all.
Value stdlib_regex_named_groups(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 2) {
        fprintf(stderr, "Error: regex.named_groups() requires 2 arguments (pattern, string)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.named_groups() arguments must be strings\n");
        return make_null();
    }

    CachedRegex* entry = NULL;
    regex_t* regex = get_compiled_regex(args[0].string_val, &entry);
    if (regex == NULL || entry == NULL) {
        return make_null();
    }

    const char* str = args[1].string_val;
    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    int result = regexec(regex, str, n_groups, matches, 0);

    if (result != 0) {
        free(matches);
        return make_null();  // No match: Python returns None only via m; here null
    }

    StructInstance* instance = malloc(sizeof(StructInstance));
    if (!instance) { free(matches); return make_null(); }
    instance->definition = match_groups_def();
    instance->fields = NULL;

    bool ok = true;
    for (int i = 0; ok && i < entry->name_count; i++) {
        int gi = entry->name_group[i];
        if ((size_t)gi < n_groups && matches[gi].rm_so != -1) {
            int len = matches[gi].rm_eo - matches[gi].rm_so;
            char* buf = malloc(len + 1);
            if (!buf) { ok = false; break; }
            memcpy(buf, str + matches[gi].rm_so, len);
            buf[len] = '\0';
            ok = add_string_field(instance, entry->names[i], buf);
            free(buf);
        } else {
            ok = add_string_field(instance, entry->names[i], "");
        }
    }

    free(matches);

    if (!ok) {
        // best effort: instance fields are freed by value_release on the caller
        // side, but we never handed it out -- free manually
        FieldValue* cur = instance->fields;
        while (cur) { FieldValue* nx = cur->next; free(cur->name); free(cur->value); free(cur); cur = nx; }
        free(instance);
        return make_null();
    }

    Value ret;
    ret.type = VAL_STRUCT_INSTANCE;
    ret.struct_instance = instance;
    return ret;
}

// regex.group_names(pattern) - Array of named group names in declaration
// order (empty array when the pattern has none). Parity probe for
// Python's compiled.groupindex keys.
Value stdlib_regex_group_names(Interpreter* interp, int argc, Value* args) {
    (void)interp;

    if (argc < 1) {
        fprintf(stderr, "Error: regex.group_names() requires 1 argument (pattern)\n");
        return make_null();
    }

    if (args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: regex.group_names() argument must be a string\n");
        return make_null();
    }

    CachedRegex* entry = NULL;
    regex_t* regex = get_compiled_regex(args[0].string_val, &entry);
    if (regex == NULL || entry == NULL) {
        return make_null();
    }

    Array* result_arr = array_create((size_t)(entry->name_count > 0 ? entry->name_count : 1));
    for (int i = 0; i < entry->name_count; i++) {
        array_push(result_arr, make_string(entry->names[i]));
    }

    Value ret;
    ret.type = VAL_ARRAY;
    ret.array_val = result_arr;
    return ret;
}

void stdlib_regex_register(void) {
    register_native("regex.compile", stdlib_regex_compile);
    register_native("regex.match", stdlib_regex_match);
    register_native("regex.findall", stdlib_regex_findall);
    register_native("regex.search", stdlib_regex_search);
    register_native("regex.replace", stdlib_regex_replace);
    register_native("regex.split", stdlib_regex_split);
    register_native("regex.groups", stdlib_regex_groups);
    register_native("regex.named_groups", stdlib_regex_named_groups);
    register_native("regex.group_names", stdlib_regex_group_names);

    // Initialize cache
    for (int i = 0; i < MAX_REGEX_CACHE; i++) {
        regex_cache[i].pattern = NULL;
        regex_cache[i].ere = NULL;
        regex_cache[i].names = NULL;
        regex_cache[i].name_group = NULL;
        regex_cache[i].name_count = 0;
        regex_cache[i].valid = false;
    }
    cache_count = 0;
}

// ⚕ Forged for Master Zarigata 🜂
