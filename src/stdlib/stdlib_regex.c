#include "stdlib_regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// Maximum number of cached compiled patterns
#define MAX_REGEX_CACHE 32

typedef struct {
    char* pattern;
    regex_t compiled;
    bool valid;
} CachedRegex;

static CachedRegex regex_cache[MAX_REGEX_CACHE];
static int cache_count = 0;

// Helper: Get or compile a regex pattern
static regex_t* get_compiled_regex(const char* pattern) {
    // Check cache first
    for (int i = 0; i < cache_count; i++) {
        if (regex_cache[i].valid && strcmp(regex_cache[i].pattern, pattern) == 0) {
            return &regex_cache[i].compiled;
        }
    }
    
    // Compile new pattern
    if (cache_count >= MAX_REGEX_CACHE) {
        // Clear oldest cache entry
        regfree(&regex_cache[0].compiled);
        free(regex_cache[0].pattern);
        memmove(&regex_cache[0], &regex_cache[1], (MAX_REGEX_CACHE - 1) * sizeof(CachedRegex));
        cache_count--;
    }
    
    int result = regcomp(&regex_cache[cache_count].compiled, pattern, REG_EXTENDED);
    if (result != 0) {
        char error_buf[256];
        regerror(result, &regex_cache[cache_count].compiled, error_buf, sizeof(error_buf));
        fprintf(stderr, "Regex error: %s\n", error_buf);
        return NULL;
    }
    
    regex_cache[cache_count].pattern = strdup(pattern);
    regex_cache[cache_count].valid = true;
    cache_count++;
    
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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
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
        strncpy(match_str, cursor + start, len);
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

// regex.search(pattern, string) - Find first match with position info
// Returns struct { match: string, start: int, end: int } or null if not found
// Run 26: the result instance's `definition` was left uninitialized —
// echo()-ing a search result dereferenced garbage (0xbebebebe...) and
// crashed in value_to_string_impl under ASAN. The free path treats
// `definition` as borrowed (value_release never frees it), so a static
// singleton label is the safe fix.
static StructDef regex_search_result_def = { "match", NULL };

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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
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
    strncpy(match_str, str + start, len);
    match_str[len] = '\0';
    
    free(matches);
    
    // Create result struct
    StructInstance* instance = malloc(sizeof(StructInstance));
    if (!instance) {
        free(match_str);
        return make_null();
    }
    
    instance->definition = &regex_search_result_def;
    instance->fields = NULL;
    
    // Add match field
    FieldValue* match_field = malloc(sizeof(FieldValue));
    match_field->name = strdup("match");
    match_field->value = malloc(sizeof(Value));
    match_field->value->type = VAL_STRING;
    match_field->value->string_val = match_str;
    match_field->next = instance->fields;
    instance->fields = match_field;
    
    // Add start field
    FieldValue* start_field = malloc(sizeof(FieldValue));
    start_field->name = strdup("start");
    start_field->value = malloc(sizeof(Value));
    start_field->value->type = VAL_INT;
    start_field->value->int_val = start;
    start_field->next = instance->fields;
    instance->fields = start_field;
    
    // Add end field
    FieldValue* end_field = malloc(sizeof(FieldValue));
    end_field->name = strdup("end");
    end_field->value = malloc(sizeof(Value));
    end_field->value->type = VAL_INT;
    end_field->value->int_val = end;
    end_field->next = instance->fields;
    instance->fields = end_field;
    
    Value ret;
    ret.type = VAL_STRUCT_INSTANCE;
    ret.struct_instance = instance;
    return ret;
}

