/* sema.h — Semantic analysis for my_compiler */
#ifndef MY_COMPILER_SEMA_H
#define MY_COMPILER_SEMA_H

#include "ast.h"
#include "symtab.h"

int sema_check(Node *prog);

#endif /* MY_COMPILER_SEMA_H */
