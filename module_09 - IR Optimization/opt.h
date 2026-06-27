/* opt.h — IR optimization passes for my_compiler
 * Module 09: IR Optimization
 * Prerequisites: ir.h
 *
 * Three simple local optimization passes, each making one linear scan
 * through the instruction list of a function.  No complex data-flow
 * analysis is required — these are intentionally beginner-friendly.
 *
 * Pass order recommendation: fold_constants → copy_prop → dead_code
 * (folding first creates more constant temps that copy_prop can clean up;
 *  dead_code last removes any leftover unreachable sequences).
 */
#ifndef MY_COMPILER_OPT_H
#define MY_COMPILER_OPT_H

#include "ir.h"

/* ------------------------------------------------------------------
 * opt_fold_constants — constant folding
 *
 * For each arithmetic/comparison instruction where BOTH src1 and src2
 * are temporaries whose values are already known (i.e., they were
 * loaded by an earlier IR_ICONST), compute the result at compile time
 * and replace the instruction with a single IR_ICONST dst = <result>.
 *
 * Supported ops: IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
 *                IR_LT, IR_GT, IR_LEQ, IR_GEQ, IR_EQ, IR_NEQ,
 *                IR_AND, IR_OR.
 *
 * A small array (indexed by temp number) tracks which temporaries hold
 * known constant values.  The array is sized to IR_MAX_TEMPS (see opt.c).
 *
 * NOTE: IR_NEG with a known src1 is also folded (unary case).
 * ------------------------------------------------------------------*/
void opt_fold_constants(IRFunc *fn);

/* ------------------------------------------------------------------
 * opt_dead_code — dead-code elimination after unconditional jumps
 *
 * Any instruction that appears between an IR_JUMP and the next
 * IR_LABEL is unreachable: control flow cannot enter that region
 * because the only way to reach it is to fall through the jump, which
 * by definition redirects execution elsewhere.
 *
 * This pass removes those unreachable instructions by unlinking them
 * from the instruction list and freeing their memory.
 *
 * Limitation: only the simple pattern "IR_JUMP ... IR_LABEL" is
 * handled.  More sophisticated dead-code elimination (e.g., removing
 * entire unreachable basic blocks) requires a control-flow graph and
 * is left for later modules.
 * ------------------------------------------------------------------*/
void opt_dead_code(IRFunc *fn);

/* ------------------------------------------------------------------
 * opt_copy_prop — copy propagation
 *
 * For each IR_COPY instruction of the form  t_dst = COPY t_src,
 * replace every subsequent USE of t_dst with t_src — until t_dst is
 * written again (i.e., appears as the dst of another instruction).
 *
 * This pass makes one forward scan through the instruction list.  It
 * maintains a substitution table: sub[t] = t means "replace uses of t
 * with t" (identity / no substitution).  When we see IR_COPY dst=t2
 * src1=t1, we set sub[t2] = sub[t1] (chaining through earlier copies).
 * When we see any instruction that writes t2 (dst == t2), we reset
 * sub[t2] = t2.
 * ------------------------------------------------------------------*/
void opt_copy_prop(IRFunc *fn);

/* ------------------------------------------------------------------
 * opt_all — apply all three passes to every function in the program
 *
 * Pass order: fold_constants → copy_prop → dead_code
 * ------------------------------------------------------------------*/
void opt_all(IRProg *prog);

#endif /* MY_COMPILER_OPT_H */
