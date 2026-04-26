#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "stdlib_io.h"
#include "stdlib_media.h"
#include "stdlib_net.h"
#include "stdlib_ffi.h"
#include "stdlib_string.h"
#include "stdlib_string_advanced.h"
#include "stdlib_math.h"
#include "stdlib_math_extended.h"
#include "stdlib_array.h"
#include "stdlib_fs.h"
#include "stdlib_json.h"
#include "stdlib_db.h"
#include "stdlib_webengine.h"
#include "stdlib_async_utils.h"
#include "stdlib_websocket.h"
#include "stdlib_graphql.h"
#include "stdlib_datetime.h"
#include "stdlib_regex.h"
#include "stdlib_test.h"
#include "stdlib_env.h"
#include "stdlib_cli.h"
#include "stdlib_csv.h"
#include "stdlib_base64.h"
#include "stdlib_random.h"
#include "stdlib_path.h"
#include "stdlib_time.h"
#include "stdlib_process.h"
#include "stdlib_uuid.h"
#include "stdlib_collections.h"
#include "stdlib_hash.h"
#include "stdlib_sort.h"
#include "stdlib_itertools.h"
#include "stdlib_http.h"
#include "stdlib_struct.h"
#include "stdlib_os.h"

// ANSI Color Codes for Chroma Effects
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_DIM       "\033[2m"

// Fallout Blue/Cyan Theme
#define COLOR_CYAN      "\033[36m"
#define COLOR_BRIGHT_CYAN   "\033[96m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_BRIGHT_BLUE   "\033[94m"

// RGB Chroma Colors
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_WHITE     "\033[97m"

// Cursor Control (for RGB cursor effect)
#define CURSOR_BLOCK_BLINK      "\033[1 q"  // Blinking block cursor
#define CURSOR_BLOCK_STEADY     "\033[2 q"  // Steady block cursor
#define CURSOR_UNDERLINE_BLINK  "\033[3 q"  // Blinking underline
#define CURSOR_UNDERLINE_STEADY "\033[4 q"  // Steady underline
#define CURSOR_BAR_BLINK        "\033[5 q"  // Blinking bar (I-beam)
#define CURSOR_BAR_STEADY       "\033[6 q"  // Steady bar

void print_usage() {
    printf("RADS Programming Language v0.0.11 \"PULSAR\"\n");
    printf("Usage: rads [options] [file]\n\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -t, --tokens   Print tokens (lexer test mode)\n");
    printf("  -i, --interactive  Enter interactive REPL mode\n");
    printf("\nIf no file is provided, RADS will start in interactive REPL mode.\n");
    printf("\n");
}

