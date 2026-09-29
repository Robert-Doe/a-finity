/*
 * symtab.h — Symbol table for mycc
 * Module 16: The Complete Compiler
 */
#ifndef SYMTAB_H
#define SYMTAB_H

#include <stddef.h>

typedef enum {
    SYM_VAR,
    SYM_FUNC
} SymKind;

typedef struct Symbol {
    char        *name;
    SymKind      kind;
    int          param_count;   /* for SYM_FUNC */
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol      *head;
    struct Scope *parent;
} Scope;

typedef struct {
    Scope *current;
} SymTab;

void    symtab_init(SymTab *st);
void    symtab_push_scope(SymTab *st);
void    symtab_pop_scope(SymTab *st);

/* Insert a symbol in the current scope; returns NULL on duplicate. */
Symbol *symtab_insert(SymTab *st, const char *name, SymKind kind);

/* Look up a name in all scopes (innermost first). */
Symbol *symtab_lookup(SymTab *st, const char *name);

void    symtab_free(SymTab *st);

#endif /* SYMTAB_H */
