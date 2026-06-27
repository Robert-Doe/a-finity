/*
 * opt.h — IR optimizer interface
 * Module 11: Code Generation — Control Flow
 *
 * Passes:
 *   1. Constant folding  — ICONST op ICONST → ICONST result
 *   2. Copy propagation  — if t1 = t2, replace uses of t1 with t2
 *   3. Dead code removal — ICONST/COPY with no downstream uses
 */

#ifndef OPT_H
#define OPT_H

#include "ir.h"

/* Run all optimization passes on prog in place. */
void opt_run(IRProg *prog);

#endif /* OPT_H */