void print_version() {
    printf("\n");
    printf("%s┌────────────────────────────────────────┐%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s│ %sRADS v0.0.11 \"PULSAR\" 💫%s           %s│%s\n",
            COLOR_BRIGHT_CYAN, COLOR_BRIGHT_MAGENTA,
            COLOR_WHITE, COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s│ %sRapid Asynchronous Data Server%s     %s│%s\n",
            COLOR_BRIGHT_CYAN, COLOR_BRIGHT_BLUE,
            COLOR_WHITE, COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s│ %sBuilt:%s %s %s                %s│%s\n",
            COLOR_BRIGHT_CYAN, COLOR_BRIGHT_YELLOW, COLOR_WHITE,
            __DATE__, __TIME__, COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s└────────────────────────────────────────┘%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("\n");
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'\n", path);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Not enough memory to read '%s'\n", path);
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Error: Could not read file '%s'\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

void test_lexer(const char* source) {
    Lexer lexer;
    lexer_init(&lexer, source);
    
    printf("\n=== LEXER OUTPUT ===\n\n");
    
    for (;;) {
        Token token = lexer_next_token(&lexer);
        token_print(&token);
        
        if (token.type == TOKEN_EOF || token.type == TOKEN_ERROR) {
            break;
        }
    }
    
    printf("\n=== END LEXER OUTPUT ===\n");
}

void print_repl_welcome() {
    printf("\n");

    // Fallout-style RADS ASCII banner with blue/cyan gradient
    printf(COLOR_BRIGHT_CYAN "  ╔═══════════════════════════════════════════════════════════╗\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ║ " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "██████╗  " COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "█████╗ " COLOR_RESET);
    printf(COLOR_CYAN "██████╗ " COLOR_RESET);
    printf(COLOR_BLUE "███████╗" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                       ║\n" COLOR_RESET);

    printf(COLOR_BRIGHT_CYAN "  ║ " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "██╔══██╗" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "██╔══██╗" COLOR_RESET);
    printf(COLOR_CYAN "██╔══██╗" COLOR_RESET);
    printf(COLOR_BLUE "██╔════╝" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                       ║\n" COLOR_RESET);

    printf(COLOR_BRIGHT_CYAN "  ║ " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "██████╔╝" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "███████║" COLOR_RESET);
    printf(COLOR_CYAN "██║  ██║" COLOR_RESET);
    printf(COLOR_BLUE "███████╗" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                       ║\n" COLOR_RESET);

    printf(COLOR_BRIGHT_CYAN "  ║ " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "██╔══██╗" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "██╔══██║" COLOR_RESET);
    printf(COLOR_CYAN "██║  ██║" COLOR_RESET);
    printf(COLOR_BLUE "╚════██║" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                       ║\n" COLOR_RESET);

    printf(COLOR_BRIGHT_CYAN "  ║ " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "██║  ██║" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "██║  ██║" COLOR_RESET);
    printf(COLOR_CYAN "██████╔╝" COLOR_RESET);
    printf(COLOR_BLUE "███████║" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                       ║\n" COLOR_RESET);

    printf(COLOR_BRIGHT_CYAN "  ║ " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "╚═╝  ╚═╝" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "╚═╝  ╚═╝" COLOR_RESET);
    printf(COLOR_CYAN "╚═════╝ " COLOR_RESET);
    printf(COLOR_BLUE "╚══════╝" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                       ║\n" COLOR_RESET);

    printf(COLOR_BRIGHT_CYAN "  ╠═══════════════════════════════════════════════════════════╣\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ║  " COLOR_RESET);
    printf(COLOR_CYAN "🌌 Interactive REPL v0.0.11 PULSAR" COLOR_RESET);
    printf(COLOR_DIM " - " COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "TURBO" COLOR_RESET);
    printf(COLOR_DIM " & " COLOR_RESET);
    printf(COLOR_BRIGHT_BLUE "RADICAL" COLOR_RESET);
    printf(COLOR_DIM "! ✨" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN " ║\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ║  " COLOR_RESET);
    printf(COLOR_BLUE "Rapid Asynchronous Data Server Language" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "               ║\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ╠═══════════════════════════════════════════════════════════╣\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ║  " COLOR_RESET);
    printf(COLOR_BRIGHT_GREEN "✨ NEW:" COLOR_RESET);
    printf(COLOR_WHITE " Bytecode VM ⚡  " COLOR_RESET);
    printf(COLOR_BRIGHT_YELLOW "✨ Profiler 📊  " COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "✨ Debugger 🐛  " COLOR_RESET);
    printf(COLOR_BRIGHT_MAGENTA "✨ WebSocket 🔥  " COLOR_RESET);
    printf(COLOR_BRIGHT_GREEN "✨ GraphQL 📊  " COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "    ║\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ╠═══════════════════════════════════════════════════════════╣\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ║  " COLOR_RESET);
    printf(COLOR_DIM "Type " COLOR_RESET);
    printf(COLOR_BRIGHT_YELLOW ".help" COLOR_RESET);
    printf(COLOR_DIM " for commands, " COLOR_RESET);
    printf(COLOR_BRIGHT_RED ".exit" COLOR_RESET);
    printf(COLOR_DIM " to quit" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "                      ║\n" COLOR_RESET);
    printf(COLOR_BRIGHT_CYAN "  ╚═══════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
}

void print_repl_help() {
    printf("\n");
    printf("%s╔══════════════════════════════════════════╗%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║         %sREPL Commands%s%s              ║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_BRIGHT_YELLOW, COLOR_BRIGHT_CYAN,
           COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s╠══════════════════════════════════════════╣%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║ %s.help%s     Show this help message      %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_BRIGHT_GREEN, COLOR_WHITE,
           COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║ %s.exit%s     Exit the REPL               %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_BRIGHT_RED, COLOR_WHITE,
           COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║ %s.clear%s    Clear the screen            %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_BRIGHT_BLUE, COLOR_WHITE,
           COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║ %s.version%s  Show version information    %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_BRIGHT_MAGENTA, COLOR_WHITE,
           COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s╠══════════════════════════════════════════╣%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║         %sTry it out:%s                  %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_BRIGHT_YELLOW, COLOR_BRIGHT_CYAN,
           COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║%s  echo(\"Hello RADS!\");              %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_WHITE, COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║%s  turbo x = 42;                      %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_WHITE, COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s║%s  echo(x);                           %s║%s\n",
           COLOR_BRIGHT_CYAN, COLOR_WHITE, COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════╝%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("\n");
}

int run_repl() {
    print_repl_welcome();

    // Initialize standard library
    stdlib_io_register();
    stdlib_media_register();
    stdlib_net_register();
    stdlib_ffi_register();
    stdlib_string_register();
    stdlib_string_advanced_register();
    stdlib_math_register();
    stdlib_math_extended_register();
    stdlib_array_register();
    stdlib_fs_register();
    stdlib_json_register();
    stdlib_db_register();
    stdlib_webengine_register();
    stdlib_datetime_register();
    stdlib_regex_register();
    stdlib_test_register();
    stdlib_env_register();
    stdlib_cli_register();
    stdlib_csv_register();
    stdlib_base64_register();
    stdlib_random_register();
    stdlib_time_register();
    stdlib_path_register();
    stdlib_hash_register();
    stdlib_sort_register();
    stdlib_http_register();
    stdlib_async_utils_register();
    stdlib_websocket_register();
    stdlib_graphql_register();
    stdlib_os_register();
    stdlib_struct_register();
    stdlib_process_register();
    stdlib_uuid_register();
    stdlib_collections_register();
    stdlib_itertools_register();
    int line_num = 1;

    // RGB Chroma color cycle for prompt (like RGB keyboard)
    const char* chroma_colors[] = {
        COLOR_BRIGHT_MAGENTA,
        COLOR_BRIGHT_BLUE,
        COLOR_BRIGHT_CYAN,
        COLOR_BRIGHT_GREEN,
        COLOR_BRIGHT_YELLOW,
        COLOR_BRIGHT_RED
    };
    int chroma_cycle_size = 6;

    // Set cursor to blinking block for better RGB visibility
    printf(CURSOR_BLOCK_BLINK);
    fflush(stdout);

    while (1) {
        // Create RGB Chroma prompt (cycles through rainbow)
        const char* prompt_color = chroma_colors[line_num % chroma_cycle_size];

        // Input text color cycles faster for RGB cursor effect
        const char* input_color = chroma_colors[(line_num * 3) % chroma_cycle_size];

        char prompt[128];
        // Prompt in one color, input text (and cursor) in another cycling color
        snprintf(prompt, sizeof(prompt), "%srads[%d]>%s %s",
                 prompt_color, line_num, COLOR_RESET, input_color);

        // Read line with readline (supports arrow keys, history, editing)
        char* line = readline(prompt);

        // Reset color after input
        printf(COLOR_RESET);
        fflush(stdout);

        // Check for EOF (Ctrl+D)
        if (line == NULL) {
            printf("\n%s👋 Goodbye! Stay %sTURBO%s, stay %sRADICAL%s! 🚀%s\n",
                   COLOR_BRIGHT_CYAN, COLOR_BRIGHT_YELLOW, COLOR_BRIGHT_CYAN,
                   COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN, COLOR_RESET);
            break;
        }

        // Get line length
        size_t len = strlen(line);

        // Skip empty lines
        if (len == 0) {
            free(line);
            continue;
        }

        // Add non-empty lines to history
        add_history(line);

        // Handle REPL commands
        if (line[0] == '.') {
            if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
                printf("%s👋 Goodbye! Stay %sTURBO%s, stay %sRADICAL%s! 🚀%s\n",
                       COLOR_BRIGHT_CYAN, COLOR_BRIGHT_YELLOW, COLOR_BRIGHT_CYAN,
                       COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN, COLOR_RESET);
                free(line);
                break;
            } else if (strcmp(line, ".help") == 0) {
                print_repl_help();
                free(line);
                continue;
            } else if (strcmp(line, ".clear") == 0) {
                printf("\033[2J\033[H");  // Clear screen
                print_repl_welcome();
                free(line);
                continue;
            } else if (strcmp(line, ".version") == 0) {
                print_version();
                free(line);
                continue;
            } else {
                printf("%s⚠ Unknown command:%s %s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET, line);
                printf("%sType %s.help%s for available commands%s\n",
                       COLOR_DIM, COLOR_BRIGHT_CYAN, COLOR_DIM, COLOR_RESET);
                free(line);
                continue;
            }
        }

        // Add semicolon if missing
        char* source;
        if (len > 0 && line[len-1] != ';' && line[len-1] != '}') {
            source = malloc(len + 2);
            snprintf(source, len + 2, "%s;", line);
        } else {
            source = strdup(line);
        }

        // Free the readline buffer
        free(line);

        // Tokenize
        Lexer lexer;
        lexer_init(&lexer, source);

        // Parse
        Parser parser;
        parser_init(&parser, &lexer);
        ASTNode* stmt = parse_statement(&parser);

        if (stmt == NULL || parser.had_error) {
            // Error already printed by parser
            free(source);
            line_num++;
            continue;
        }

        // Execute statement in REPL context (preserves environment)
        interpret_repl_statement(stmt);

        // Cleanup AST and source (environment persists)
        ast_free(stmt);
        free(source);

        line_num++;
    }

    // Reset cursor to default on exit
    printf(CURSOR_BAR_BLINK);  // Reset to normal I-beam cursor
    printf(COLOR_RESET);
    fflush(stdout);

    // Clean up environment and event loop on exit
    interpreter_cleanup_environment();
    interpreter_cleanup_event_loop();
    return 0;
}

int main(int argc, char* argv[]) {
    // If no arguments, start REPL
    if (argc < 2) {
        return run_repl();
    }
    
    bool token_mode = false;
    const char* filename = NULL;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tokens") == 0) {
            token_mode = true;
        } else {
            filename = argv[i];
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage();
        return 1;
    }
    
    // Read source file
    char* source = read_file(filename);
    if (source == NULL) {
        return 1;
    }
    
    // Lexer test mode
    if (token_mode) {
        test_lexer(source);
        free(source);
        return 0;
    }
    
    // Normal execution mode
    printf("%s🌌 RADS v0.0.11 PULSAR%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("%sExecuting:%s %s%s%s\n\n",
           COLOR_BRIGHT_YELLOW, COLOR_RESET,
           COLOR_BRIGHT_MAGENTA, filename, COLOR_RESET);
    stdlib_io_register();
    stdlib_media_register();
    stdlib_net_register();
    stdlib_ffi_register();
    stdlib_string_register();
    stdlib_string_advanced_register();
    stdlib_math_register();
    stdlib_math_extended_register();
    stdlib_array_register();
    stdlib_fs_register();
    stdlib_json_register();
    stdlib_db_register();
    stdlib_webengine_register();
    stdlib_async_utils_register();
    stdlib_datetime_register();
    stdlib_regex_register();
    stdlib_test_register();
    stdlib_env_register();
    stdlib_cli_register();
    stdlib_csv_register();
    stdlib_base64_register();
    stdlib_random_register();
    stdlib_time_register();
    stdlib_path_register();
    stdlib_hash_register();
    stdlib_sort_register();
    stdlib_http_register();
    stdlib_websocket_register();
    stdlib_graphql_register();
    stdlib_os_register();
    stdlib_struct_register();
    stdlib_process_register();
    stdlib_uuid_register();
    stdlib_collections_register();
    stdlib_itertools_register();

    // Tokenize
    Lexer lexer;
    lexer_init(&lexer, source);
    
    // Parse
    Parser parser;
    parser_init(&parser, &lexer);
    ASTNode* program = parser_parse(&parser);
    
    if (program == NULL) {
        fprintf(stderr, "\n❌ Compilation failed\n");
        free(source);
        return 1;
    }
    
    // Interpret
    int result = interpret(program);
    
    // Cleanup
    ast_free(program);
    free(source);
    
    return result;
}
