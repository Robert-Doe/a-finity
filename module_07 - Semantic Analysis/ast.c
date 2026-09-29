/*
 * ast.c — AST node allocation, printing, and freeing (unchanged)
 */

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* node_new                                                            */
/* ------------------------------------------------------------------ */
Node *node_new(NodeKind kind, int line)
{
    Node *n = calloc(1, sizeof(Node));
    if (!n) {
        fprintf(stderr, "ast: out of memory\n");
        exit(1);
    }
    n->kind = kind;
    n->line = line;
    return n;
}

/* ------------------------------------------------------------------ */
/* node_print                                                          */
/* ------------------------------------------------------------------ */
static void indent_print(int depth)
{
    for (int i = 0; i < depth * 2; i++) putchar(' ');
}

void node_print(const Node *n, int indent)
{
    if (!n) { indent_print(indent); printf("(null)\n"); return; }

    indent_print(indent);
    switch (n->kind) {
        case AST_INT_LIT:
            printf("INT_LIT(%ld)\n", n->ival);
            break;
        case AST_IDENT:
            printf("IDENT(%s)\n", n->sval ? n->sval : "?");
            break;
        case AST_UNARY:
            printf("UNARY(%c)\n", n->op);
            node_print(n->left, indent + 1);
            break;
        case AST_BINARY:
            printf("BINARY(%c)\n", n->op);
            node_print(n->left,  indent + 1);
            node_print(n->right, indent + 1);
            break;
        case AST_CALL:
            printf("CALL(%s, %d args)\n", n->sval ? n->sval : "?", n->nargs);
            for (int i = 0; i < n->nargs; i++)
                node_print(n->args[i], indent + 1);
            break;
        case AST_ASSIGN:
            printf("ASSIGN\n");
            node_print(n->left,  indent + 1);
            node_print(n->right, indent + 1);
            break;
        case AST_RETURN:
            printf("RETURN\n");
            if (n->left) node_print(n->left, indent + 1);
            break;
        case AST_IF:
            printf("IF\n");
            indent_print(indent + 1); printf("[cond]\n");
            node_print(n->left,  indent + 2);
            indent_print(indent + 1); printf("[then]\n");
            node_print(n->right, indent + 2);
            if (n->extra) {
                indent_print(indent + 1); printf("[else]\n");
                node_print(n->extra, indent + 2);
            }
            break;
        case AST_WHILE:
            printf("WHILE\n");
            indent_print(indent + 1); printf("[cond]\n");
            node_print(n->left,  indent + 2);
            indent_print(indent + 1); printf("[body]\n");
            node_print(n->right, indent + 2);
            break;
        case AST_BLOCK:
            printf("BLOCK(%d stmts)\n", n->nargs);
            for (int i = 0; i < n->nargs; i++)
                node_print(n->args[i], indent + 1);
            break;
        case AST_VAR_DECL:
            printf("VAR_DECL(%s)\n", n->sval ? n->sval : "?");
            break;
        case AST_EXPR_STMT:
            printf("EXPR_STMT\n");
            node_print(n->left, indent + 1);
            break;
        case AST_FUNC:
            printf("FUNC(%s, %d params)\n",
                   n->sval ? n->sval : "?", n->nargs);
            for (int i = 0; i < n->nargs; i++)
                node_print(n->args[i], indent + 1);
            node_print(n->extra, indent + 1);
            break;
        case AST_PROGRAM:
            printf("PROGRAM(%d funcs)\n", n->nargs);
            for (int i = 0; i < n->nargs; i++)
                node_print(n->args[i], indent + 1);
            break;
        default:
            printf("UNKNOWN_NODE\n");
            break;
    }
}

/* ------------------------------------------------------------------ */
/* node_free                                                           */
/* ------------------------------------------------------------------ */
void node_free(Node *n)
{
    if (!n) return;

    /* Free string values.                                           */
    free(n->sval);

    /* Recursively free children.                                    */
    node_free(n->left);
    node_free(n->right);
    node_free(n->extra);

    /* Free args array elements, then the array itself.             */
    for (int i = 0; i < n->nargs; i++)
        node_free(n->args[i]);
    free(n->args);

    free(n);
}
