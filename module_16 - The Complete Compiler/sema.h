/*
 * sema.h — Semantic analysis interface for mycc
 * Module 16: The Complete Compiler
 */
#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "symtab.h"

typedef struct {
    SymTab  symtab;
    int     errors;
    char   *current_func;   /* name of function being analysed */
} SemaCtx;

/* Run semantic checks on the whole program.
   Returns the number of errors found. */
int sema_check(SemaCtx *ctx, ASTNode *program);

void sema_ctx_init(SemaCtx *ctx);
void sema_ctx_free(SemaCtx *ctx);

#endif /* SEMA_H */
