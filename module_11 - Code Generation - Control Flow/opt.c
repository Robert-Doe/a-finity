/*
 * opt.c — IR optimization passes
 * Module 11: Code Generation — Control Flow
 *
 * Three passes applied repeatedly until stable:
 *   1. Constant folding
 *   2. Copy propagation
 *   3. Dead-code elimination
 *
 * These are deliberately simple (single-pass, no def-use chains).
 * They catch the most common patterns from our irgen.
 */

#include <string.h>
#include <stdio.h>
#include "opt.h"

#define MAX_TEMPS 512

/* ---------------------------------------------------------------- helpers */

static long eval_binop(IROp op, long a, long b) {
    switch (op) {
    case IR_ADD: return a + b;
    case IR_SUB: return a - b;
    case IR_MUL: return a * b;
    case IR_DIV: return b != 0 ? a / b : 0;
    case IR_MOD: return b != 0 ? a % b : 0;
    case IR_LT:  return a < b;
    case IR_GT:  return a > b;
    case IR_LEQ: return a <= b;
    case IR_GEQ: return a >= b;
    case IR_EQ:  return a == b;
    case IR_NEQ: return a != b;
    case IR_AND: return a && b;
    case IR_OR:  return a || b;
    default:     return 0;
    }
}

static int is_binop(IROp op) {
    return op == IR_ADD || op == IR_SUB || op == IR_MUL ||
           op == IR_DIV || op == IR_MOD || op == IR_LT  ||
           op == IR_GT  || op == IR_LEQ || op == IR_GEQ ||
           op == IR_EQ  || op == IR_NEQ || op == IR_AND ||
           op == IR_OR;
}

/* -------------------------------------------------------- constant folding */

static int fold_constants(IRFunc *fn) {
    /* Map: temp slot → const value (valid[slot] = 1 if known constant) */
    long  cvals[MAX_TEMPS];
    int   cknown[MAX_TEMPS];
    memset(cknown, 0, sizeof(cknown));

    int changed = 0;

    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->op == IR_ICONST && ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            cknown[ins->dst] = 1;
            cvals[ins->dst]  = ins->ival;
        }
        if (is_binop(ins->op)) {
            int s1 = ins->src1, s2 = ins->src2;
            if (s1 >= 0 && s1 < MAX_TEMPS && cknown[s1] &&
                s2 >= 0 && s2 < MAX_TEMPS && cknown[s2]) {
                long res = eval_binop(ins->op, cvals[s1], cvals[s2]);
                ins->op   = IR_ICONST;
                ins->ival = res;
                ins->src1 = ins->src2 = -1;
                if (ins->dst >= 0 && ins->dst < MAX_TEMPS) {
                    cknown[ins->dst] = 1;
                    cvals[ins->dst]  = res;
                }
                changed = 1;
            }
        }
        if (ins->op == IR_NEG) {
            int s = ins->src1;
            if (s >= 0 && s < MAX_TEMPS && cknown[s]) {
                ins->op   = IR_ICONST;
                ins->ival = -cvals[s];
                ins->src1 = -1;
                if (ins->dst >= 0 && ins->dst < MAX_TEMPS) {
                    cknown[ins->dst] = 1;
                    cvals[ins->dst]  = ins->ival;
                }
                changed = 1;
            }
        }
        if (ins->op == IR_COPY && ins->src1 >= 0 && ins->src1 < MAX_TEMPS
                && cknown[ins->src1]) {
            ins->op   = IR_ICONST;
            ins->ival = cvals[ins->src1];
            ins->src1 = -1;
            if (ins->dst >= 0 && ins->dst < MAX_TEMPS) {
                cknown[ins->dst] = 1;
                cvals[ins->dst]  = ins->ival;
            }
            changed = 1;
        }
    }
    return changed;
}

/* ------------------------------------------------------- copy propagation */

static int propagate_copies(IRFunc *fn) {
    /* Map: temp slot → canonical temp (or itself if no copy) */
    int canon[MAX_TEMPS];
    for (int i = 0; i < MAX_TEMPS; i++) canon[i] = i;

    int changed = 0;
    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        /* Rewrite sources through canon map */
        if (ins->src1 >= 0 && ins->src1 < MAX_TEMPS && canon[ins->src1] != ins->src1) {
            ins->src1 = canon[ins->src1]; changed = 1;
        }
        if (ins->src2 >= 0 && ins->src2 < MAX_TEMPS && canon[ins->src2] != ins->src2) {
            ins->src2 = canon[ins->src2]; changed = 1;
        }
        /* Record copy */
        if (ins->op == IR_COPY && ins->dst >= 0 && ins->dst < MAX_TEMPS
                && ins->src1 >= 0 && ins->src1 < MAX_TEMPS) {
            canon[ins->dst] = canon[ins->src1];
        }
    }
    return changed;
}

/* --------------------------------------------------- dead code elimination */

static int elim_dead(IRFunc *fn) {
    /* Count uses of each temp */
    int uses[MAX_TEMPS];
    memset(uses, 0, sizeof(uses));

    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->src1 >= 0 && ins->src1 < MAX_TEMPS) uses[ins->src1]++;
        if (ins->src2 >= 0 && ins->src2 < MAX_TEMPS) uses[ins->src2]++;
        /* JUMPZ / RETURN use src1 */
    }

    /* Remove ICONST / COPY / NEG / binop whose dst is unused */
    int changed = 0;
    IRInstr *prev = NULL;
    IRInstr *ins  = fn->head;
    while (ins) {
        int removable = (ins->op == IR_ICONST || ins->op == IR_COPY ||
                         ins->op == IR_NEG    || is_binop(ins->op))
                        && ins->dst >= 0 && ins->dst < MAX_TEMPS
                        && uses[ins->dst] == 0;
        if (removable) {
            IRInstr *next = ins->next;
            if (prev) prev->next = next;
            else       fn->head  = next;
            if (!next) fn->tail  = prev;
            free(ins);
            ins = next;
            changed = 1;
        } else {
            prev = ins;
            ins  = ins->next;
        }
    }
    return changed;
}

/* --------------------------------------------------------- public entry */

void opt_run(IRProg *prog) {
    for (int f = 0; f < prog->n_funcs; f++) {
        IRFunc *fn = &prog->funcs[f];
        /* Iterate until no more changes */
        int max_iter = 20;
        while (max_iter-- > 0) {
            int c = 0;
            c |= fold_constants(fn);
            c |= propagate_copies(fn);
            c |= elim_dead(fn);
            if (!c) break;
        }
    }
}
