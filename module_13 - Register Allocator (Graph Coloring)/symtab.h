/* symtab.h — Symbol table for bob_compiler
 * Module 03: Symbol Table
 */
#ifndef BOB_SYMTAB_H
#define BOB_SYMTAB_H

#define SYMTAB_MAX 256

typedef struct {
    char name[64];
    int  is_func;
    int  n_params;
} Symbol;

typedef struct {
    Symbol entries[SYMTAB_MAX];
    int    count;
} SymTab;

void    symtab_init(SymTab *st);
Symbol *symtab_insert(SymTab *st, const char *name, int is_func, int n_params);
Symbol *symtab_lookup(SymTab *st, const char *name);

#endif /* BOB_SYMTAB_H */
