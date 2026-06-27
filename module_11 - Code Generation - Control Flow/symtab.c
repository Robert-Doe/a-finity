/*
 * symtab.c — Symbol table implementation
 * Module 11: Code Generation — Control Flow
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symtab.h"

void symtab_init(SymTab *st) {
    st->top = NULL;
}

void symtab_push_scope(SymTab *st) {
    Scope *s = calloc(1, sizeof(Scope));
    s->parent = st->top;
    st->top   = s;
}

void symtab_pop_scope(SymTab *st) {
    if (!st->top) return;
    Scope *s = st->top;
    st->top  = s->parent;

    /* Free all symbols in this scope */
    Symbol *sym = s->syms;
    while (sym) {
        Symbol *next = sym->next;
        free(sym);
        sym = next;
    }
    free(s);
}

int symtab_declare(SymTab *st, const char *name, SymKind kind, int nparam) {
    if (!st->top) {
        fprintf(stderr, "symtab: no active scope\n");
        return 0;
    }
    /* Check for duplicate in current scope */
    for (Symbol *s = st->top->syms; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            fprintf(stderr, "sema error: redeclaration of '%s'\n", name);
            return 0;
        }
    }
    Symbol *sym = calloc(1, sizeof(Symbol));
    strncpy(sym->name, name, sizeof(sym->name) - 1);
    sym->kind   = kind;
    sym->nparam = nparam;
    sym->next   = st->top->syms;
    st->top->syms = sym;
    return 1;
}

Symbol *symtab_lookup(SymTab *st, const char *name) {
    for (Scope *sc = st->top; sc; sc = sc->parent) {
        for (Symbol *s = sc->syms; s; s = s->next) {
            if (strcmp(s->name, name) == 0) return s;
        }
    }
    return NULL;
}

void symtab_free(SymTab *st) {
    while (st->top) symtab_pop_scope(st);
}
