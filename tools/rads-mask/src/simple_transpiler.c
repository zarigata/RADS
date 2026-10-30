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
#include <stdarg.h>

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

static void replace_in_line(char* line, const char* from, const char* to) {
    /* Bounded: replacements can grow the text (range( -> array.range( adds
     * 6 bytes per hit), so every copy step respects the buffer end and
     * truncates instead of smashing the stack. */
    char buffer[MAX_LINE_LENGTH];
    char* pos = line;
    char* temp = buffer;
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);

    char* buf_end = buffer + sizeof(buffer) - 1;
    while (*pos && temp < buf_end) {
        if (strncmp(pos, from, from_len) == 0) {
            if (temp + to_len > buf_end) break;
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


// ============================================================================
// Bounded output append (nightshift 20261028)
// ============================================================================

/* append_formatted: vsnprintf-based append that never writes past cap.
 * Why: the old append_formatted(output, MAX_LINE_LENGTH, ...) appends could overflow
 * the fixed MAX_LINE_LENGTH buffers on long lines + keyword expansion.
 * How to extend: raise MAX_LINE_LENGTH or switch to heap GrowBuf if inputs
 * grow; returns bytes written, silently truncates at the cap (transpiler
 * output stays valid C-like text either way). */
static int append_formatted(char* output, size_t cap, const char* fmt, ...) {
    size_t used = strlen(output);
    if (used >= cap - 1) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(output + used, cap - used, fmt, ap);
    va_end(ap);
    if (n < 0) return 0;
    return n;
}

// ============================================================================
// Transpilation Rules
// ============================================================================

static void transpile_line(const char* input, char* output, int* indent_level) {
    char line[MAX_LINE_LENGTH];
    strncpy(line, input, MAX_LINE_LENGTH - 1);
    line[MAX_LINE_LENGTH - 1] = '\0';
    /* fgets keeps the trailing newline; rules that append ';' would place it
     * after the newline ("x = 42\n;"). Strip EOL here so every rule sees a
     * clean single-line string. */
    size_t line_len = strlen(line);
    while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
        line[--line_len] = '\0';
    }

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

    char converted[MAX_LINE_LENGTH];
    strcpy(converted, trimmed);

    // Rule 1: def function_name(...): → blast function_name(...) {
    if (starts_with(converted, "def ")) {
        char* colon = strrchr(converted, ':');
        if (colon) {
            *colon = '\0';
            append_formatted(output, MAX_LINE_LENGTH, "blast %s {", converted + 4);
            *indent_level = new_indent + 1;
            return;
        }
    }

    // Rule 2: Variable assignment with type inference
    // Python: x = 42  → RADS: turbo x = 42;
    char* equals = strchr(converted, '=') ;
    if (equals && equals > converted) {
        // Check if it's not a comparison and no parens before equals
        if (*(equals - 1) != '=' && *(equals - 1) != '!' &&
            *(equals - 1) != '<' && *(equals - 1) != '>') {

            // Check if there are parentheses before the equals (like function def)
            int has_paren_before = 0;
            for (char* p = converted; p < equals; p++) {
                if (*p == '(') {
                    has_paren_before = 1;
                    break;
                }
            }

            if (!has_paren_before) {
                char var_name[MAX_LINE_LENGTH];
                char* start = converted;
                while (isspace(*start)) start++;
                char* end = equals - 1;
                while (end > start && isspace(*end)) end--;
                int len = end - start + 1;
                if (len >= (int)sizeof(var_name)) len = sizeof(var_name) - 1;
                if (len < 0) len = 0;
                strncpy(var_name, start, len);
                var_name[len] = '\0';

                char* value = equals + 1;
                while (isspace(*value)) value++;

                // Always use turbo for variable declarations
                append_formatted(output, MAX_LINE_LENGTH, "turbo %s = %s;", var_name, value);
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

            append_formatted(output, MAX_LINE_LENGTH, "if (%s) {", condition);
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

            // Reduce indent for closing brace
            output[0] = '\0';
            for (int i = 0; i < new_indent - 1; i++) {
                strcat(output, "    ");
            }
            append_formatted(output, MAX_LINE_LENGTH, "} elif (%s) {", condition);
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

            append_formatted(output, MAX_LINE_LENGTH, "loop (%s) {", condition);
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

                char var[MAX_LINE_LENGTH];
                snprintf(var, sizeof(var), "%s", converted + 4);
                trim(var);

                char collection[MAX_LINE_LENGTH];
                strcpy(collection, in_pos + 4);
                trim(collection);

                replace_in_line(collection, "range(", "array.range(");

                append_formatted(output, MAX_LINE_LENGTH, "cruise (%s) { turbo %s;", collection, var);
                *indent_level = new_indent + 1;
                return;
            }
        }
    }

    // Rule 8: return value → return value;
    if (starts_with(converted, "return")) {
        append_formatted(output, MAX_LINE_LENGTH, "%s;", converted);
        trim(output);
        return;
    }

    // Rule 9: break / continue
    if (strcmp(converted, "break") == 0 || strcmp(converted, "continue") == 0) {
        append_formatted(output, MAX_LINE_LENGTH, "%s;", converted);
        return;
    }

    // Rule 10: print(...) → echo(...)
    if (starts_with(converted, "print(")) {
        char* rest = converted + 5;  // Skip "print"
        replace_in_line(rest, "True", "true");
        replace_in_line(rest, "False", "false");
        replace_in_line(rest, "None", "null");
        append_formatted(output, MAX_LINE_LENGTH, "echo%s;", rest);
        trim(output);
        return;
    }

    // Rule 11: Function calls and expressions
    if (strchr(converted, '(') && !strchr(converted, '=')) {
        append_formatted(output, MAX_LINE_LENGTH, "%s;", converted);
        trim(output);
        return;
    }

    // Rule 12: import module → import "module.rads";
    if (starts_with(converted, "import ")) {
        char module[MAX_LINE_LENGTH];
        snprintf(module, sizeof(module), "%s", converted + 7);
        trim(module);
        append_formatted(output, MAX_LINE_LENGTH, "import \"%s.rads\";", module);
        return;
    }

    // Default: output as-is with semicolon (if not already there)
    append_formatted(output, MAX_LINE_LENGTH, "%s", converted);
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
        int current_indent = count_leading_spaces(line) / 4;

        // Handle dedents (closing braces)
        if (current_indent < prev_indent) {
            int dedent_count = prev_indent - current_indent;
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
