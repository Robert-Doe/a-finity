/*
 * sema.c — Semantic analysis
 * Module 11: Code Generation — Control Flow
 *
 * Checks:
 *  - All identifiers are declared before use.
 *  - Function calls reference a declared function with the correct arity.
 *  - No redeclarations in the same scope.
 */

#include <stdio.h>
#include <string.h>
#include "sema.h"

void sema_init(Sema *sema) {
    symtab_init(&sema->st);
    sema->errors = 0;
}

void sema_free(Sema *sema) {
    symtab_free(&sema->st);
}

/* Forward declaration */
static void check_node(Sema *sema, Node *n);

static void check_expr(Sema *sema, Node *n) {
    if (!n) return;
    switch (n->kind) {
    case AST_INT_LIT: break;
    case AST_IDENT: {
        Symbol *s = symtab_lookup(&sema->st, n->sval);
        if (!s) {
            fprintf(stderr, "sema error at line %d: undeclared identifier '%s'\n",
                    n->line, n->sval);
            sema->errors++;
        }
        break;
    }
    case AST_UNARY:
        check_expr(sema, n->left);
        break;
    case AST_BINARY:
        check_expr(sema, n->left);
        check_expr(sema, n->right);
        break;
    case AST_CALL: {
        Symbol *s = symtab_lookup(&sema->st, n->sval);
        if (!s) {
            fprintf(stderr, "sema error at line %d: undeclared function '%s'\n",
                    n->line, n->sval);
            sema->errors++;
        } else if (s->kind == SYM_FUNC && s->nparam != n->nargs) {
            fprintf(stderr,
                "sema error at line %d: '%s' expects %d args, got %d\n",
                n->line, n->sval, s->nparam, n->nargs);
            sema->errors++;
        }
        for (int i = 0; i < n->nargs; i++) check_expr(sema, n->args[i]);
        break;
    }
    case AST_ASSIGN: {
        Symbol *s = symtab_lookup(&sema->st, n->sval);
        if (!s) {
            fprintf(stderr, "sema error at line %d: undeclared variable '%s'\n",
                    n->line, n->sval);
            sema->errors++;
        }
        check_expr(sema, n->right);
        break;
    }
    default: check_node(sema, n); break;
    }
}

static void check_node(Sema *sema, Node *n) {
    if (!n) return;
    switch (n->kind) {
    case AST_VAR_DECL:
        symtab_declare(&sema->st, n->sval, SYM_VAR, 0);
        if (n->right) check_expr(sema, n->right);
        break;
    case AST_EXPR_STMT:
        check_expr(sema, n->left);
        break;
    case AST_RETURN:
        check_expr(sema, n->left);
        break;
    case AST_IF:
        check_expr(sema, n->left);
        check_node(sema, n->right);
        check_node(sema, n->extra);
        break;
    case AST_WHILE:
        check_expr(sema, n->left);
        check_node(sema, n->right);
        break;
    case AST_BLOCK:
        symtab_push_scope(&sema->st);
        for (int i = 0; i < n->nargs; i++) check_node(sema, n->args[i]);
        symtab_pop_scope(&sema->st);
        break;
    default:
        check_expr(sema, n);
        break;
    }
}

static void check_func(Sema *sema, Node *fn) {
    /* Declare function in outer scope first (so it can be called recursively) */
    symtab_declare(&sema->st, fn->sval, SYM_FUNC, fn->nargs);

    /* Open a new scope for parameters and body */
    symtab_push_scope(&sema->st);
    for (int i = 0; i < fn->nargs; i++) {
        symtab_declare(&sema->st, fn->args[i]->sval, SYM_VAR, 0);
    }
    /* Check body — it's an AST_BLOCK, handle inline to share the param scope */
    Node *body = fn->right;
    if (body && body->kind == AST_BLOCK) {
        for (int i = 0; i < body->nargs; i++) check_node(sema, body->args[i]);
    }
    symtab_pop_scope(&sema->st);
}

int sema_check(Sema *sema, Node *prog) {
    symtab_push_scope(&sema->st); /* global scope */
    for (int i = 0; i < prog->nargs; i++) {
        check_func(sema, prog->args[i]);
    }
    symtab_pop_scope(&sema->st);
    return sema->errors == 0;
}
