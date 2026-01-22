/**
 * RADS Masquerading - RADS Code Generator
 *
 * Converts language-agnostic AST to RADS source code.
 *
 * Version: 0.0.5 "CHAMELEON"
 */

#ifndef RADS_MASK_CODEGEN_H
#define RADS_MASK_CODEGEN_H

#include "../ast.h"
#include <stdio.h>

// ============================================================================
// Code Generation Context
// ============================================================================

typedef struct {
    FILE* output;           // Output stream
    int indent_level;       // Current indentation level
    int line_number;        // Current line number in output
} RADSCodegenContext;

// ============================================================================
// API Functions
// ============================================================================

/**
 * Initialize a code generation context
 */
RADSCodegenContext* rads_codegen_init(FILE* output);

/**
 * Generate RADS code from AST
 * Returns 0 on success, -1 on error
 */
int rads_codegen_generate(RADSCodegenContext* ctx, MaskASTNode* ast);

/**
 * Free code generation context
 */
void rads_codegen_free(RADSCodegenContext* ctx);

/**
 * Generate RADS code for a single node
 */
int rads_codegen_node(RADSCodegenContext* ctx, MaskASTNode* node);

/**
 * Generate RADS code for a list of nodes
 */
int rads_codegen_list(RADSCodegenContext* ctx, MaskASTList* list, const char* separator);

#endif // RADS_MASK_CODEGEN_H
