/* regalloc.h — Register allocator for my_compiler
 * Module 13: Register Allocation
 *
 * We use a simple stack-slot allocator: every IR temporary and named
 * local variable gets a fixed slot on the stack frame.  The codegen
 * layer uses [rbp - 8*slot] to access each value.
 */
#ifndef MY_COMPILER_REGALLOC_H
#define MY_COMPILER_REGALLOC_H

#include "ir.h"

#define REGMAP_MAX 256

/* Maps IR temporary indices and named locals to stack-frame slots. */
typedef struct {
    /* temp_slot[t] = frame slot for temporary t (1-based, 0 = unallocated). */
    int temp_slot[REGMAP_MAX];
    /* named_slot[i] = frame slot for local variable i (1-based). */
    int named_slot[REGMAP_MAX];
    /* Number of frame slots used (= total stack frame size / 8). */
    int n_slots;
    /* Number of named locals tracked (mirrors IRFunc.n_locals). */
    int n_named;
    /* Names of named locals, in slot order. */
    char named_name[REGMAP_MAX][64];
} RegMap;

/* Build a RegMap for a single IR function.
 * Assigns every temporary and named local a unique stack slot. */
void regmap_build(RegMap *rm, const IRFunc *f);

/* Return the slot number (1-based) for temporary t.
 * Slot n lives at [rbp - 8*n]. */
int  regmap_temp_slot(const RegMap *rm, int t);

/* Return the slot number for a named local variable (by name).
 * Returns 0 if not found. */
int  regmap_named_slot(const RegMap *rm, const char *name);

#endif /* MY_COMPILER_REGALLOC_H */
