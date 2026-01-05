#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "stdlib_io.h"
#include "stdlib_media.h"
#include "stdlib_net.h"
#include "stdlib_ffi.h"
#include "stdlib_string.h"
#include "stdlib_math.h"
#include "stdlib_fs.h"
#include "stdlib_json.h"

void print_usage() {
    printf("RADS Programming Language v0.1.0-alpha\n");
    printf("Usage: rads [options] <file>\n\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -t, --tokens   Print tokens (lexer test mode)\n");
    printf("\n");
}

void print_version() {
    printf("RADS v0.1.0-alpha\n");
    printf("Rapid Asynchronous Data Server Language\n");
    printf("Built: %s %s\n", __DATE__, __TIME__);
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
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
    printf("🚀 RADS v0.1.0-alpha\n");
    printf("Executing: %s\n\n", filename);
    
    // Initialize standard library
    stdlib_io_register();
    stdlib_media_register();
    stdlib_net_register();
    stdlib_ffi_register();
    stdlib_string_register();
    stdlib_math_register();
    stdlib_fs_register();
    stdlib_json_register();
    
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
