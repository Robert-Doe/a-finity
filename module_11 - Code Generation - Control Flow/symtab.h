/*
 * symtab.h — Symbol table interface
 * Module 11: Code Generation — Control Flow
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#include <stddef.h>

/* Kind of symbol */
typedef enum {
    SYM_VAR,  /* local variable or parameter */
    SYM_FUNC  /* function name */
} SymKind;

typedef struct Symbol {
    char         name[64];
    SymKind      kind;
    int          nparam;   /* for SYM_FUNC: number of parameters */
    struct Symbol *next;
} Symbol;

/* A single scope frame */
typedef struct Scope {
    Symbol      *syms;
    struct Scope *parent;
} Scope;

typedef struct {
    Scope *top; /* innermost scope */
} SymTab;

void   symtab_init(SymTab *st);
void   symtab_push_scope(SymTab *st);
void   symtab_pop_scope(SymTab *st);
int    symtab_declare(SymTab *st, const char *name, SymKind kind, int nparam);
Symbol *symtab_lookup(SymTab *st, const char *name);
void   symtab_free(SymTab *st);

#endif /* SYMTAB_H */
