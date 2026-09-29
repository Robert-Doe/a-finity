/* sema.h — Semantic analyser interface for my_compiler
 * Module 07: Semantic Analysis
 * Prerequisites: ast.h, symtab.h
 */
#ifndef MY_COMPILER_SEMA_H
#define MY_COMPILER_SEMA_H

#include "ast.h"
#include "symtab.h"

/* Semantic analyser state. */
typedef struct {
    SymTab  globals;    /* global function and variable symbols */
    SymTab  locals;     /* local variables in the current function */
    int     had_error;  /* 1 if any semantic error was found */
} Sema;

/* Run semantic checks on prog. Returns 0 on success, 1 on error. */
int sema_check(Sema *s, Node *prog);

#endif /* MY_COMPILER_SEMA_H */
