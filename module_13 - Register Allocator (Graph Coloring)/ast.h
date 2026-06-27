/* ast.h — Abstract Syntax Tree for bob_compiler
 * Module 05: Parser / AST
 */
#ifndef BOB_AST_H
#define BOB_AST_H

#include <stdio.h>

typedef enum {
    AST_INT_LIT,    /* integer literal */
    AST_IDENT,      /* identifier reference */
    AST_UNARY,      /* unary operation: op left */
    AST_BINARY,     /* binary operation: left op right */
    AST_CALL,       /* function call: sval(args[0..nargs-1]) */
    AST_ASSIGN,     /* assignment: left = right */
    AST_RETURN,     /* return left; */
    AST_IF,         /* if(left) right [else extra] */
    AST_WHILE,      /* while(left) right */
    AST_BLOCK,      /* { args[0..nargs-1] } */
    AST_VAR_DECL,   /* int sval [= left] */
    AST_EXPR_STMT,  /* left; */
    AST_FUNC,       /* function def: sval, params in args[0..nargs-2], body = extra */
    AST_PROGRAM     /* top-level: args[0..nargs-1] are functions */
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind  kind;
    long      ival;     /* AST_INT_LIT: value */
    char     *sval;     /* AST_IDENT / AST_CALL / AST_FUNC / AST_VAR_DECL: name */
    char      op;       /* AST_UNARY / AST_BINARY: operator */
    Node     *left;     /* first child */
    Node     *right;    /* second child */
    Node     *extra;    /* third child (else-branch, func body) */
    Node    **args;     /* variable-length child list */
    int       nargs;    /* entries in args */
    int       line;     /* source line */
};

Node *node_new(NodeKind kind, int line);
void  node_print(const Node *n, FILE *fp, int indent);
void  node_free(Node *n);

#endif /* BOB_AST_H */
