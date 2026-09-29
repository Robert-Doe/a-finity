/*
 * ast.h — Abstract Syntax Tree node types
 * Module 11: Code Generation — Control Flow
 */

#ifndef AST_H
#define AST_H

typedef enum {
    AST_INT_LIT,   /* integer literal: ival */
    AST_IDENT,     /* identifier: sval */
    AST_UNARY,     /* unary op: op, left */
    AST_BINARY,    /* binary op: op, left, right */
    AST_CALL,      /* function call: sval (name), args, nargs */
    AST_ASSIGN,    /* assignment: sval (target), right */
    AST_RETURN,    /* return: left (may be NULL) */
    AST_IF,        /* if: left=cond, right=then-block, extra=else-block */
    AST_WHILE,     /* while: left=cond, right=body */
    AST_BLOCK,     /* block: args=stmts, nargs=count */
    AST_VAR_DECL,  /* variable declaration: sval (name), right (init or NULL) */
    AST_EXPR_STMT, /* expression used as statement: left */
    AST_FUNC,      /* function: sval=name, args=params (AST_IDENT), nargs, right=body */
    AST_PROGRAM    /* top-level: args=funcs, nargs */
} NodeKind;

typedef struct Node {
    NodeKind     kind;
    long         ival;    /* AST_INT_LIT */
    char        *sval;    /* AST_IDENT, AST_CALL, AST_VAR_DECL, AST_FUNC */
    char         op;      /* AST_UNARY, AST_BINARY: operator character */
    struct Node *left;
    struct Node *right;
    struct Node *extra;   /* AST_IF: else branch */
    struct Node **args;   /* AST_CALL: argument exprs; AST_BLOCK/FUNC: stmt/param list */
    int          nargs;
    int          line;
} Node;

/* Allocate a new node (zero-initialised). Caller fills fields. */
Node *ast_node(NodeKind kind, int line);

/* Recursively free a node tree. */
void ast_free(Node *n);

/* Pretty-print a node tree (for debugging). */
void ast_print(const Node *n, int indent);

#endif /* AST_H */
