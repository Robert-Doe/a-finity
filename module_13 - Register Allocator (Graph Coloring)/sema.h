/* sema.h — Semantic analysis for bob_compiler
 * Module 07: Semantic Analysis
 */
#ifndef BOB_SEMA_H
#define BOB_SEMA_H

#include "ast.h"
#include "symtab.h"

/* Returns 0 if clean, nonzero if any errors. */
int sema_check(Node *prog);

#endif /* BOB_SEMA_H */
