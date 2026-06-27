/*
 * ast.c — AST node helpers
 * Module 11: Code Generation — Control Flow
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

Node *ast_node(NodeKind kind, int line) {
    Node *n = calloc(1, sizeof(Node));
    n->kind = kind;
    n->line = line;
    return n;
}

void ast_free(Node *n) {
    if (!n) return;
    free(n->sval);
    ast_free(n->left);
    ast_free(n->right);
    ast_free(n->extra);
    for (int i = 0; i < n->nargs; i++) ast_free(n->args[i]);
    free(n->args);
    free(n);
}

static void indent_print(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void ast_print(const Node *n, int indent) {
    if (!n) return;
    indent_print(indent);
    switch (n->kind) {
    case AST_INT_LIT:   printf("INT(%ld)\n", n->ival); break;
    case AST_IDENT:     printf("IDENT(%s)\n", n->sval); break;
    case AST_UNARY:     printf("UNARY(%c)\n", n->op);
                        ast_print(n->left, indent+1); break;
    case AST_BINARY:    printf("BINARY(%c)\n", n->op);
                        ast_print(n->left, indent+1);
                        ast_print(n->right, indent+1); break;
    case AST_CALL:      printf("CALL(%s)\n", n->sval);
                        for (int i=0;i<n->nargs;i++) ast_print(n->args[i],indent+1); break;
    case AST_ASSIGN:    printf("ASSIGN(%s)\n", n->sval);
                        ast_print(n->right, indent+1); break;
    case AST_RETURN:    printf("RETURN\n"); ast_print(n->left, indent+1); break;
    case AST_IF:        printf("IF\n");
                        ast_print(n->left,  indent+1);
                        ast_print(n->right, indent+1);
                        if (n->extra) { indent_print(indent); printf("ELSE\n");
                            ast_print(n->extra, indent+1); } break;
    case AST_WHILE:     printf("WHILE\n");
                        ast_print(n->left,  indent+1);
                        ast_print(n->right, indent+1); break;
    case AST_BLOCK:     printf("BLOCK\n");
                        for (int i=0;i<n->nargs;i++) ast_print(n->args[i],indent+1); break;
    case AST_VAR_DECL:  printf("VAR_DECL(%s)\n", n->sval);
                        ast_print(n->right, indent+1); break;
    case AST_EXPR_STMT: printf("EXPR_STMT\n"); ast_print(n->left, indent+1); break;
    case AST_FUNC:      printf("FUNC(%s)\n", n->sval);
                        ast_print(n->right, indent+1); break;
    case AST_PROGRAM:   printf("PROGRAM\n");
                        for (int i=0;i<n->nargs;i++) ast_print(n->args[i],indent+1); break;
    default:            printf("???\n"); break;
    }
}
