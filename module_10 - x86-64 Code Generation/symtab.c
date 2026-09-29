/* symtab.c — Symbol table implementation for my_compiler
 * Module 03: Symbol Table
 */
#include "symtab.h"

#include <string.h>
#include <stdio.h>

/* Initialise an empty symbol table. */
void symtab_init(SymTab *st) {
    st->count = 0;
}

/* Insert a new symbol. Returns the entry, or NULL if full. */
Symbol *symtab_insert(SymTab *st, const char *name, int is_func, int n_params) {
    if (st->count >= SYMTAB_MAX) {
        fprintf(stderr, "error: symbol table full\n");
        return NULL;
    }
    Symbol *sym = &st->entries[st->count++];
    strncpy(sym->name, name, sizeof(sym->name) - 1);
    sym->name[sizeof(sym->name) - 1] = '\0';
    sym->is_func  = is_func;
    sym->n_params = n_params;
    return sym;
}

/* Linear scan for a symbol by name. */
Symbol *symtab_lookup(SymTab *st, const char *name) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0)
            return &st->entries[i];
    }
    return NULL;
}
