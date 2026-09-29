/* ast.h — Abstract Syntax Tree for my_compiler
 * Module 05: Parser / AST
 * Prerequisites: token.h
 */
#ifndef MY_COMPILER_AST_H
#define MY_COMPILER_AST_H

#include <stdio.h>

/* Every kind of node that can appear in the AST. */
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
    AST_FUNC,       /* function def: sval, params in args, body in extra */
    AST_PROGRAM     /* top-level: args[0..nargs-1] are functions */
} NodeKind;

/* A single node in the AST. */
typedef struct Node Node;
struct Node {
    NodeKind  kind;     /* what kind of node */
    long      ival;     /* AST_INT_LIT: the value */
    char     *sval;     /* AST_IDENT / AST_CALL / AST_FUNC / AST_VAR_DECL: name */
    char      op;       /* AST_UNARY / AST_BINARY: operator character */
    Node     *left;     /* first child */
    Node     *right;    /* second child */
    Node     *extra;    /* third child (else-branch, function body) */
    Node    **args;     /* variable-length child list */
    int       nargs;    /* number of entries in args */
    int       line;     /* source line for error messages */
};

/* Allocate a new zeroed node of the given kind. Exits on OOM. */
Node *node_new(NodeKind kind, int line);

/* Print the AST to fp in a human-readable indented form. */
void node_print(const Node *n, FILE *fp, int indent);

/* Recursively free the AST rooted at n. */
void node_free(Node *n);

#endif /* MY_COMPILER_AST_H */
