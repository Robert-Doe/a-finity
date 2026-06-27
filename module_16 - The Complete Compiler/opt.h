/*
 * opt.h — IR optimiser interface for mycc
 * Module 16: The Complete Compiler
 */
#ifndef OPT_H
#define OPT_H

#include "ir.h"

/*
 * Run all enabled optimisation passes over the IR program.
 * Currently: constant folding on CONST + arithmetic pairs.
 * Returns the number of instructions eliminated.
 */
int opt_all(IRProgram *prog);

/* Individual passes (exposed for testing). */
int opt_const_fold(IRFunc *fn);
int opt_dead_code(IRFunc *fn);

#endif /* OPT_H */
