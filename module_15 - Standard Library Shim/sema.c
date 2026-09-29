/* sema.c — Semantic analysis implementation for my_compiler
 * Module 15: Standard Library Shim (adds AST_PRINT handling)
 */
#include "sema.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Forward declarations                                                  */
/* ------------------------------------------------------------------ */
static void check_expr(Sema *s, Node *n);
static void check_stmt(Sema *s, Node *n);

/* ------------------------------------------------------------------ */
/* Helpers                                                               */
/* ------------------------------------------------------------------ */

static void sema_error(Sema *s, int line, const char *msg, const char *name) {
    fprintf(stderr, "semantic error (line %d): %s '%s'\n", line, msg, name);
    s->had_error = 1;
}

/* ------------------------------------------------------------------ */
/* Expression checking                                                   */
/* ------------------------------------------------------------------ */

static void check_expr(Sema *s, Node *n) {
    if (!n) return;
    switch (n->kind) {
        case AST_INT_LIT:
            break; /* always valid */

        case AST_IDENT:
            if (!symtab_lookup(&s->locals, n->name) &&
                !symtab_lookup(&s->globals, n->name)) {
                sema_error(s, n->line, "undeclared identifier", n->name);
            }
            break;

        case AST_UNARY:
            check_expr(s, n->args[0]);
            break;

        case AST_BINARY:
            check_expr(s, n->args[0]);
            check_expr(s, n->args[1]);
            break;

        case AST_ASSIGN: {
            Symbol *sym = symtab_lookup(&s->locals, n->name);
            if (!sym) sym = symtab_lookup(&s->globals, n->name);
            if (!sym) sema_error(s, n->line, "undeclared variable in assignment", n->name);
            check_expr(s, n->args[0]);
            break;
        }

        case AST_CALL: {
            Symbol *sym = symtab_lookup(&s->globals, n->name);
            if (!sym || !sym->is_func) {
                sema_error(s, n->line, "undeclared function", n->name);
            } else if (sym->n_params != n->n_args) {
                fprintf(stderr, "semantic error (line %d): function '%s' expects %d args, got %d\n",
                        n->line, n->name, sym->n_params, n->n_args);
                s->had_error = 1;
            }
            for (int i = 0; i < n->n_args; i++) check_expr(s, n->args[i]);
            break;
        }

        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Statement checking                                                    */
/* ------------------------------------------------------------------ */

static void check_stmt(Sema *s, Node *n) {
    if (!n) return;
    switch (n->kind) {
        case AST_RETURN:
            if (n->n_args > 0) check_expr(s, n->args[0]);
            break;

        case AST_IF:
            check_expr(s, n->args[0]);
            check_stmt(s, n->args[1]);
            if (n->n_args > 2) check_stmt(s, n->args[2]);
            break;

        case AST_WHILE:
            check_expr(s, n->args[0]);
            check_stmt(s, n->args[1]);
            break;

        case AST_BLOCK:
            for (int i = 0; i < n->n_args; i++) check_stmt(s, n->args[i]);
            break;

        case AST_EXPR_STMT:
            check_expr(s, n->args[0]);
            break;

        case AST_VAR_DECL:
            symtab_insert(&s->locals, n->name, 0, 0);
            if (n->n_args > 0) check_expr(s, n->args[0]);
            break;

        case AST_ASSIGN:
            check_expr(s, n);
            break;

        /* Module 15: print(expr) — recursively check the expression.
         * No other semantic restriction: any integer expression is valid. */
        case AST_PRINT:
            check_expr(s, n->left);
            break;

        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Function and program checking                                         */
/* ------------------------------------------------------------------ */

static void check_function(Sema *s, Node *func) {
    symtab_init(&s->locals);

    int body_idx = func->n_args - 1;
    for (int i = 0; i < body_idx; i++) {
        symtab_insert(&s->locals, func->args[i]->name, 0, 0);
    }

    check_stmt(s, func->args[body_idx]);
}

static void register_functions(Sema *s, Node *prog) {
    for (int i = 0; i < prog->n_args; i++) {
        Node *func = prog->args[i];
        if (func->kind != AST_FUNC) continue;
        int n_params = func->n_args - 1; /* last arg is body */
        symtab_insert(&s->globals, func->name, 1, n_params);
    }
}

int sema_check(Sema *s, Node *prog) {
    symtab_init(&s->globals);
    symtab_init(&s->locals);
    s->had_error = 0;

    register_functions(s, prog);

    for (int i = 0; i < prog->n_args; i++) {
        if (prog->args[i]->kind == AST_FUNC)
            check_function(s, prog->args[i]);
    }

    return s->had_error;
}
