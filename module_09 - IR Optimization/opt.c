/* opt.c — IR optimization passes for my_compiler
 * Module 09: IR Optimization
 *
 * Three simple local optimization passes.  Each pass makes one linear
 * scan through the instruction linked list.  No control-flow graph,
 * no dominators, no SSA — just clean, readable code that beginners
 * can follow step by step.
 */
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum number of temporaries we track in per-pass arrays.
 * Adjust upward if you compile very large functions. */
#define IR_MAX_TEMPS 1024

/* ================================================================== */
/* Pass 1: Constant Folding                                             */
/* ================================================================== */

/*
 * How it works:
 *   We keep two parallel arrays, both indexed by temporary number:
 *     is_const[t]  — 1 if temporary t has a known constant value
 *     const_val[t] — the actual long value, valid when is_const[t]==1
 *
 *   We walk the instruction list once, left to right.
 *   - IR_ICONST dst=t ival=v  → mark is_const[t]=1, const_val[t]=v
 *   - Binary op where is_const[src1] && is_const[src2]
 *       → compute result, rewrite instruction to IR_ICONST dst=result,
 *         mark is_const[dst]=1
 *   - IR_NEG where is_const[src1]
 *       → rewrite to IR_ICONST dst=-src1_value
 *   - Any instruction that writes dst but is NOT folded
 *       → mark is_const[dst]=0 (we don't know the value at compile time)
 */
void opt_fold_constants(IRFunc *fn) {
    /* Stack-allocate the tracking arrays (zero-initialised). */
    int  is_const[IR_MAX_TEMPS];
    long const_val[IR_MAX_TEMPS];
    memset(is_const,  0, sizeof(is_const));
    memset(const_val, 0, sizeof(const_val));

    for (IRInstr *in = fn->head; in != NULL; in = in->next) {

        /* ---- IR_ICONST: record that dst is a known constant ---- */
        if (in->op == IR_ICONST) {
            if (in->dst >= 0 && in->dst < IR_MAX_TEMPS) {
                is_const[in->dst]  = 1;
                const_val[in->dst] = in->ival;
            }
            continue;
        }

        /* ---- IR_NEG: unary fold --------------------------------- */
        if (in->op == IR_NEG) {
            int s = in->src1;
            if (s >= 0 && s < IR_MAX_TEMPS && is_const[s]) {
                long result = -const_val[s];
                in->op   = IR_ICONST;
                in->ival = result;
                in->src1 = IR_NO_TEMP;
                if (in->dst >= 0 && in->dst < IR_MAX_TEMPS) {
                    is_const[in->dst]  = 1;
                    const_val[in->dst] = result;
                }
            } else {
                /* Not foldable — dst is unknown */
                if (in->dst >= 0 && in->dst < IR_MAX_TEMPS)
                    is_const[in->dst] = 0;
            }
            continue;
        }

        /* ---- Binary arithmetic/comparison: try to fold ---------- */
        int is_binary = 0;
        switch (in->op) {
            case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
            case IR_LT:  case IR_GT:  case IR_LEQ: case IR_GEQ:
            case IR_EQ:  case IR_NEQ: case IR_AND: case IR_OR:
                is_binary = 1;
                break;
            default:
                break;
        }

        if (is_binary) {
            int s1 = in->src1, s2 = in->src2;
            int both_known = (s1 >= 0 && s1 < IR_MAX_TEMPS && is_const[s1]) &&
                             (s2 >= 0 && s2 < IR_MAX_TEMPS && is_const[s2]);

            if (both_known) {
                long v1 = const_val[s1], v2 = const_val[s2];
                long result = 0;
                int  folded = 1;

                switch (in->op) {
                    case IR_ADD: result = v1 + v2; break;
                    case IR_SUB: result = v1 - v2; break;
                    case IR_MUL: result = v1 * v2; break;
                    case IR_DIV:
                        if (v2 == 0) {
                            /* Division by zero — leave instruction as-is. */
                            folded = 0;
                        } else {
                            result = v1 / v2;
                        }
                        break;
                    case IR_MOD:
                        if (v2 == 0) {
                            folded = 0;
                        } else {
                            result = v1 % v2;
                        }
                        break;
                    case IR_LT:  result = (v1 <  v2) ? 1 : 0; break;
                    case IR_GT:  result = (v1 >  v2) ? 1 : 0; break;
                    case IR_LEQ: result = (v1 <= v2) ? 1 : 0; break;
                    case IR_GEQ: result = (v1 >= v2) ? 1 : 0; break;
                    case IR_EQ:  result = (v1 == v2) ? 1 : 0; break;
                    case IR_NEQ: result = (v1 != v2) ? 1 : 0; break;
                    case IR_AND: result = (v1 && v2) ? 1 : 0; break;
                    case IR_OR:  result = (v1 || v2) ? 1 : 0; break;
                    default:     folded = 0; break;
                }

                if (folded) {
                    /* Rewrite to IR_ICONST */
                    in->op   = IR_ICONST;
                    in->ival = result;
                    in->src1 = IR_NO_TEMP;
                    in->src2 = IR_NO_TEMP;
                    if (in->dst >= 0 && in->dst < IR_MAX_TEMPS) {
                        is_const[in->dst]  = 1;
                        const_val[in->dst] = result;
                    }
                    continue;
                }
            }

            /* Could not fold — dst value is unknown at compile time */
            if (in->dst >= 0 && in->dst < IR_MAX_TEMPS)
                is_const[in->dst] = 0;
            continue;
        }

        /* ---- Any other instruction that writes dst --------------- */
        if (in->dst != IR_NO_TEMP && in->dst >= 0 && in->dst < IR_MAX_TEMPS)
            is_const[in->dst] = 0;
    }
}

