/* opt.h — IR optimizer for my_compiler */
#ifndef MY_COMPILER_OPT_H
#define MY_COMPILER_OPT_H

#include "ir.h"

void opt_fold_constants(IRFunc *f);
void opt_dead_code(IRFunc *f);
void opt_copy_prop(IRFunc *f);
void opt_all(IRProg *prog);

#endif /* MY_COMPILER_OPT_H */