// regex.replace(pattern, string, replacement) - Replace matches
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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
    if (regex == NULL) {
        return make_null();
    }
    
    const char* str = args[1].string_val;
    const char* replacement = args[2].string_val;
    size_t repl_len = strlen(replacement);

    size_t n_groups = regex->re_nsub + 1;
    regmatch_t* matches = malloc(n_groups * sizeof(regmatch_t));

    // Two-pass replace (run 26; sibling of the run-25 str.replace fix).
    // Pass 1 sizes the result EXACTLY using the same regexec walk the
    // write pass uses. The old estimate (2*strlen + 2*repl_len) heap-
    // overflowed on all-match strings with multi-char replacements
    // (e.g. [0-9] on "1111111111" with "XYZ": needs 31, allocated 26).
    // Empty matches keep the current char literally and advance one
    // byte; a trailing empty match at EOS still fires one replacement
    // (Python parity: re.sub(r'x*','-','abc') == '-a-b-c-').
    size_t copy_len = 0;   // literal bytes kept
    size_t n_matches = 0;
    const char* cursor = str;
    while (*cursor != '\0') {
        if (regexec(regex, cursor, n_groups, matches, 0) != 0) break;
        int start = matches[0].rm_so;
        int end = matches[0].rm_eo;
        if (start == -1 || end == -1) break;
        copy_len += (size_t)start;
        n_matches++;
        if (end == start) {          // empty match: keep this char
            copy_len++;
            cursor++;
        } else {
            cursor += end;
        }
    }
    size_t tail_len = strlen(cursor);
    // Trailing empty match at EOS: if the pattern can match empty it
    // matches once more at end-of-string (Python parity:
    // re.sub(r'a*','-','aaaa') == '--'). The loop above consumes every
    // byte via empty-match advances, so reaching tail_len==0 means the
    // whole string was walked and the EOS probe is the only remaining
    // match. Probe the compiled regex against "" — cheap and exact for
    // POSIX ERE (no lookarounds to lie about adjacency).
    bool eos_match = false;
    {
        regmatch_t probe;
        if (tail_len == 0 &&
            regexec(regex, "", 1, &probe, 0) == 0 &&
            probe.rm_so == 0 && probe.rm_eo == 0) {
            eos_match = true;
            n_matches++;
        }
    }

    char* result = malloc(copy_len + tail_len + n_matches * repl_len + 1);

    // Pass 2: write (mirror of pass 1 — any divergence is a bug)
    char* result_ptr = result;
    cursor = str;
    bool replaced = false;
    while (*cursor != '\0') {
        if (regexec(regex, cursor, n_groups, matches, 0) != 0) break;
        int start = matches[0].rm_so;
        int end = matches[0].rm_eo;
        if (start == -1 || end == -1) break;
        memcpy(result_ptr, cursor, (size_t)start);
        result_ptr += start;
        memcpy(result_ptr, replacement, repl_len);
        result_ptr += repl_len;
        replaced = true;
        // Move cursor past match
        if (end == start) {          // empty match: copy the char
            *result_ptr++ = *cursor++;
        } else {
            cursor += end;
        }
    }
    // Trailing empty match at EOS (sized in pass 1, written here)
    if (eos_match) {
        memcpy(result_ptr, replacement, repl_len);
        result_ptr += repl_len;
        replaced = true;
    }
    strcpy(result_ptr, cursor);      // tail; buffer sized exactly for it

    free(matches);

    // If no replacement happened, return original string
    if (!replaced) {
        free(result);
        return make_string(args[1].string_val);
    }
    
    Value ret = make_string(result);
    free(result);
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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
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
        int len = (cursor + start) - last_end;
        char* part = malloc(len + 1);
        strncpy(part, last_end, len);
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
    
    // Add remaining part if cursor stopped at end
    if (*last_end != '\0' && *cursor == '\0') {
        // Already handled above
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
    
    regex_t* regex = get_compiled_regex(args[0].string_val);
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
            strncpy(group_str, str + matches[i].rm_so, len);
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

void stdlib_regex_register(void) {
    register_native("regex.compile", stdlib_regex_compile);
    register_native("regex.match", stdlib_regex_match);
    register_native("regex.findall", stdlib_regex_findall);
    register_native("regex.search", stdlib_regex_search);
    register_native("regex.replace", stdlib_regex_replace);
    register_native("regex.split", stdlib_regex_split);
    register_native("regex.groups", stdlib_regex_groups);
    
    // Initialize cache
    for (int i = 0; i < MAX_REGEX_CACHE; i++) {
        regex_cache[i].pattern = NULL;
        regex_cache[i].valid = false;
    }
    cache_count = 0;
}