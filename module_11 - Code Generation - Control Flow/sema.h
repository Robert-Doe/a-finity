/*
 * sema.h — Semantic analysis interface
 * Module 11: Code Generation — Control Flow
 */

#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "symtab.h"

typedef struct {
    SymTab  st;
    int     errors;
} Sema;

/* Run semantic checks on the AST. Returns 0 if errors were found. */
int sema_check(Sema *sema, Node *prog);

void sema_init(Sema *sema);
void sema_free(Sema *sema);

#endif /* SEMA_H */
