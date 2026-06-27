/* opt.h — IR optimizer for my_compiler
 * Module 09: Optimization
 * Prerequisites: ir.h
 */
#ifndef MY_COMPILER_OPT_H
#define MY_COMPILER_OPT_H

#include "ir.h"

/* Fold constant expressions: ICONST op ICONST -> ICONST. */
void opt_fold_constants(IRFunc *f);

/* Remove instructions whose result is never used. */
void opt_dead_code(IRFunc *f);

/* Replace copies of constants with the constant directly. */
void opt_copy_prop(IRFunc *f);

/* Run all optimisation passes over every function. */
void opt_all(IRProg *prog);

#endif /* MY_COMPILER_OPT_H */
