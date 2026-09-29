/* ast.h — Abstract Syntax Tree for my_compiler
 * Module 15: Standard Library Shim (adds AST_PRINT node)
 * Prerequisites: source.h, token.h
 */
#ifndef MY_COMPILER_AST_H
#define MY_COMPILER_AST_H

#define AST_MAX_ARGS 32 /* maximum children a node can have */

/* Every possible kind of AST node. */
typedef enum {
    /* Expressions */
    AST_INT_LIT,    /* integer literal: ival holds the value */
    AST_IDENT,      /* identifier: name holds the text */
    AST_UNARY,      /* unary op: op holds the token type, args[0] is operand */
    AST_BINARY,     /* binary op: op holds the token type, args[0]=left, args[1]=right */
    AST_CALL,       /* function call: name=func, args[0..n_args-1]=arguments */
    AST_ASSIGN,     /* assignment: name=lhs variable, args[0]=rhs expression */

    /* Statements */
    AST_RETURN,     /* return stmt: args[0]=expression (or no args for void) */
    AST_IF,         /* if stmt: args[0]=cond, args[1]=then, args[2]=else (optional) */
    AST_WHILE,      /* while stmt: args[0]=cond, args[1]=body */
    AST_BLOCK,      /* { stmts }: args[0..n_args-1]=statements */
    AST_VAR_DECL,   /* variable declaration: name=var, args[0]=init (optional) */
    AST_EXPR_STMT,  /* expression used as statement: args[0]=expr */
    AST_PRINT,      /* print statement (Module 15): left=expression to print */

    /* Top-level */
    AST_FUNC,       /* function def: name=func, args[0..n_params-1]=params, last arg=body */
    AST_PROGRAM,    /* whole file: args[0..n_args-1]=functions */
} NodeKind;

/* A node in the AST. */
typedef struct Node Node;
struct Node {
    NodeKind  kind;                 /* what kind of node */
    long      ival;                 /* integer value (AST_INT_LIT) */
    int       op;                   /* operator token type (AST_UNARY, AST_BINARY) */
    char      name[64];             /* identifier / function name */
    Node     *args[AST_MAX_ARGS];  /* child nodes */
    int       n_args;               /* number of valid children */
    int       line;                 /* source line for error reporting */
    /* Convenience pointer used by AST_PRINT: the expression to print. */
    Node     *left;
};

/* Allocate a new node of the given kind. Returns NULL on OOM. */
Node *node_new(NodeKind kind, int line);

/* Recursively print the AST to stdout (for debugging). */
void  node_print(const Node *n, int indent);

/* Recursively free an AST. */
void  node_free(Node *n);

#endif /* MY_COMPILER_AST_H */
