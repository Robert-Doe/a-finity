/*
 * sema.c — Module 07: Semantic Analysis implementation
 *
 * Two-pass walk of the AST:
 *   1. Collect every function name + parameter count.
 *   2. Walk each function body, maintaining a variable scope stack,
 *      and emit errors for undeclared variables, bad call-sites, etc.
 */

#include "sema.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Error helpers                                                       */
/* ------------------------------------------------------------------ */

/* SEMA_ERR prints an error to stderr and increments the error count.
 * We use a macro so the format string can be a variadic literal.    */
#define SEMA_ERR(ctx, line, ...) \
    do { \
        fprintf(stderr, "sema error: " __VA_ARGS__); \
        fprintf(stderr, " (line %d)\n", (line)); \
        (ctx)->errors++; \
    } while (0)

/* ------------------------------------------------------------------ */
/* Pass 1 — collect function definitions                              */
/* ------------------------------------------------------------------ */

/* Grow the function table if needed and add one entry.              */
static void register_func(SemaCtx *ctx, const char *name, int nparams)
{
    /* Check for duplicate function name.                            */
    for (int i = 0; i < ctx->nfuncs; i++) {
        if (strcmp(ctx->funcs[i].name, name) == 0) {
            /* We still report but don't add a second entry.        */
            fprintf(stderr,
                    "sema error: function '%s' defined more than once\n",
                    name);
            ctx->errors++;
            return;
        }
    }

    if (ctx->nfuncs == ctx->funcs_cap) {
        ctx->funcs_cap = ctx->funcs_cap ? ctx->funcs_cap * 2 : 8;
        ctx->funcs = realloc(ctx->funcs,
                             sizeof(FuncEntry) * (size_t)ctx->funcs_cap);
        if (!ctx->funcs) {
            fprintf(stderr, "sema: out of memory\n");
            exit(1);
        }
    }
    FuncEntry *e = &ctx->funcs[ctx->nfuncs++];
    e->name    = strdup(name);
    e->nparams = nparams;
    if (!e->name) { fprintf(stderr, "sema: out of memory\n"); exit(1); }
}

