/* symtab.c — Symbol table implementation for my_compiler
 * Module 03 — introduced here, unchanged across all modules.
 * Prerequisites: symtab.h.
 */
#include "symtab.h"

#include <stdio.h>   /* printf */
#include <string.h>  /* memset, strncpy, strcmp */

/* symtab_init: clear the table so count==0 and all entries are zero. */
void symtab_init(SymTab *st) {
    memset(st, 0, sizeof(*st));
}

/* symtab_add: insert a new symbol.
 * Returns 0 (failure) if the table is full or the name already exists. */
int symtab_add(SymTab *st, const char *name, SymKind kind, int offset) {
    if (st->count >= SYMTAB_MAX) {
        fprintf(stderr, "symtab: table full, cannot add '%s'\n", name);
        return 0;
    }
    /* Duplicate check - linear scan */
    if (symtab_lookup(st, name) != NULL) {
        fprintf(stderr, "symtab: duplicate symbol '%s'\n", name);
        return 0;
    }
    Symbol *s = &st->entries[st->count++];
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';  /* ensure NUL termination */
    s->kind   = kind;
    s->offset = offset;
    return 1;
}

/* symtab_lookup: linear search for 'name'.
 * Returns pointer to the entry or NULL. */
Symbol *symtab_lookup(SymTab *st, const char *name) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            return &st->entries[i];
        }
    }
    return NULL;
}

/* symtab_dump: print all symbols to stdout. */
void symtab_dump(const SymTab *st) {
    printf("=== Symbol Table (%d entries) ===\n", st->count);
    for (int i = 0; i < st->count; i++) {
        const Symbol *s = &st->entries[i];
        printf("  [%d] %-20s  kind=%s  offset=%d\n",
               i, s->name,
               s->kind == SYM_VAR ? "VAR" : "FUNC",
               s->offset);
    }
}
