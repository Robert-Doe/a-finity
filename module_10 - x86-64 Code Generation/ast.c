/* ast.c — AST node allocation, printing, and freeing
 * Module 05: Parser / AST
 */
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Allocation                                                            */
/* ------------------------------------------------------------------ */

Node *node_new(NodeKind kind, int line) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) {
        fprintf(stderr, "error: out of memory allocating AST node\n");
        exit(1);
    }
    n->kind = kind;
    n->line = line;
    return n;
}

/* ------------------------------------------------------------------ */
/* Printing                                                              */
/* ------------------------------------------------------------------ */

static void indent_print(FILE *fp, int indent) {
    for (int i = 0; i < indent; i++) fprintf(fp, "  ");
}

static const char *kind_name(NodeKind k) {
    switch (k) {
        case AST_INT_LIT:  return "INT_LIT";
        case AST_IDENT:    return "IDENT";
        case AST_UNARY:    return "UNARY";
        case AST_BINARY:   return "BINARY";
        case AST_CALL:     return "CALL";
        case AST_ASSIGN:   return "ASSIGN";
        case AST_RETURN:   return "RETURN";
        case AST_IF:       return "IF";
        case AST_WHILE:    return "WHILE";
        case AST_BLOCK:    return "BLOCK";
        case AST_VAR_DECL: return "VAR_DECL";
        case AST_EXPR_STMT:return "EXPR_STMT";
        case AST_FUNC:     return "FUNC";
        case AST_PROGRAM:  return "PROGRAM";
        default:           return "?";
    }
}

void node_print(const Node *n, FILE *fp, int indent) {
    if (!n) return;
    indent_print(fp, indent);
    fprintf(fp, "(%s", kind_name(n->kind));

    switch (n->kind) {
        case AST_INT_LIT:
            fprintf(fp, " %ld", n->ival);
            break;
        case AST_IDENT:
        case AST_VAR_DECL:
        case AST_FUNC:
        case AST_CALL:
            if (n->sval) fprintf(fp, " \"%s\"", n->sval);
            break;
        case AST_UNARY:
        case AST_BINARY:
            fprintf(fp, " '%c'", n->op);
            break;
        default:
            break;
    }

    if (n->left  || n->right || n->extra || n->nargs > 0)
        fprintf(fp, "\n");

    node_print(n->left,  fp, indent + 1);
    node_print(n->right, fp, indent + 1);
    node_print(n->extra, fp, indent + 1);
    for (int i = 0; i < n->nargs; i++)
        node_print(n->args[i], fp, indent + 1);

    if (n->left || n->right || n->extra || n->nargs > 0)
        indent_print(fp, indent);

    fprintf(fp, ")\n");
}

/* ------------------------------------------------------------------ */
/* Freeing                                                               */
/* ------------------------------------------------------------------ */

void node_free(Node *n) {
    if (!n) return;
    free(n->sval);
    node_free(n->left);
    node_free(n->right);
    node_free(n->extra);
    for (int i = 0; i < n->nargs; i++)
        node_free(n->args[i]);
    free(n->args);
    free(n);
}
