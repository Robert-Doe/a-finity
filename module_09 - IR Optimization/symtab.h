/* symtab.h — Symbol table for my_compiler
 * Module 03: Symbol Table
 * Prerequisites: source.h
 */
#ifndef MY_COMPILER_SYMTAB_H
#define MY_COMPILER_SYMTAB_H

#define SYMTAB_MAX 256 /* maximum symbols per scope */

/* A single symbol (variable or function) in the table. */
typedef struct {
    char name[64];  /* symbol name */
    int  is_func;   /* 1 if function, 0 if variable */
    int  n_params;  /* number of parameters (functions only) */
} Symbol;

/* A flat symbol table for one scope level. */
typedef struct {
    Symbol entries[SYMTAB_MAX]; /* symbol storage */
    int    count;               /* number of symbols currently stored */
} SymTab;

/* Initialise an empty symbol table. */
void    symtab_init(SymTab *st);

/* Insert a symbol; returns pointer to entry, or NULL if table is full. */
Symbol *symtab_insert(SymTab *st, const char *name, int is_func, int n_params);

/* Look up a symbol by name; returns pointer or NULL if not found. */
Symbol *symtab_lookup(SymTab *st, const char *name);

#endif /* MY_COMPILER_SYMTAB_H */
