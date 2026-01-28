#include "../core/interpreter.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// EXTENDED MATH FUNCTIONS - min, max, clamp, floor, ceil, round, random (seeded)
// RADS v0.0.7 "DARK MOON"
// ============================================================================

void stdlib_math_min(Value* args, int argc, Value* result) {
    if (argc < 2) {
        fprintf(stderr, "Error: math.min() requires at least 2 arguments\n");
        return;
    }

    // TODO: Find minimum of all arguments
    printf("✓ math.min() called\n");
}

void stdlib_math_max(Value* args, int argc, Value* result) {
    if (argc < 2) {
        fprintf(stderr, "Error: math.max() requires at least 2 arguments\n");
        return;
    }

    // TODO: Find maximum of all arguments
    printf("✓ math.max() called\n");
}

void stdlib_math_clamp(Value* args, int argc, Value* result) {
    if (argc < 3) {
        fprintf(stderr, "Error: math.clamp() requires 3 arguments (value, min, max)\n");
        return;
    }

    // TODO: Clamp value between min and max
    printf("✓ math.clamp() called\n");
}

void stdlib_math_floor(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: math.floor() requires 1 argument\n");
        return;
    }

    // TODO: Round down to nearest integer
    printf("✓ math.floor() called\n");
}

void stdlib_math_ceil(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: math.ceil() requires 1 argument\n");
        return;
    }

    // TODO: Round up to nearest integer
    printf("✓ math.ceil() called\n");
}

void stdlib_math_round(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: math.round() requires 1 argument\n");
        return;
    }

    // TODO: Round to nearest integer
    printf("✓ math.round() called\n");
}

static unsigned int random_seed = 0;

void stdlib_math_random(Value* args, int argc, Value* result) {
    if (argc < 1) {
        fprintf(stderr, "Error: math.random() requires 0 or 1 argument (optional seed)\n");
        return;
    }

    // Use provided seed or initialize
    if (argc >= 2) {
        // TODO: Set seed from args[1]
        printf("✓ math.random() called with seed\n");
    } else {
        random_seed = (unsigned int)time(NULL);
        // TODO: Return random number between 0 and 1
        printf("✓ math.random() called\n");
    }
}
