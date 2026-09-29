/* opt.h — IR optimisation passes for bob_compiler
 * Module 09: Optimisation
 * Prerequisites: ir.h
 *
 * Module 09 adds a constant-folding pass over the IR.
 * If both sources of an arithmetic instruction are IR_ICONST
 * values we know at compile time, we replace the instruction
 * with a single IR_ICONST.
 */
#ifndef BOB_OPT_H
#define BOB_OPT_H

#include "ir.h"

/* Run all optimisation passes over prog in place.
 * Currently: constant folding.
 * Returns the total number of instructions eliminated. */
int opt_run(IRProg *prog);

#endif /* BOB_OPT_H */
