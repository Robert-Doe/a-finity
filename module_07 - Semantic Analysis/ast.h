/*
 * ast.h — Abstract Syntax Tree (Module 05 interface, unchanged)
 *
 * Every syntactic construct in our tiny C subset is represented as a
 * Node.  The NodeKind discriminates the union of fields that are
 * meaningful for each construct.
 */

#ifndef AST_H
#define AST_H

/* ------------------------------------------------------------------ */
/* Node kinds                                                          */
/* ------------------------------------------------------------------ */
typedef enum {
    /* Expressions */
    AST_INT_LIT,   /* integer literal: ival holds the value          */
    AST_IDENT,     /* identifier: sval holds the name                */
    AST_UNARY,     /* unary op:  op, left (operand)                  */
    AST_BINARY,    /* binary op: op, left, right                     */
    AST_CALL,      /* function call: sval=name, args[], nargs        */
    AST_ASSIGN,    /* assignment: left=lvalue ident, right=expr      */

    /* Statements */
    AST_RETURN,    /* return stmt: left=expr (or NULL for void)      */
    AST_IF,        /* if stmt: left=cond, right=then, extra=else     */
    AST_WHILE,     /* while stmt: left=cond, right=body              */
    AST_BLOCK,     /* block { ... }: args[]=stmts, nargs=count       */
    AST_VAR_DECL,  /* variable declaration: sval=name                */
    AST_EXPR_STMT, /* expression used as a statement: left=expr      */

    /* Top-level */
    AST_FUNC,      /* function def: sval=name, args[]=params, nargs,
                      extra=body (AST_BLOCK)                         */
    AST_PROGRAM    /* whole program: args[]=funcs, nargs=count       */
} NodeKind;

/* ------------------------------------------------------------------ */
/* Node                                                                */
/* ------------------------------------------------------------------ */
typedef struct Node {
    NodeKind     kind;    /* discriminator                           */
    long         ival;    /* AST_INT_LIT value                       */
    char        *sval;    /* heap-allocated identifier / func name   */
    char         op;      /* operator char: '+', '-', '*', '/', etc. */
    struct Node *left;    /* primary child                           */
    struct Node *right;   /* secondary child                         */
    struct Node *extra;   /* tertiary child (else branch / body)     */
    struct Node **args;   /* heap array of child pointers            */
    int          nargs;   /* number of entries in args[]             */
    int          line;    /* source line for error messages          */
} Node;

/* ------------------------------------------------------------------ */
/* Functions                                                           */
/* ------------------------------------------------------------------ */

/* Allocate a zeroed Node of the given kind.  Dies on OOM.           */
Node *node_new(NodeKind kind, int line);

/* Recursively print an AST in a readable indented format.           */
void  node_print(const Node *n, int indent);

/* Recursively free an AST.                                          */
void  node_free(Node *n);

#endif /* AST_H */
