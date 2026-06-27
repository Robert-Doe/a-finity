/*
 * symtab.c — Symbol table implementation for mycc
 * Module 16: The Complete Compiler
 */
#include "symtab.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void symtab_init(SymTab *st)
{
    st->current = NULL;
    symtab_push_scope(st);   /* global scope */
}

void symtab_push_scope(SymTab *st)
{
    Scope *s = (Scope *)malloc(sizeof(Scope));
    if (!s) { fprintf(stderr, "symtab: out of memory\n"); exit(1); }
    s->head   = NULL;
    s->parent = st->current;
    st->current = s;
}

void symtab_pop_scope(SymTab *st)
{
    if (!st->current) return;
    Scope *s = st->current;
    st->current = s->parent;

    Symbol *sym = s->head;
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym);
        sym = next;
    }
    free(s);
}

Symbol *symtab_insert(SymTab *st, const char *name, SymKind kind)
{
    /* Check for duplicate in current scope only */
    for (Symbol *s = st->current->head; s; s = s->next)
        if (strcmp(s->name, name) == 0)
            return NULL;

    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) { fprintf(stderr, "symtab: out of memory\n"); exit(1); }
    sym->name        = strdup(name);
    sym->kind        = kind;
    sym->param_count = 0;
    sym->next        = st->current->head;
    st->current->head = sym;
    return sym;
}

Symbol *symtab_lookup(SymTab *st, const char *name)
{
    for (Scope *sc = st->current; sc; sc = sc->parent)
        for (Symbol *s = sc->head; s; s = s->next)
            if (strcmp(s->name, name) == 0)
                return s;
    return NULL;
}

void symtab_free(SymTab *st)
{
    while (st->current)
        symtab_pop_scope(st);
}
