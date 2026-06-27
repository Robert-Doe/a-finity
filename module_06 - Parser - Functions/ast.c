/* ast.c — AST node implementation for my_compiler
 * Module 04 — introduced here, unchanged in Module 06.
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
 *   - ival becomes 0, the natural default for a non-literal node.
 *   - nargs becomes 0, so args[] loops run zero times safely.
 *   - Without memset, reading uninitialised fields is undefined behaviour.
 */
Node *node_new(NodeKind kind, int line) {
    Node *n = malloc(sizeof(Node));       /* allocate raw memory */
    if (!n) {
        fprintf(stderr, "out of memory allocating AST node\n");
        exit(1);
    }
    memset(n, 0, sizeof(Node));           /* zero ALL fields */
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

/* -------------------------------------------------------------------------
 * node_print
 * ---------------------------------------------------------------------- */

/* node_print: recursively pretty-print the AST rooted at *n.
 * Each nesting level adds two spaces of indentation.
 * This is the primary debugging tool when developing the parser.
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
        /* show name, return type char ('i'=int, 'v'=void), and param count */
        printf("FUNC '%s' ret='%c' (%d params)\n", n->sval, n->op, n->nargs);
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
 * ORDER: always free children before the parent.
 *   1. Recurse into left, right, extra.
 *   2. Recurse into every element of args[].
 *   3. Free the args[] array.
 *   4. Free sval.
 *   5. Free the node struct itself.
 */
void node_free(Node *n) {
    if (!n) return;  /* nothing to do for NULL */

    node_free(n->left);  /* step 1: named single children */
    node_free(n->right);
    node_free(n->extra);

    for (int i = 0; i < n->nargs; i++) {
        node_free(n->args[i]);  /* step 2: array children */
    }
    free(n->args);   /* step 3: the pointer array */
    free(n->sval);   /* step 4: heap-allocated name string */
    free(n);         /* step 5: the node itself */
}