/* ================================================================== */
/* Pass 2: Dead-Code Elimination After Unconditional Jumps             */
/* ================================================================== */

/*
 * How it works:
 *   We walk the instruction list.  When we encounter IR_JUMP (unconditional
 *   branch), every instruction until the next IR_LABEL is unreachable:
 *   nothing can fall through to those instructions.
 *
 *   We remove unreachable instructions by relinking the previous node's
 *   ->next pointer to skip over them, then freeing their memory.
 *
 *   State machine:
 *     in_dead_zone = 0  normal — emit instructions as-is
 *     in_dead_zone = 1  after IR_JUMP — skip/free instructions
 *       IR_LABEL resets in_dead_zone = 0 (label is reachable via jumps)
 */
void opt_dead_code(IRFunc *fn) {
    IRInstr *prev = NULL;
    IRInstr *in   = fn->head;
    int in_dead_zone = 0;

    while (in != NULL) {
        IRInstr *next = in->next;

        if (in->op == IR_LABEL) {
            /* A label is always kept — it may be the target of a jump
             * from somewhere else in the function. */
            in_dead_zone = 0;
            prev = in;
            in   = next;
            continue;
        }

        if (in_dead_zone) {
            /* Unlink this unreachable instruction */
            if (prev != NULL) {
                prev->next = next;
            } else {
                fn->head = next;
            }
            /* Update tail if we just removed the last instruction */
            if (fn->tail == in) {
                fn->tail = prev;
            }
            free(in);
            /* prev stays the same — it now points to `next` */
            in = next;
            continue;
        }

        if (in->op == IR_JUMP) {
            /* Everything after this (until the next label) is dead */
            in_dead_zone = 1;
        }

        prev = in;
        in   = next;
    }
}

/* ================================================================== */
/* Pass 3: Copy Propagation                                             */
/* ================================================================== */

/*
 * How it works:
 *   We maintain a substitution array  sub[t]  where sub[t] == t means
 *   "no substitution" and sub[t] == s (s != t) means "replace uses of
 *   t with s".
 *
 *   Initialise: sub[t] = t  for all t  (identity mapping).
 *
 *   Forward scan:
 *     IR_COPY dst=t2 src1=t1
 *       → sub[t2] = sub[t1]   (chain: if t1 was itself a copy of t0,
 *                               we propagate t0 directly)
 *       → rewrite src1 to sub[t1] in place (optional cleanup)
 *
 *     Any other instruction:
 *       → for every src field (src1, src2), replace with sub[src]
 *       → if this instruction writes dst=t, reset sub[t] = t
 *         (t is now redefined, so old substitution is invalid)
 *
 *   After this pass many IR_COPY instructions become   t2 = COPY t2
 *   (no-ops), but we leave removing them to a later cleanup pass or
 *   the code generator — it keeps this pass simple.
 */
void opt_copy_prop(IRFunc *fn) {
    int sub[IR_MAX_TEMPS];

    /* Initialise substitution to identity */
    for (int i = 0; i < IR_MAX_TEMPS; i++) sub[i] = i;

/* Helper macro: resolve a temp through the substitution table */
#define RESOLVE(t) ((t) >= 0 && (t) < IR_MAX_TEMPS ? sub[(t)] : (t))

    for (IRInstr *in = fn->head; in != NULL; in = in->next) {

        if (in->op == IR_COPY) {
            /* Apply existing substitution to the source */
            int resolved_src = RESOLVE(in->src1);
            in->src1 = resolved_src;

            /* Record that uses of dst can be replaced with resolved_src */
            if (in->dst >= 0 && in->dst < IR_MAX_TEMPS) {
                sub[in->dst] = resolved_src;
            }
            /* dst is "written" by this copy — but the substitution we just
             * set IS the right one (propagate the source), so we do NOT
             * reset sub[dst] here. */
            continue;
        }

        /* For all other instructions, substitute sources first */
        if (in->src1 != IR_NO_TEMP) in->src1 = RESOLVE(in->src1);
        if (in->src2 != IR_NO_TEMP) in->src2 = RESOLVE(in->src2);

        /* If this instruction writes dst, invalidate the substitution
         * for that temp (it now holds a new, unknown value). */
        if (in->dst != IR_NO_TEMP && in->dst >= 0 && in->dst < IR_MAX_TEMPS) {
            sub[in->dst] = in->dst; /* reset to identity */
        }
    }

#undef RESOLVE
}

/* ================================================================== */
/* opt_all — apply all three passes to every function                  */
/* ================================================================== */

void opt_all(IRProg *prog) {
    for (int i = 0; i < prog->n_funcs; i++) {
        IRFunc *fn = &prog->funcs[i];
        opt_fold_constants(fn);  /* fold first: creates more ICONST temps */
        opt_copy_prop(fn);       /* then propagate copies away */
        opt_dead_code(fn);       /* finally prune unreachable instructions */
    }
}
