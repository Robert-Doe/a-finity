/* ast.h — Abstract Syntax Tree for my_compiler */
#ifndef MY_COMPILER_AST_H
#define MY_COMPILER_AST_H

#include <stdio.h>

typedef enum {
    AST_INT_LIT,
    AST_IDENT,
    AST_UNARY,
    AST_BINARY,
    AST_CALL,
    AST_ASSIGN,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_EXPR_STMT,
    AST_FUNC,
    AST_PROGRAM
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind  kind;
    long      ival;
    char     *sval;
    char      op;
    Node     *left;
    Node     *right;
    Node     *extra;
    Node    **args;
    int       nargs;
    int       line;
};

Node *node_new(NodeKind kind, int line);
void  node_print(const Node *n, FILE *fp, int indent);
void  node_free(Node *n);

#endif /* MY_COMPILER_AST_H */
