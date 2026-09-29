/*
 * symtab.c — Symbol Table implementation
 *
 * File:        symtab.c
 * Module:      03 — Symbol Table
 * Description: Flat, fixed-size array of Symbol entries with linear-scan
 *              lookup.  Provides init, add, lookup, and dump operations.
 * Prerequisites: symtab.h
 *
 * Compile:  gcc -Wall -Wextra -Werror -std=c11 -c symtab.c
 */

#include "symtab.h"

#include <string.h>  /* memset, strncpy, strcmp */
#include <stdio.h>   /* printf, fprintf */

/*
 * symtab_init — zero-initialise a SymTab.
 *
 * st: pointer to an uninitialised SymTab to set up.
 * Note: zeroing the entire struct ensures name[0]=='\0' for every slot,
 *       so stale memory never looks like a valid entry.
 */
void symtab_init(SymTab *st)
{
    /* memset is the safest way to zero a struct that contains arrays */
    memset(st, 0, sizeof(*st));
    /* count is now 0, all name fields are '\0', all offsets are 0 */
}

/*
 * symtab_add — insert a new symbol into the table.
 *
 * st:     the symbol table to modify.
 * name:   null-terminated identifier (truncated to 63 chars if longer).
 * kind:   SYM_VAR or SYM_FUNC.
 * offset: stack offset for variables; parameter count for functions.
 * Returns: index of the newly inserted entry on success; -1 on failure
 *          (duplicate name or table full).
 * Assumes: name is not NULL.
 */
int symtab_add(SymTab *st, const char *name, SymKind kind, int offset)
{
    /* reject if we have already reached capacity */
    if (st->count >= SYMTAB_MAX) {
        fprintf(stderr, "symtab_add: table full (max %d)\n", SYMTAB_MAX);
        return -1;
    }

    /* linear scan to detect duplicate names before inserting */
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            /* name already exists — callers must handle redeclaration */
            return -1;
        }
    }

    /* insert at the next available slot */
    Symbol *sym = &st->entries[st->count];

    /* strncpy copies at most 63 chars and leaves the final byte available
     * for the null terminator; we then force-terminate to be safe */
    strncpy(sym->name, name, sizeof(sym->name) - 1);
    sym->name[sizeof(sym->name) - 1] = '\0'; /* guarantee null termination */

    sym->kind   = kind;
    sym->offset = offset;

    /* bump the count and return the index of the new entry */
    return st->count++;
}

/*
 * symtab_lookup — find a symbol by name using a linear scan.
 *
 * st:   the symbol table to search.
 * name: null-terminated string to match against Symbol.name.
 * Returns: pointer to the first matching Symbol, or NULL if not found.
 * Note: linear scan is O(n); at Module 03 programs have fewer than 50
 *       symbols so this is fast enough.  A hash table would give O(1).
 */
Symbol *symtab_lookup(SymTab *st, const char *name)
{
    for (int i = 0; i < st->count; i++) {
        /* strcmp returns 0 on exact match */
        if (strcmp(st->entries[i].name, name) == 0) {
            return &st->entries[i]; /* return pointer into the entries array */
        }
    }
    return NULL; /* not found */
}

/*
 * symtab_dump — print all symbols in the table to stdout.
 *
 * st: the symbol table to display.
 * Note: intended for debugging; format is human-readable not machine-parseable.
 */
void symtab_dump(const SymTab *st)
{
    printf("Symbol Table:\n");
    for (int i = 0; i < st->count; i++) {
        const Symbol *s = &st->entries[i]; /* pointer for concise access */

        if (s->kind == SYM_FUNC) {
            /* for functions, offset encodes the parameter count */
            printf("  [%d] %-10s FUNC  params=%d\n",
                   i, s->name, s->offset);
        } else {
            /* for variables, offset is the byte offset within the stack frame */
            printf("  [%d] %-10s VAR   offset=%d\n",
                   i, s->name, s->offset);
        }
    }
    printf("Total: %d symbols\n", st->count);
}
