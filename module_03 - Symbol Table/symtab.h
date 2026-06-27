/*
 * symtab.h — Symbol Table API
 *
 * Module: 03 — Symbol Table
 * Description: Declares the SymTab data structure and the four public
 *              functions for initialising, adding, looking up, and
 *              dumping symbols.
 *
 * Prerequisites: none (this header has no external dependencies).
 */

#ifndef MY_COMPILER_SYMTAB_H
#define MY_COMPILER_SYMTAB_H

/*
 * SYMTAB_MAX — maximum number of symbols in one flat symbol table.
 *
 * 256 is large enough for any function body we will write during the
 * course while keeping the table small enough to fit on the stack if
 * needed.  A production compiler would use a dynamically-grown array
 * or a hash table instead.
 */
#define SYMTAB_MAX 256

/*
 * SymKind — distinguishes variable declarations from function declarations.
 *
 * Only two kinds are needed at Module 03.  Module 07 will add SYM_TYPE
 * once we have user-defined type declarations.
 */
typedef enum {
    SYM_VAR,   /* a local or global variable */
    SYM_FUNC   /* a function name */
} SymKind;

/*
 * Symbol — one entry in the symbol table.
 *
 * Fields:
 *   name[64] — the identifier as a null-terminated string.
 *              64 characters covers all C identifiers in POSIX (at least 31
 *              significant chars required); true unlimited-length identifiers
 *              would require dynamic allocation.
 *   kind     — SYM_VAR or SYM_FUNC.
 *   offset   — for SYM_VAR: the stack frame offset in bytes (e.g. -8 means
 *              8 bytes below the frame pointer); for SYM_FUNC: the number of
 *              parameters the function declares.  This field will be
 *              repurposed in later modules when a proper type system exists.
 */
typedef struct {
    char    name[64];
    SymKind kind;
    int     offset;  /* stack offset for vars; param count for funcs */
} Symbol;

/*
 * SymTab — a flat, fixed-capacity array of Symbols.
 *
 * Entries are stored in insertion order.  Lookup is a linear scan.
 * There is no scope information at this stage — Module 07 introduces
 * a scope stack built from multiple SymTab instances.
 */
typedef struct {
    Symbol entries[SYMTAB_MAX]; /* storage for all symbols */
    int    count;               /* number of valid entries (0 .. SYMTAB_MAX-1) */
} SymTab;

/*
 * symtab_init — zero-initialise a SymTab before first use.
 *
 * st: pointer to an uninitialised SymTab.
 */
void symtab_init(SymTab *st);

/*
 * symtab_add — add a new symbol to the table.
 *
 * st:     the symbol table to update.
 * name:   null-terminated identifier string (at most 63 characters used).
 * kind:   SYM_VAR or SYM_FUNC.
 * offset: stack offset (vars) or parameter count (funcs).
 * Returns: the index of the new entry on success,
 *          -1 if name already exists (duplicate) or the table is full.
 */
int symtab_add(SymTab *st, const char *name, SymKind kind, int offset);

/*
 * symtab_lookup — find a symbol by name.
 *
 * st:   the symbol table to search.
 * name: null-terminated identifier string.
 * Returns: pointer to the matching Symbol, or NULL if not found.
 *          The returned pointer is valid until the SymTab is modified.
 */
Symbol *symtab_lookup(SymTab *st, const char *name);

/*
 * symtab_dump — print every entry in the table to stdout (for debugging).
 *
 * st: the symbol table to display.
 */
void symtab_dump(const SymTab *st);

#endif /* MY_COMPILER_SYMTAB_H */
