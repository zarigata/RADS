/**
 * RADS Masquerading - RADS Code Generator Implementation
 *
 * Converts language-agnostic AST to beautiful RADS source code.
 *
 * Version: 0.0.5 "CHAMELEON"
 */

#include "rads_codegen.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Helper Functions
// ============================================================================

static void emit_indent(RADSCodegenContext* ctx) {
    for (int i = 0; i < ctx->indent_level; i++) {
        fprintf(ctx->output, "    ");
    }
}

static void emit(RADSCodegenContext* ctx, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->output, fmt, args);
    va_end(args);
}

static void emit_line(RADSCodegenContext* ctx, const char* fmt, ...) {
    emit_indent(ctx);
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->output, fmt, args);
    va_end(args);
    fprintf(ctx->output, "\n");
    ctx->line_number++;
}

static const char* operator_to_string(MaskOperator op) {
    switch (op) {
        case MASK_OP_ADD: return "+";
        case MASK_OP_SUB: return "-";
        case MASK_OP_MUL: return "*";
        case MASK_OP_DIV: return "/";
        case MASK_OP_MOD: return "%";
        case MASK_OP_POW: return "**";
        case MASK_OP_EQ: return "==";
        case MASK_OP_NE: return "!=";
        case MASK_OP_LT: return "<";
        case MASK_OP_LE: return "<=";
        case MASK_OP_GT: return ">";
        case MASK_OP_GE: return ">=";
        case MASK_OP_AND: return "&&";
        case MASK_OP_OR: return "||";
        case MASK_OP_NOT: return "!";
        case MASK_OP_NEGATE: return "-";
        case MASK_OP_POSITIVE: return "+";
        default: return "?";
    }
}

// ============================================================================
// Initialization and Cleanup
// ============================================================================

RADSCodegenContext* rads_codegen_init(FILE* output) {
    RADSCodegenContext* ctx = (RADSCodegenContext*)malloc(sizeof(RADSCodegenContext));
    if (!ctx) {
        return NULL;
    }

    ctx->output = output;
    ctx->indent_level = 0;
    ctx->line_number = 1;

    return ctx;
}

void rads_codegen_free(RADSCodegenContext* ctx) {
    if (ctx) {
        free(ctx);
    }
}

// ============================================================================
// Forward Declarations
// ============================================================================

static int rads_codegen_node_impl(RADSCodegenContext* ctx, MaskASTNode* node, int emit_semicolon);
static int rads_codegen_expression(RADSCodegenContext* ctx, MaskASTNode* node);
static int rads_codegen_statement(RADSCodegenContext* ctx, MaskASTNode* node);

// ============================================================================
// List Code Generation
// ============================================================================

int rads_codegen_list(RADSCodegenContext* ctx, MaskASTList* list, const char* separator) {
    if (!list) return 0;

    MaskASTList* current = list;
    while (current) {
        if (rads_codegen_expression(ctx, current->node) != 0) {
            return -1;
        }
        if (current->next && separator) {
            emit(ctx, "%s ", separator);
        }
        current = current->next;
    }

    return 0;
}

// ============================================================================
// Expression Code Generation
// ============================================================================

