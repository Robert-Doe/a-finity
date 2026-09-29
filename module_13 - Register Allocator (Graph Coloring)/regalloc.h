/* regalloc.h — Register allocation via graph coloring for bob_compiler
 * Module 13: Register Allocation
 * Prerequisites: ir.h
 *
 * Strategy overview
 * -----------------
 * 1. Liveness analysis: for every temporary t we compute first_def[t]
 *    (the instruction index where t is first written) and last_use[t]
 *    (the last instruction index that reads t).  The live range of t is
 *    the closed interval [first_def[t], last_use[t]].
 *
 * 2. Interference graph: two temporaries t1 and t2 interfere when their
 *    live ranges overlap, i.e. they are both live at some program point
 *    at the same time.  Two ranges [a,b] and [c,d] overlap iff a<=d && c<=b.
 *
 * 3. Greedy graph coloring: process temporaries in order of first_def.
 *    For each temp, collect the set of colors already used by interfering
 *    temps that have already been colored.  Assign the lowest color (register
 *    index) not in that set.  If all NUM_REGS colors are taken, spill: assign
 *    the temp a stack slot instead of a register.
 *
 * Register set
 * ------------
 * We use six registers: rbx, r12, r13, r14, r15 (callee-saved) and r10
 * (caller-saved scratch).  We deliberately avoid rax/rcx/rdx/rdi/rsi/rsi
 * because they carry special roles in the System V AMD64 ABI (return value,
 * argument passing, idiv).  Using callee-saved registers means the function
 * prologue must push them if used, and the epilogue must pop them.
 */
#ifndef BOB_REGALLOC_H
#define BOB_REGALLOC_H

#include "ir.h"

#define MAX_TEMPS 256
#define NUM_REGS  6

/* The six registers we allocate into (in priority order). */
extern const char *REG_NAMES[NUM_REGS]; /* { "rbx","r12","r13","r14","r15","r10" } */

/*
 * RegMap — the result of register allocation for one function.
 *
 *   assign[t]     = register index 0..NUM_REGS-1  if t got a real register
 *                 = -1                             if t was spilled to the stack
 *
 *   spill_slot[t] = byte offset from rbp (negative) for spilled temps,
 *                   e.g. -8 for the first spilled temp, -16 for the second.
 *                   Only meaningful when assign[t] == -1.
 *
 *   n_spilled     = total number of temporaries that were spilled.
 */
typedef struct {
    int assign[MAX_TEMPS];
    int spill_slot[MAX_TEMPS];
    int n_spilled;
} RegMap;

/*
 * regalloc — perform register allocation for one function.
 *
 * Builds liveness intervals, constructs the interference graph, then
 * runs greedy graph coloring to fill in the RegMap.
 */
RegMap regalloc(const IRFunc *fn);

/*
 * regmap_print — print the register assignment for the first n_temps
 * temporaries of a function to stdout.
 */
void regmap_print(const RegMap *rm, int n_temps);

#endif /* BOB_REGALLOC_H */
