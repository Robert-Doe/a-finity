/* ast.c — AST node implementation for my_compiler
 * Module 04 — introduced here.
 * Prerequisites: ast.h, <stdlib.h>, <stdio.h>, <string.h>.
 */
#include "ast.h"

#include <stdio.h>   /* printf, fprintf */
#include <stdlib.h>  /* malloc, free, exit */
#include <string.h>  /* memset */

/* -------------------------------------------------------------------------
 * node_new
 * ---------------------------------------------------------------------- */

/* node_new: allocate a zeroed Node and set its kind and source line.
 *
 * WHY memset(0) is important:
 *   - All pointer fields (left, right, extra, args, sval) become NULL.
 *     NULL is the safe "not present" sentinel we check throughout the compiler.
 *   - ival becomes 0, which is the natural default for a non-literal node.
 *   - nargs becomes 0, so args[] loops execute zero times safely.
 *   - Without memset, reading an uninitialised field is undefined behaviour
 *     and a classic source of heisenbugs.
 */
Node *node_new(NodeKind kind, int line) {
    Node *n = malloc(sizeof(Node));       /* allocate raw memory */
    if (!n) {
        fprintf(stderr, "out of memory allocating AST node\n");
        exit(1);
    }
    memset(n, 0, sizeof(Node));           /* zero ALL fields (see comment above) */
    n->kind = kind;                       /* set the tag */
    n->line = line;                       /* record source location for errors */
    return n;
}

/* -------------------------------------------------------------------------
 * node_print helpers
 * ---------------------------------------------------------------------- */

/* indent: print 'depth * 2' spaces to create a visual tree structure. */
static void indent(int depth) {
    for (int i = 0; i < depth * 2; i++) putchar(' ');
}

/* kind_name: return a short readable string for each NodeKind. */
static const char *kind_name(NodeKind k) {
    switch (k) {
    case AST_INT_LIT:   return "INT";
    case AST_IDENT:     return "IDENT";
    case AST_UNARY:     return "UNARY";
    case AST_BINARY:    return "BINARY";
    case AST_CALL:      return "CALL";
    case AST_ASSIGN:    return "ASSIGN";
    case AST_RETURN:    return "RETURN";
    case AST_IF:        return "IF";
    case AST_WHILE:     return "WHILE";
    case AST_BLOCK:     return "BLOCK";
    case AST_VAR_DECL:  return "VAR_DECL";
    case AST_EXPR_STMT: return "EXPR_STMT";
    case AST_FUNC:      return "FUNC";
    case AST_PROGRAM:   return "PROGRAM";
    }
    return "UNKNOWN";
}

/* -------------------------------------------------------------------------
 * node_print
 * ---------------------------------------------------------------------- */

/* node_print: recursively pretty-print the AST rooted at *n.
 *
 * The output format mirrors the tree structure: each level adds two spaces
 * of indentation.  For each node we print the kind name and the most useful
 * field for that kind, then recurse into children.
 *
 * This function is intentionally verbose — it is the main debugging tool
 * when developing the parser and later passes.
 */
void node_print(const Node *n, int depth) {
    if (!n) {                             /* guard against NULL children */
        indent(depth);
        printf("<NULL>\n");
        return;
    }

    indent(depth);

    switch (n->kind) {

    case AST_INT_LIT:
        printf("INT %ld\n", n->ival);    /* show the integer value */
        break;

    case AST_IDENT:
        printf("IDENT '%s'\n", n->sval); /* show the identifier name */
        break;

    case AST_UNARY:
        printf("UNARY '%c'\n", n->op);   /* show the operator char */
        node_print(n->left, depth + 1);  /* recurse into the operand */
        break;

    case AST_BINARY:
        printf("BINARY '%c'\n", n->op);  /* show the operator char */
        node_print(n->left,  depth + 1); /* left operand */
        node_print(n->right, depth + 1); /* right operand */
        break;

    case AST_CALL:
        printf("CALL '%s' (%d args)\n", n->sval, n->nargs);
        for (int i = 0; i < n->nargs; i++) {
            node_print(n->args[i], depth + 1); /* each argument */
        }
        break;

    case AST_ASSIGN:
        printf("ASSIGN '%s'\n", n->sval);
        node_print(n->left, depth + 1); /* the right-hand-side value */
        break;

    case AST_RETURN:
        printf("RETURN\n");
        if (n->left) node_print(n->left, depth + 1); /* optional return value */
        break;

    case AST_IF:
        printf("IF\n");
        indent(depth + 1); printf("cond:\n");
        node_print(n->left,  depth + 2); /* condition */
        indent(depth + 1); printf("then:\n");
        node_print(n->right, depth + 2); /* then-branch */
        if (n->extra) {
            indent(depth + 1); printf("else:\n");
            node_print(n->extra, depth + 2); /* optional else-branch */
        }
        break;

    case AST_WHILE:
        printf("WHILE\n");
        indent(depth + 1); printf("cond:\n");
        node_print(n->left,  depth + 2); /* condition */
        indent(depth + 1); printf("body:\n");
        node_print(n->right, depth + 2); /* loop body */
        break;

    case AST_BLOCK:
        printf("BLOCK (%d stmts)\n", n->nargs);
        for (int i = 0; i < n->nargs; i++) {
            node_print(n->args[i], depth + 1); /* each statement */
        }
        break;

    case AST_VAR_DECL:
        printf("VAR_DECL '%s'\n", n->sval);
        if (n->left) node_print(n->left, depth + 1); /* optional initialiser */
        break;

    case AST_EXPR_STMT:
        printf("EXPR_STMT\n");
        node_print(n->left, depth + 1); /* the expression */
        break;

    case AST_FUNC:
        printf("FUNC '%s' (%d params)\n", n->sval, n->nargs);
        for (int i = 0; i < n->nargs; i++) {
            node_print(n->args[i], depth + 1); /* each parameter */
        }
        indent(depth + 1); printf("body:\n");
        node_print(n->right, depth + 2); /* function body */
        break;

    case AST_PROGRAM:
        printf("PROGRAM (%d functions)\n", n->nargs);
        for (int i = 0; i < n->nargs; i++) {
            node_print(n->args[i], depth + 1); /* each top-level function */
        }
        break;
    }
}

/* -------------------------------------------------------------------------
 * node_free
 * ---------------------------------------------------------------------- */

/* node_free: recursively free the entire subtree rooted at *n.
 *
 * ORDER MATTERS — always free children before the parent node:
 *   1. Recurse into left, right, extra (if non-NULL).
 *   2. Recurse into and free every element in args[].
 *   3. Free the args[] array itself.
 *   4. Free sval (if non-NULL — only some node kinds have it).
 *   5. Free the node struct itself.
 *
 * Freeing the parent first would leave dangling pointers in the children,
 * which is undefined behaviour.  By going children-first we ensure every
 * pointer is valid when we free through it.
 */
void node_free(Node *n) {
    if (!n) return;  /* nothing to do for NULL */

    /* Step 1: free named single children */
    node_free(n->left);
    node_free(n->right);
    node_free(n->extra);

    /* Step 2 & 3: free array children, then the array */
    for (int i = 0; i < n->nargs; i++) {
        node_free(n->args[i]);           /* free each child node */
    }
    free(n->args);                       /* free the pointer array itself */

    /* Step 4: free the heap-allocated name string (if any) */
    free(n->sval);

    /* Step 5: free this node — all its children are already freed */
    free(n);
}