static int rads_codegen_expression(RADSCodegenContext* ctx, MaskASTNode* node) {
    if (!node) return 0;

    switch (node->type) {
        case MASK_NODE_INT_LITERAL:
            emit(ctx, "%lld", node->data.int_literal.int_value);
            break;

        case MASK_NODE_FLOAT_LITERAL:
            emit(ctx, "%.15g", node->data.float_literal.float_value);
            break;

        case MASK_NODE_STRING_LITERAL:
            emit(ctx, "\"%s\"", node->data.string_literal.string_value);
            break;

        case MASK_NODE_BOOL_LITERAL:
            emit(ctx, "%s", node->data.bool_literal.bool_value ? "true" : "false");
            break;

        case MASK_NODE_NULL_LITERAL:
            emit(ctx, "null");
            break;

        case MASK_NODE_IDENTIFIER:
            emit(ctx, "%s", node->data.identifier.name);
            break;

        case MASK_NODE_BINARY_OP:
            emit(ctx, "(");
            rads_codegen_expression(ctx, node->data.binary_op.left);
            emit(ctx, " %s ", operator_to_string(node->data.binary_op.op));
            rads_codegen_expression(ctx, node->data.binary_op.right);
            emit(ctx, ")");
            break;

        case MASK_NODE_UNARY_OP:
            emit(ctx, "%s", operator_to_string(node->data.unary_op.op));
            rads_codegen_expression(ctx, node->data.unary_op.operand);
            break;

        case MASK_NODE_FUNCTION_CALL:
            rads_codegen_expression(ctx, node->data.function_call.function);
            emit(ctx, "(");
            if (node->data.function_call.args) {
                if (node->data.function_call.args->type == MASK_NODE_ARG_LIST ||
                    node->data.function_call.args->type == MASK_NODE_PARAM_LIST) {
                    rads_codegen_list(ctx, node->data.function_call.args->data.block.statements, ",");
                } else {
                    rads_codegen_expression(ctx, node->data.function_call.args);
                }
            }
            emit(ctx, ")");
            break;

        case MASK_NODE_ARRAY_LITERAL:
            emit(ctx, "[");
            rads_codegen_list(ctx, node->data.array_literal.elements, ",");
            emit(ctx, "]");
            break;

        case MASK_NODE_MEMBER_ACCESS:
            rads_codegen_expression(ctx, node->data.member_access.object);
            emit(ctx, ".%s", node->data.member_access.member);
            break;

        case MASK_NODE_INDEX_ACCESS:
            rads_codegen_expression(ctx, node->data.index_access.object);
            emit(ctx, "[");
            rads_codegen_expression(ctx, node->data.index_access.index);
            emit(ctx, "]");
            break;

        default:
            fprintf(stderr, "Error: Unexpected node type %d in expression\n", node->type);
            return -1;
    }

    return 0;
}

// ============================================================================
// Statement Code Generation
// ============================================================================

