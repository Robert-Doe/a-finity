/*
 * sema.h — Module 07: Semantic Analysis interface
 *
 * After parsing we have a perfectly structured AST, but the parser
 * doesn't know whether the names in that AST are actually defined.
 * That's the job of semantic analysis.
 *
 * We perform two passes over the AST:
 *
 *   Pass 1 — Function collection
 *     Walk every AST_FUNC node at the top level and record the function
 *     name and its parameter count in a function table.  This lets us
 *     validate call sites even when the callee is defined later.
 *
 *   Pass 2 — Body checking
 *     For each function body we open a new variable scope, seed it with
 *     the function's parameters, then walk every statement and expression:
 *       - AST_VAR_DECL  → define the name in the current scope
 *       - AST_IDENT     → look up the name; error if not found
 *       - AST_ASSIGN    → ensure the lvalue identifier is in scope
 *       - AST_CALL      → ensure the callee is in the function table
 *                         and that the argument count matches
 *
 * All errors are printed immediately but checking continues so that the
 * user gets every error in one run.
 */

#ifndef SEMA_H
#define SEMA_H

#include "ast.h"

/* ------------------------------------------------------------------ */
/* SemaCtx — semantic analysis context                                */
/* ------------------------------------------------------------------ */

/* One entry in the function table.                                  */
typedef struct {
    char *name;     /* heap-allocated function name                  */
    int   nparams;  /* number of declared parameters                 */
} FuncEntry;

/* The top-level semantic context passed through the walk.           */
typedef struct {
    FuncEntry *funcs;       /* heap array of known functions         */
    int        nfuncs;      /* number of entries used                */
    int        funcs_cap;   /* capacity of the array                 */
    char      *current_fn;  /* name of the function being checked    */
    int        errors;      /* running error count                   */
} SemaCtx;

/* ------------------------------------------------------------------ */
/* sema_check                                                         */
/* ------------------------------------------------------------------ */

/*
 * Walk the entire AST rooted at `program` (which must be AST_PROGRAM)
 * and perform semantic analysis as described above.
 *
 * Error messages are printed to stderr in the form:
 *   sema error: 'x' used before declaration (line 5)
 *   sema error: 'foo' called with 3 args but defined with 2 (line 8)
 *   sema error: 'x' already declared in this scope (line 3)
 *   sema error: call to undefined function 'bar' (line 9)
 *
 * Returns 0 if no errors were found, or the total error count if errors
 * were found.  Checking continues past every error.
 */
int sema_check(Node *program);

#endif /* SEMA_H */
