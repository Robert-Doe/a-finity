/* opt.h — IR optimisation pass interface for my_compiler
 * Module 09: Optimisation (constant folding, copy propagation)
 * Prerequisites: ir.h
 *
 * The optimiser operates on the IR in-place: it walks the instruction
 * list of each function and rewrites or removes instructions that can
 * be simplified without changing the program's observable behaviour.
 */
#ifndef MY_COMPILER_OPT_H
#define MY_COMPILER_OPT_H

#include "ir.h"

/* Run all optimisation passes on every function in prog.
 * Modifies prog in place.  Safe to call with prog == NULL. */
void opt_run(IRProg *prog);

#endif /* MY_COMPILER_OPT_H */