static int rads_codegen_statement(RADSCodegenContext* ctx, MaskASTNode* node) {
    if (!node) return 0;

    switch (node->type) {
        case MASK_NODE_VAR_DECL:
            emit_indent(ctx);
            emit(ctx, "turbo %s", node->data.var_decl.name);
            if (node->data.var_decl.init_value) {
                emit(ctx, " = ");
                rads_codegen_expression(ctx, node->data.var_decl.init_value);
            }
            emit(ctx, ";\n");
            ctx->line_number++;
            break;

        case MASK_NODE_ASSIGN:
            emit_indent(ctx);
            rads_codegen_expression(ctx, node->data.assign.target);
            emit(ctx, " = ");
            rads_codegen_expression(ctx, node->data.assign.value);
            emit(ctx, ";\n");
            ctx->line_number++;
            break;

        case MASK_NODE_IF_STMT:
            emit_indent(ctx);
            emit(ctx, "if (");
            rads_codegen_expression(ctx, node->data.if_stmt.condition);
            emit(ctx, ") {\n");
            ctx->line_number++;
            ctx->indent_level++;

            if (node->data.if_stmt.then_block) {
                rads_codegen_statement(ctx, node->data.if_stmt.then_block);
            }

            ctx->indent_level--;
            emit_line(ctx, "}");

            if (node->data.if_stmt.else_block) {
                emit_indent(ctx);
                emit(ctx, "else {\n");
                ctx->line_number++;
                ctx->indent_level++;

                rads_codegen_statement(ctx, node->data.if_stmt.else_block);

                ctx->indent_level--;
                emit_line(ctx, "}");
            }
            break;

        case MASK_NODE_WHILE_LOOP:
            emit_indent(ctx);
            emit(ctx, "loop (");
            rads_codegen_expression(ctx, node->data.while_loop.condition);
            emit(ctx, ") {\n");
            ctx->line_number++;
            ctx->indent_level++;

            if (node->data.while_loop.body) {
                rads_codegen_statement(ctx, node->data.while_loop.body);
            }

            ctx->indent_level--;
            emit_line(ctx, "}");
            break;

        case MASK_NODE_FOR_LOOP:
            // RADS doesn't have traditional for loops, convert to while
            if (node->data.for_loop.init) {
                rads_codegen_statement(ctx, node->data.for_loop.init);
            }

            emit_indent(ctx);
            emit(ctx, "loop (");
            rads_codegen_expression(ctx, node->data.for_loop.condition);
            emit(ctx, ") {\n");
            ctx->line_number++;
            ctx->indent_level++;

            if (node->data.for_loop.body) {
                rads_codegen_statement(ctx, node->data.for_loop.body);
            }

            if (node->data.for_loop.increment) {
                rads_codegen_statement(ctx, node->data.for_loop.increment);
            }

            ctx->indent_level--;
            emit_line(ctx, "}");
            break;

        case MASK_NODE_RETURN:
            emit_indent(ctx);
            emit(ctx, "return");
            if (node->data.return_stmt.value) {
                emit(ctx, " ");
                rads_codegen_expression(ctx, node->data.return_stmt.value);
            }
            emit(ctx, ";\n");
            ctx->line_number++;
            break;

        case MASK_NODE_BREAK:
            emit_line(ctx, "break;");
            break;

        case MASK_NODE_CONTINUE:
            emit_line(ctx, "continue;");
            break;

        case MASK_NODE_FUNCTION_DEF:
            emit_indent(ctx);
            emit(ctx, "blast %s(", node->data.function_def.name);

            if (node->data.function_def.params) {
                if (node->data.function_def.params->type == MASK_NODE_PARAM_LIST) {
                    rads_codegen_list(ctx, node->data.function_def.params->data.block.statements, ",");
                } else {
                    rads_codegen_expression(ctx, node->data.function_def.params);
                }
            }

            emit(ctx, ") {\n");
            ctx->line_number++;
            ctx->indent_level++;

            if (node->data.function_def.body) {
                rads_codegen_statement(ctx, node->data.function_def.body);
            }

            ctx->indent_level--;
            emit_line(ctx, "}");
            emit(ctx, "\n");
            ctx->line_number++;
            break;

        case MASK_NODE_EXPR_STMT:
            emit_indent(ctx);
            rads_codegen_expression(ctx, node->data.expr_stmt.expr);
            emit(ctx, ";\n");
            ctx->line_number++;
            break;

        case MASK_NODE_IMPORT:
            emit_line(ctx, "import \"%s\";", node->data.import.module_name);
            break;

        case MASK_NODE_BLOCK:
            if (node->data.block.statements) {
                MaskASTList* current = node->data.block.statements;
                while (current) {
                    rads_codegen_statement(ctx, current->node);
                    current = current->next;
                }
            }
            break;

        case MASK_NODE_PROGRAM:
            emit(ctx, "// Auto-generated by rads-mask v0.0.5 \"CHAMELEON\"\n");
            emit(ctx, "// Generated from source language transpilation\n\n");
            ctx->line_number += 3;

            if (node->data.program.statements) {
                MaskASTList* current = node->data.program.statements;
                while (current) {
                    rads_codegen_statement(ctx, current->node);
                    current = current->next;
                }
            }
            break;

        default:
            // Try as expression statement
            emit_indent(ctx);
            if (rads_codegen_expression(ctx, node) != 0) {
                return -1;
            }
            emit(ctx, ";\n");
            ctx->line_number++;
            break;
    }

    return 0;
}

// ============================================================================
// Public API Implementation
// ============================================================================

int rads_codegen_node(RADSCodegenContext* ctx, MaskASTNode* node) {
    return rads_codegen_node_impl(ctx, node, 1);
}

static int rads_codegen_node_impl(RADSCodegenContext* ctx, MaskASTNode* node, int emit_semicolon) {
    return rads_codegen_statement(ctx, node);
}

int rads_codegen_generate(RADSCodegenContext* ctx, MaskASTNode* ast) {
    if (!ctx || !ast) {
        return -1;
    }

    return rads_codegen_statement(ctx, ast);
}
