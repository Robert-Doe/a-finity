/* ast.c — AST node implementation for my_compiler
 * Module 15: Standard Library Shim (adds AST_PRINT)
 */
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Allocate and zero-initialise a new AST node. */
Node *node_new(NodeKind kind, int line) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) { fprintf(stderr, "error: out of memory\n"); return NULL; }
    n->kind = kind;
    n->line = line;
    return n;
}

/* Map a NodeKind to a printable string. */
static const char *kind_name(NodeKind k) {
    switch (k) {
        case AST_INT_LIT:   return "INT_LIT";
        case AST_IDENT:     return "IDENT";
        case AST_UNARY:     return "UNARY";
        case AST_BINARY:    return "BINARY";
        case AST_ASSIGN:    return "ASSIGN";
        case AST_CALL:      return "CALL";
        case AST_RETURN:    return "RETURN";
        case AST_IF:        return "IF";
        case AST_WHILE:     return "WHILE";
        case AST_BLOCK:     return "BLOCK";
        case AST_EXPR_STMT: return "EXPR_STMT";
        case AST_VAR_DECL:  return "VAR_DECL";
        case AST_PRINT:     return "PRINT";
        case AST_FUNC:      return "FUNC";
        case AST_PROGRAM:   return "PROGRAM";
        default:            return "?";
    }
}

/* Recursively print the AST with indentation. */
void node_print(const Node *n, int indent) {
    if (!n) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("[%s", kind_name(n->kind));
    if (n->kind == AST_INT_LIT)                        printf(" %ld", n->ival);
    if (n->name[0])                                    printf(" '%s'", n->name);
    if (n->kind == AST_BINARY || n->kind == AST_UNARY) printf(" op=%d", n->op);
    printf("]\n");

    /* AST_PRINT stores its expression in ->left */
    if (n->kind == AST_PRINT) {
        node_print(n->left, indent + 1);
        return;
    }

    for (int i = 0; i < n->n_args; i++)
        node_print(n->args[i], indent + 1);
}

/* Recursively free the AST. */
void node_free(Node *n) {
    if (!n) return;
    /* Free the print expression if present */
    if (n->kind == AST_PRINT) {
        node_free(n->left);
        free(n);
        return;
    }
    for (int i = 0; i < n->n_args; i++) node_free(n->args[i]);
    free(n);
}
