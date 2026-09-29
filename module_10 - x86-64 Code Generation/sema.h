/* sema.h — Semantic analysis for my_compiler
 * Module 07: Semantic Analysis
 * Prerequisites: ast.h, symtab.h
 */
#ifndef MY_COMPILER_SEMA_H
#define MY_COMPILER_SEMA_H

#include "ast.h"
#include "symtab.h"

/* Run semantic checks on prog.
 * Returns 0 if clean, nonzero if any errors were found. */
int sema_check(Node *prog);

#endif /* MY_COMPILER_SEMA_H */