/* Look up a function by name; returns NULL if not found.            */
static FuncEntry *lookup_func(SemaCtx *ctx, const char *name)
{
    for (int i = 0; i < ctx->nfuncs; i++) {
        if (strcmp(ctx->funcs[i].name, name) == 0)
            return &ctx->funcs[i];
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Pass 2 — walk the AST                                              */
/* ------------------------------------------------------------------ */

/* Forward declaration; check_node calls check_expr and check_stmt
 * which may call back into check_node.                              */
static void check_node(SemaCtx *ctx, SymTab *st, Node *n);

/* Walk a list of nodes (args array).                                */
static void check_args(SemaCtx *ctx, SymTab *st, Node **args, int nargs)
{
    for (int i = 0; i < nargs; i++)
        check_node(ctx, st, args[i]);
}

static void check_node(SemaCtx *ctx, SymTab *st, Node *n)
{
    if (!n) return;

    switch (n->kind) {

    /* ---- Literals ------------------------------------------------ */
    case AST_INT_LIT:
        /* Always valid; nothing to check.                           */
        break;

    /* ---- Identifier reference ----------------------------------- */
    case AST_IDENT:
        /*
         * Every bare identifier (not on the left of an assignment)
         * must name a variable that was declared in the current or an
         * enclosing scope.
         */
        if (!symtab_lookup(st, n->sval)) {
            SEMA_ERR(ctx, n->line,
                     "'%s' used before declaration", n->sval);
        }
        break;

    /* ---- Unary expression --------------------------------------- */
    case AST_UNARY:
        check_node(ctx, st, n->left);
        break;

    /* ---- Binary expression -------------------------------------- */
    case AST_BINARY:
        check_node(ctx, st, n->left);
        check_node(ctx, st, n->right);
        break;

    /* ---- Function call ------------------------------------------ */
    case AST_CALL: {
        /*
         * First validate the callee name against the function table
         * collected in pass 1, then check each argument expression.
         */
        FuncEntry *fe = lookup_func(ctx, n->sval);
        if (!fe) {
            SEMA_ERR(ctx, n->line,
                     "call to undefined function '%s'", n->sval);
        } else if (n->nargs != fe->nparams) {
            SEMA_ERR(ctx, n->line,
                     "'%s' called with %d arg%s but defined with %d",
                     n->sval,
                     n->nargs, n->nargs == 1 ? "" : "s",
                     fe->nparams);
        }
        /* Always check argument expressions even if callee is bad.  */
        check_args(ctx, st, n->args, n->nargs);
        break;
    }

    /* ---- Assignment --------------------------------------------- */
    case AST_ASSIGN:
        /*
         * The left child is always an AST_IDENT (guaranteed by the
         * parser).  We check that the variable is in scope, then walk
         * the right-hand expression.
         */
        if (n->left && n->left->kind == AST_IDENT) {
            if (!symtab_lookup(st, n->left->sval)) {
                SEMA_ERR(ctx, n->left->line,
                         "'%s' used before declaration", n->left->sval);
            }
            /* Do NOT call check_node on n->left here; we already did
             * the lookup manually to give a better error message.   */
        }
        check_node(ctx, st, n->right);
        break;

    /* ---- Return statement --------------------------------------- */
    case AST_RETURN:
        /*
         * A return statement is always inside a function (guaranteed
         * by the grammar).  We just check the return expression.
         */
        check_node(ctx, st, n->left);
        break;

    /* ---- If statement ------------------------------------------- */
    case AST_IF:
        check_node(ctx, st, n->left);    /* condition                */
        check_node(ctx, st, n->right);   /* then-block               */
        check_node(ctx, st, n->extra);   /* else-block (may be NULL) */
        break;

    /* ---- While statement ---------------------------------------- */
    case AST_WHILE:
        check_node(ctx, st, n->left);    /* condition                */
        check_node(ctx, st, n->right);   /* body                     */
        break;

    /* ---- Block -------------------------------------------------- */
    case AST_BLOCK:
        /*
         * A block opens a new scope.  Declarations inside the block
         * are visible only within it.
         */
        symtab_push_scope(st);
        check_args(ctx, st, n->args, n->nargs);
        symtab_pop_scope(st);
        break;

    /* ---- Variable declaration ----------------------------------- */
    case AST_VAR_DECL:
        /*
         * Attempt to define the name in the current scope.
         * symtab_define returns 0 if the name is already defined at
         * this scope level — that's a duplicate declaration.
         */
        if (!symtab_define(st, n->sval)) {
            SEMA_ERR(ctx, n->line,
                     "'%s' already declared in this scope", n->sval);
        }
        break;

    /* ---- Expression statement ----------------------------------- */
    case AST_EXPR_STMT:
        check_node(ctx, st, n->left);
        break;

    /* ---- Function definition ------------------------------------ */
    case AST_FUNC: {
        /*
         * Set the current function name for context, open a scope,
         * seed it with the parameters, then walk the body block.
         * We don't push another scope here because parse_block pushes
         * its own scope — but we DO need a scope for the parameters
         * that is enclosing the body scope.  So:
         *
         *   scope[0] = function parameter scope  (push here)
         *     scope[1] = body block scope        (pushed by AST_BLOCK)
         *       ... nested blocks ...
         */
        ctx->current_fn = n->sval;

        symtab_push_scope(st);    /* parameter scope */

        /* Define each parameter as a variable.                     */
        for (int i = 0; i < n->nargs; i++) {
            Node *param = n->args[i];
            if (param && param->kind == AST_VAR_DECL) {
                /* Parameters cannot be duplicate at this level;
                 * symtab_define handles that.                       */
                if (!symtab_define(st, param->sval)) {
                    SEMA_ERR(ctx, param->line,
                             "duplicate parameter '%s'", param->sval);
                }
            }
        }

        /* Walk the body (AST_BLOCK — it will push its own scope).   */
        check_node(ctx, st, n->extra);

        symtab_pop_scope(st);     /* close parameter scope           */
        ctx->current_fn = NULL;
        break;
    }

    /* ---- Program ------------------------------------------------ */
    case AST_PROGRAM:
        /* Pass 1: collect all function signatures.                  */
        for (int i = 0; i < n->nargs; i++) {
            Node *fn = n->args[i];
            if (fn && fn->kind == AST_FUNC)
                register_func(ctx, fn->sval, fn->nargs);
        }
        /* Pass 2: check each function body.                         */
        for (int i = 0; i < n->nargs; i++)
            check_node(ctx, st, n->args[i]);
        break;

    default:
        /* Unrecognised node kind — skip silently.                   */
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int sema_check(Node *program)
{
    /* Initialise the context. */
    SemaCtx ctx;
    ctx.funcs      = NULL;
    ctx.nfuncs     = 0;
    ctx.funcs_cap  = 0;
    ctx.current_fn = NULL;
    ctx.errors     = 0;

    /* Initialise the symbol table. */
    SymTab *st = symtab_new();
    if (!st) {
        fprintf(stderr, "sema: failed to create symbol table\n");
        return 1;
    }

    /* Walk the entire AST. */
    check_node(&ctx, st, program);

    /* Clean up. */
    symtab_free(st);
    for (int i = 0; i < ctx.nfuncs; i++)
        free(ctx.funcs[i].name);
    free(ctx.funcs);

    return ctx.errors;
}
