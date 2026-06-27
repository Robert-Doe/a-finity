/* sema.c — Semantic analysis implementation for bob_compiler
 * Checks: undefined variables/functions, duplicate declarations, wrong arg counts.
 */
#include "sema.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    SymTab globals;
    SymTab locals;
    int    errors;
} SemaCtx;

static void serr(SemaCtx *ctx, int line, const char *msg, const char *name) {
    fprintf(stderr, "sema error at line %d: %s '%s'\n", line, msg, name);
    ctx->errors++;
}

static void check_expr(SemaCtx *ctx, const Node *n) {
    if (!n) return;
    switch (n->kind) {
        case AST_INT_LIT: break;
        case AST_IDENT:
            if (!symtab_lookup(&ctx->locals, n->sval))
                serr(ctx, n->line, "undefined variable", n->sval);
            break;
        case AST_UNARY:
            check_expr(ctx, n->left);
            break;
        case AST_BINARY:
            check_expr(ctx, n->left);
            check_expr(ctx, n->right);
            break;
        case AST_ASSIGN:
            if (!symtab_lookup(&ctx->locals, n->left->sval))
                serr(ctx, n->line, "undefined variable", n->left->sval);
            check_expr(ctx, n->right);
            break;
        case AST_CALL: {
            Symbol *sym = symtab_lookup(&ctx->globals, n->sval);
            if (!sym) {
                serr(ctx, n->line, "undefined function", n->sval);
            } else if (sym->n_params != n->nargs) {
                fprintf(stderr,
                    "sema error at line %d: function '%s' expects %d args, got %d\n",
                    n->line, n->sval, sym->n_params, n->nargs);
                ctx->errors++;
            }
            for (int i = 0; i < n->nargs; i++) check_expr(ctx, n->args[i]);
            break;
        }
        default: break;
    }
}

static void check_stmt(SemaCtx *ctx, const Node *n) {
    if (!n) return;
    switch (n->kind) {
        case AST_VAR_DECL:
            if (symtab_lookup(&ctx->locals, n->sval))
                serr(ctx, n->line, "duplicate variable", n->sval);
            else
                symtab_insert(&ctx->locals, n->sval, 0, 0);
            if (n->left) check_expr(ctx, n->left);
            break;
        case AST_RETURN:
            check_expr(ctx, n->left);
            break;
        case AST_IF:
            check_expr(ctx, n->left);
            check_stmt(ctx, n->right);
            check_stmt(ctx, n->extra);
            break;
        case AST_WHILE:
            check_expr(ctx, n->left);
            check_stmt(ctx, n->right);
            break;
        case AST_BLOCK:
            for (int i = 0; i < n->nargs; i++) check_stmt(ctx, n->args[i]);
            break;
        case AST_EXPR_STMT:
            check_expr(ctx, n->left);
            break;
        default: break;
    }
}

static void check_func(SemaCtx *ctx, const Node *fn) {
    symtab_init(&ctx->locals);
    for (int i = 0; i < fn->nargs; i++)
        symtab_insert(&ctx->locals, fn->args[i]->sval, 0, 0);
    check_stmt(ctx, fn->extra);
}

int sema_check(Node *prog) {
    SemaCtx ctx;
    ctx.errors = 0;
    symtab_init(&ctx.globals);
    symtab_init(&ctx.locals);

    for (int i = 0; i < prog->nargs; i++) {
        const Node *fn = prog->args[i];
        if (symtab_lookup(&ctx.globals, fn->sval)) {
            fprintf(stderr, "sema error: duplicate function '%s'\n", fn->sval);
            ctx.errors++;
        } else {
            symtab_insert(&ctx.globals, fn->sval, 1, fn->nargs);
        }
    }
    for (int i = 0; i < prog->nargs; i++) check_func(&ctx, prog->args[i]);
    return ctx.errors;
}
