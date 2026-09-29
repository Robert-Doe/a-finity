/*
 * symtab.h — Symbol table with scoped push/pop (new in Module 07)
 *
 * We use a simple stack of hash-chained buckets.  Each "scope" is
 * one level of the stack; pushing opens a new scope, popping closes
 * it and discards all symbols defined in that scope.
 *
 * In our simplified language every declared symbol is a local variable
 * of type int, so we only need to store the name.
 */

#ifndef SYMTAB_H
#define SYMTAB_H

/* ------------------------------------------------------------------ */
/* Opaque symbol-table type                                           */
/* ------------------------------------------------------------------ */
typedef struct SymTab SymTab;

/* Allocate an empty symbol table (no scopes yet).
 * Returns NULL on allocation failure.                                */
SymTab *symtab_new(void);

/* Release all memory owned by the symbol table.                     */
void    symtab_free(SymTab *st);

/* Open a new nested scope (call on entering a function / block).    */
void    symtab_push_scope(SymTab *st);

/* Close the current scope and discard all symbols in it.
 * Calling this with no open scope is a programming error.           */
void    symtab_pop_scope(SymTab *st);

/* Declare a name in the CURRENT (innermost) scope.
 * Returns 1 if the name was newly added.
 * Returns 0 if the name already exists in the current scope
 *   (the caller should report a "duplicate declaration" error).     */
int     symtab_define(SymTab *st, const char *name);

/* Look up a name in ALL scopes, innermost first.
 * Returns 1 if found anywhere in the scope stack, 0 if not found.  */
int     symtab_lookup(SymTab *st, const char *name);

#endif /* SYMTAB_H */
