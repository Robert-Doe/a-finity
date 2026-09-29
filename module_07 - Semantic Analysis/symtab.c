/*
 * symtab.c — Symbol table implementation
 *
 * Layout:
 *   SymTab holds a dynamically-grown array of Scope pointers.
 *   Each Scope is a singly-linked list of Entry nodes.
 *   We do a linear scan within each scope (small scopes in practice).
 */

#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal types                                                      */
/* ------------------------------------------------------------------ */

/* One declared name inside a scope.                                 */
typedef struct Entry {
    char        *name;   /* heap-allocated copy of the identifier    */
    struct Entry *next;  /* next entry in this scope's list          */
} Entry;

/* One scope level (function body, block, etc.).                     */
typedef struct Scope {
    Entry       *head;   /* first entry in this scope                */
    struct Scope *prev;  /* enclosing scope (NULL if outermost)      */
} Scope;

/* The public opaque type.                                           */
struct SymTab {
    Scope *top;          /* current (innermost) scope; NULL if empty */
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

SymTab *symtab_new(void)
{
    SymTab *st = malloc(sizeof(SymTab));
    if (!st) { fprintf(stderr, "symtab: out of memory\n"); return NULL; }
    st->top = NULL;
    return st;
}

void symtab_free(SymTab *st)
{
    if (!st) return;
    /* Pop every open scope to free all entries.                     */
    while (st->top) symtab_pop_scope(st);
    free(st);
}

void symtab_push_scope(SymTab *st)
{
    Scope *s = malloc(sizeof(Scope));
    if (!s) { fprintf(stderr, "symtab: out of memory\n"); return; }
    s->head = NULL;
    s->prev = st->top;
    st->top  = s;
}

void symtab_pop_scope(SymTab *st)
{
    if (!st->top) {
        fprintf(stderr, "symtab: pop with no open scope\n");
        return;
    }
    Scope *s = st->top;
    st->top   = s->prev;

    /* Free every entry in this scope.                               */
    Entry *e = s->head;
    while (e) {
        Entry *next = e->next;
        free(e->name);
        free(e);
        e = next;
    }
    free(s);
}

int symtab_define(SymTab *st, const char *name)
{
    if (!st->top) {
        fprintf(stderr, "symtab: define '%s' with no open scope\n", name);
        return 0;
    }

    /* Check for duplicate in the CURRENT scope only.               */
    for (Entry *e = st->top->head; e; e = e->next) {
        if (strcmp(e->name, name) == 0) return 0;   /* already there */
    }

    /* Add a new entry at the front of the current scope's list.    */
    Entry *e = malloc(sizeof(Entry));
    if (!e) { fprintf(stderr, "symtab: out of memory\n"); return 0; }
    e->name = strdup(name);
    if (!e->name) {
        fprintf(stderr, "symtab: out of memory\n");
        free(e);
        return 0;
    }
    e->next       = st->top->head;
    st->top->head = e;
    return 1;
}

int symtab_lookup(SymTab *st, const char *name)
{
    /* Walk from innermost scope outward.                            */
    for (Scope *s = st->top; s; s = s->prev) {
        for (Entry *e = s->head; e; e = e->next) {
            if (strcmp(e->name, name) == 0) return 1;
        }
    }
    return 0;
}
