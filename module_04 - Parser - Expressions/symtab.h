/* symtab.h — Symbol table for my_compiler
 * Module 03 — introduced here, unchanged across all modules.
 * Prerequisites: Module 01 (source.h).
 */
#ifndef MY_COMPILER_SYMTAB_H
#define MY_COMPILER_SYMTAB_H

#define SYMTAB_MAX 256  /* maximum number of symbols in one table */

/* SymKind: whether a symbol is a variable or a function name. */
typedef enum {
    SYM_VAR,   /* local or global variable */
    SYM_FUNC   /* function name */
} SymKind;

/* Symbol: one entry in the symbol table. */
typedef struct {
    char   name[64]; /* NUL-terminated identifier name */
    SymKind kind;    /* variable or function */
    int    offset;   /* byte offset in the stack frame (for variables) */
} Symbol;

/* SymTab: a flat array of Symbol entries.
 * For this course we use a simple linear-search table; a production compiler
 * would use a hash table or a scoped linked-list structure. */
typedef struct {
    Symbol entries[SYMTAB_MAX]; /* the symbol entries */
    int    count;               /* number of entries currently in use */
} SymTab;

/* symtab_init: zero-initialise *st so it is ready to use. */
void    symtab_init(SymTab *st);

/* symtab_add: add a new symbol with the given name, kind, and offset.
 * Returns 1 on success, 0 if the table is full or name is a duplicate. */
int     symtab_add(SymTab *st, const char *name, SymKind kind, int offset);

/* symtab_lookup: find a symbol by name.
 * Returns a pointer into st->entries, or NULL if not found. */
Symbol *symtab_lookup(SymTab *st, const char *name);

/* symtab_dump: print all entries to stdout (useful for debugging). */
void    symtab_dump(const SymTab *st);

#endif /* MY_COMPILER_SYMTAB_H */
