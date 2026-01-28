#include "../core/interpreter.h"
#include <stdio.h>

// ============================================================================
// FILESYSTEM OPERATIONS - readdir, stat, unlink, path manipulation
// RADS v0.0.7 "DARK MOON"
// ============================================================================

void stdlib_fs_readdir(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: fs.readdir() requires 1 argument (directory)\n");
        return;
    }

    // TODO: Read directory and return array of files
    printf("✓ fs.readdir() called\n");
}

void stdlib_fs_stat(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: fs.stat() requires 1 argument (path)\n");
        return;
    }

    // TODO: Get file status (size, type, permissions)
    printf("✓ fs.stat() called\n");
}

void stdlib_fs_unlink(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: fs.unlink() requires 1 argument (path)\n");
        return;
    }

    // TODO: Delete file
    printf("✓ fs.unlink() called\n");
}
