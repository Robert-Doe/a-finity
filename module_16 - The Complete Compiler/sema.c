/*
 * sema.c — Semantic analysis for mycc
 * Module 16: The Complete Compiler
 *
 * Checks:
 *   - All referenced variables are declared
 *   - Variables are not declared twice in the same scope
 *   - Called functions are declared
 *   - Argument counts match parameter counts
 */
#include "sema.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sema_ctx_init(SemaCtx *ctx)
{
    symtab_init(&ctx->symtab);
    ctx->errors       = 0;
    ctx->current_func = NULL;
}

void sema_ctx_free(SemaCtx *ctx)
{
    symtab_free(&ctx->symtab);
    free(ctx->current_func);
}

static void sema_error(SemaCtx *ctx, int line, const char *msg)
{
    fprintf(stderr, "sema: line %d: %s\n", line, msg);
    ctx->errors++;
}

static void sema_expr(SemaCtx *ctx, ASTNode *node);
static void sema_stmt(SemaCtx *ctx, ASTNode *node);

static void sema_expr(SemaCtx *ctx, ASTNode *node)
{
    if (!node) return;
    char buf[256];
    switch (node->kind) {
        case AST_INT_LIT:
            break;
        case AST_IDENT:
            if (!symtab_lookup(&ctx->symtab, node->u.name)) {
                snprintf(buf, sizeof buf, "undeclared variable '%s'", node->u.name);
                sema_error(ctx, node->line, buf);
            }
            break;
        case AST_BINOP:
        case AST_UNOP:
            sema_expr(ctx, node->u.binop.left);
            sema_expr(ctx, node->u.binop.right);
            break;
        case AST_ASSIGN:
            if (!symtab_lookup(&ctx->symtab, node->u.assign.name)) {
                snprintf(buf, sizeof buf, "undeclared variable '%s'", node->u.assign.name);
                sema_error(ctx, node->line, buf);
            }
            sema_expr(ctx, node->u.assign.value);
            break;
        case AST_CALL: {
            Symbol *sym = symtab_lookup(&ctx->symtab, node->u.call.name);
            if (!sym) {
                snprintf(buf, sizeof buf, "undeclared function '%s'", node->u.call.name);
                sema_error(ctx, node->line, buf);
            } else if (sym->kind != SYM_FUNC) {
                snprintf(buf, sizeof buf, "'%s' is not a function", node->u.call.name);
                sema_error(ctx, node->line, buf);
            } else if (sym->param_count != node->u.call.argc) {
                snprintf(buf, sizeof buf,
                         "function '%s' expects %d args, got %d",
                         node->u.call.name, sym->param_count, node->u.call.argc);
                sema_error(ctx, node->line, buf);
            }
            for (int i = 0; i < node->u.call.argc; i++)
                sema_expr(ctx, node->u.call.args[i]);
            break;
        }
        default:
            break;
    }
}

static void sema_stmt(SemaCtx *ctx, ASTNode *node)
{
    if (!node) return;
    char buf[256];
    switch (node->kind) {
        case AST_VAR_DECL:
            if (!symtab_insert(&ctx->symtab, node->u.name, SYM_VAR)) {
                snprintf(buf, sizeof buf, "redeclaration of '%s'", node->u.name);
                sema_error(ctx, node->line, buf);
            }
            break;
        case AST_RETURN:
            sema_expr(ctx, node->u.expr);
            break;
        case AST_PRINT:
            sema_expr(ctx, node->u.expr);
            break;
        case AST_EXPR_STMT:
            sema_expr(ctx, node->u.expr);
            break;
        case AST_IF:
            sema_expr(ctx, node->u.if_stmt.cond);
            sema_stmt(ctx, node->u.if_stmt.then_branch);
            sema_stmt(ctx, node->u.if_stmt.else_branch);
            break;
        case AST_WHILE:
            sema_expr(ctx, node->u.while_stmt.cond);
            sema_stmt(ctx, node->u.while_stmt.body);
            break;
        case AST_BLOCK:
            symtab_push_scope(&ctx->symtab);
            for (int i = 0; i < node->u.block.count; i++)
                sema_stmt(ctx, node->u.block.stmts[i]);
            symtab_pop_scope(&ctx->symtab);
            break;
        default:
            sema_expr(ctx, node);
            break;
    }
}

int sema_check(SemaCtx *ctx, ASTNode *program)
{
    if (!program || program->kind != AST_PROGRAM) return 1;

    /* First pass: register all function names so forward calls work */
    for (int i = 0; i < program->u.program.count; i++) {
        ASTNode *fn = program->u.program.funcs[i];
        Symbol *sym = symtab_insert(&ctx->symtab, fn->u.func.name, SYM_FUNC);
        if (!sym) {
            char buf[128];
            snprintf(buf, sizeof buf, "redefinition of function '%s'", fn->u.func.name);
            sema_error(ctx, fn->line, buf);
        } else {
            sym->param_count = fn->u.func.param_count;
        }
    }

    /* Second pass: check bodies */
    for (int i = 0; i < program->u.program.count; i++) {
        ASTNode *fn = program->u.program.funcs[i];
        symtab_push_scope(&ctx->symtab);

        /* Insert parameters */
        for (int j = 0; j < fn->u.func.param_count; j++)
            symtab_insert(&ctx->symtab, fn->u.func.params[j], SYM_VAR);

        /* Check body statements (body is a BLOCK, so push/pop is inside sema_stmt) */
        /* We need to check block contents directly here without extra scope push */
        ASTNode *body = fn->u.func.body;
        if (body && body->kind == AST_BLOCK) {
            for (int k = 0; k < body->u.block.count; k++)
                sema_stmt(ctx, body->u.block.stmts[k]);
        } else {
            sema_stmt(ctx, body);
        }

        symtab_pop_scope(&ctx->symtab);
    }

    return ctx->errors;
}
