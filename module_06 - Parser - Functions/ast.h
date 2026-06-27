/* ast.h — AST node types for my_compiler — introduced in Module 04
 * Module: 04
 * Prerequisites: Module 01 (source.h), Module 02 (token.h, lexer.h),
 *                Module 03 (symtab.h).
 *
 * The AST (Abstract Syntax Tree) is the central data structure of the
 * compiler front-end.  Every language construct becomes a Node in this tree.
 */
#ifndef MY_COMPILER_AST_H
#define MY_COMPILER_AST_H

#include <stddef.h>  /* size_t */

/* -------------------------------------------------------------------------
 * NodeKind — the tag that tells us which kind of AST node this is.
 * The comment after each enumerator lists which Node fields are populated.
 * ---------------------------------------------------------------------- */
typedef enum {
    AST_INT_LIT,   /* integer literal:          node->ival                              */
    AST_IDENT,     /* identifier:               node->sval (heap-copied name)           */
    AST_UNARY,     /* unary operation:          node->op (char), node->left             */
    AST_BINARY,    /* binary operation:         node->op (char), node->left, node->right*/
    AST_CALL,      /* function call:            node->sval (func name),
                                                node->args[], node->nargs               */
    AST_ASSIGN,    /* assignment:               node->sval (var name),
                                                node->left (value expr)                 */
    AST_RETURN,    /* return statement:         node->left (expr, may be NULL)          */
    AST_IF,        /* if statement:             node->left (condition),
                                                node->right (then branch),
                                                node->extra (else branch, may be NULL)  */
    AST_WHILE,     /* while loop:               node->left (condition),
                                                node->right (body)                      */
    AST_BLOCK,     /* block of statements:      node->args[] (stmt list), node->nargs   */
    AST_VAR_DECL,  /* variable declaration:     node->sval (name),
                                                node->left (init expr, may be NULL)     */
    AST_EXPR_STMT, /* expression as statement:  node->left (the expression)             */
    AST_FUNC,      /* function definition:      node->sval (name),
                                                node->args[] (param nodes), node->nargs,
                                                node->right (body block),
                                                node->op ('i'=int return, 'v'=void)    */
    AST_PROGRAM    /* whole program:            node->args[] (function list), node->nargs */
} NodeKind;

/* Binary operator encoding (stored in node->op):
 *   '+' '-' '*' '/' '%'  — arithmetic
 *   '<' '>'              — less-than, greater-than
 *   'L' 'G'              — <=  >=
 *   'E' 'N'              — ==  !=
 *   'A' 'O'              — &&  ||
 * For AST_FUNC: 'i' = int return type, 'v' = void return type
 */

/* -------------------------------------------------------------------------
 * Node — a single node in the AST.
 *
 * We use a single "fat" struct rather than a union-of-structs so that
 * every node has the same size and can be allocated uniformly.  Different
 * NodeKinds use different subsets of the fields (see NodeKind comments).
 * ---------------------------------------------------------------------- */
typedef struct Node Node;
struct Node {
    NodeKind  kind;    /* which AST construct this node represents */
    long      ival;    /* integer value — used by AST_INT_LIT */
    char     *sval;    /* heap-allocated name string — used by AST_IDENT,
                          AST_CALL, AST_ASSIGN, AST_VAR_DECL, AST_FUNC */
    char      op;      /* operator character — used by AST_UNARY, AST_BINARY,
                          and AST_FUNC (return type: 'i'=int, 'v'=void) */
    Node     *left;    /* first child node (meaning depends on NodeKind) */
    Node     *right;   /* second child node (meaning depends on NodeKind) */
    Node     *extra;   /* third child node — used by AST_IF for the else branch */
    Node    **args;    /* heap-allocated array of child nodes — used by AST_CALL
                          (arguments), AST_BLOCK (statements), AST_FUNC (params),
                          AST_PROGRAM (functions) */
    int       nargs;   /* number of valid pointers in args[] */
    int       line;    /* source line where this node originates (for errors) */
};

/* node_new: allocate and zero-initialise a Node with the given kind and line.
 * Returns a pointer to the new node.  Exits on allocation failure. */
Node *node_new(NodeKind kind, int line);

/* node_print: recursively pretty-print the subtree rooted at *n to stdout.
 * 'depth' is the current indentation level (pass 0 for the root).
 * This is the primary debugging tool for inspecting parsed ASTs. */
void  node_print(const Node *n, int depth);

/* node_free: recursively free the entire subtree rooted at *n.
 * After this call *n is invalid; set your pointer to NULL afterwards. */
void  node_free(Node *n);

#endif /* MY_COMPILER_AST_H */
