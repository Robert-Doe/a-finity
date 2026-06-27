/* opt.c — IR optimisation passes for my_compiler
 * Module 09: Optimization
 *
 * Three passes:
 *   1. Constant folding  — evaluate binary ops on two ICONST temps at compile time
 *   2. Copy propagation  — replace uses of a COPY/ICONST temp with the source value
 *   3. Dead code elim    — delete instructions whose dst temp is never used
 */
#include "opt.h"

#include <stdlib.h>
#include <string.h>

/* Maximum temporaries we track. */
#define MAX_TEMPS 1024

/* ------------------------------------------------------------------ */
/* Constant folding                                                      */
/* ------------------------------------------------------------------ */

void opt_fold_constants(IRFunc *f) {
    /* Build a map: temp_index -> constant value (if known). */
    long  val[MAX_TEMPS];
    int   known[MAX_TEMPS];
    memset(known, 0, sizeof(known));

    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if (ins->op == IR_ICONST && ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            val[ins->dst]   = ins->ival;
            known[ins->dst] = 1;
        }

        /* Binary ops with two known sources */
        int has_s1 = (ins->src1 >= 0 && ins->src1 < MAX_TEMPS && known[ins->src1]);
        int has_s2 = (ins->src2 >= 0 && ins->src2 < MAX_TEMPS && known[ins->src2]);
        if (has_s1 && has_s2 && ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            long a = val[ins->src1], b = val[ins->src2], r = 0;
            int folded = 1;
            switch (ins->op) {
                case IR_ADD: r = a + b; break;
                case IR_SUB: r = a - b; break;
                case IR_MUL: r = a * b; break;
                case IR_DIV: if (b != 0) r = a / b; else folded = 0; break;
                case IR_MOD: if (b != 0) r = a % b; else folded = 0; break;
                case IR_LT:  r = a < b;  break;
                case IR_GT:  r = a > b;  break;
                case IR_LEQ: r = a <= b; break;
                case IR_GEQ: r = a >= b; break;
                case IR_EQ:  r = a == b; break;
                case IR_NEQ: r = a != b; break;
                case IR_AND: r = (a && b); break;
                case IR_OR:  r = (a || b); break;
                default: folded = 0; break;
            }
            if (folded) {
                ins->op   = IR_ICONST;
                ins->ival = r;
                ins->src1 = IR_NO_TEMP;
                ins->src2 = IR_NO_TEMP;
                val[ins->dst]   = r;
                known[ins->dst] = 1;
            }
        }

        /* Unary NEG with known source */
        if (ins->op == IR_NEG &&
            ins->src1 >= 0 && ins->src1 < MAX_TEMPS && known[ins->src1] &&
            ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            ins->op   = IR_ICONST;
            ins->ival = -val[ins->src1];
            ins->src1 = IR_NO_TEMP;
            val[ins->dst]   = ins->ival;
            known[ins->dst] = 1;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Copy propagation                                                      */
/* ------------------------------------------------------------------ */

void opt_copy_prop(IRFunc *f) {
    /* Map temp -> temp it is a direct copy of (or IR_NO_TEMP). */
    int copy_src[MAX_TEMPS];
    for (int i = 0; i < MAX_TEMPS; i++) copy_src[i] = IR_NO_TEMP;

    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        /* Resolve src1 through the copy chain */
        if (ins->src1 >= 0 && ins->src1 < MAX_TEMPS &&
            copy_src[ins->src1] != IR_NO_TEMP)
            ins->src1 = copy_src[ins->src1];

        /* Resolve src2 through the copy chain */
        if (ins->src2 >= 0 && ins->src2 < MAX_TEMPS &&
            copy_src[ins->src2] != IR_NO_TEMP)
            ins->src2 = copy_src[ins->src2];

        /* Record copy relationships */
        if (ins->op == IR_COPY && ins->dst >= 0 && ins->dst < MAX_TEMPS)
            copy_src[ins->dst] = ins->src1;
    }
}

/* ------------------------------------------------------------------ */
/* Dead code elimination                                                 */
/* ------------------------------------------------------------------ */

void opt_dead_code(IRFunc *f) {
    /* Mark temps that are used anywhere as live. */
    int used[MAX_TEMPS];
    memset(used, 0, sizeof(used));

    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if (ins->src1 >= 0 && ins->src1 < MAX_TEMPS) used[ins->src1] = 1;
        if (ins->src2 >= 0 && ins->src2 < MAX_TEMPS) used[ins->src2] = 1;
    }

    /* Remove instructions with an unused dst (except side-effect ops). */
    IRInstr *prev = NULL;
    IRInstr *ins  = f->head;
    while (ins) {
        int removable = 0;
        if (ins->dst >= 0 && ins->dst < MAX_TEMPS && !used[ins->dst]) {
            switch (ins->op) {
                case IR_ICONST:
                case IR_COPY:
                case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
                case IR_NEG:
                case IR_LT: case IR_GT: case IR_LEQ: case IR_GEQ:
                case IR_EQ: case IR_NEQ: case IR_AND: case IR_OR:
                case IR_LOAD:
                    removable = 1;
                    break;
                default:
                    break;
            }
        }
        if (removable) {
            IRInstr *dead = ins;
            if (prev) prev->next = ins->next;
            else      f->head    = ins->next;
            if (f->tail == ins) f->tail = prev;
            ins = ins->next;
            free(dead);
        } else {
            prev = ins;
            ins  = ins->next;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Run all passes                                                        */
/* ------------------------------------------------------------------ */

void opt_all(IRProg *prog) {
    for (int i = 0; i < prog->n_funcs; i++) {
        opt_fold_constants(&prog->funcs[i]);
        opt_copy_prop(&prog->funcs[i]);
        opt_dead_code(&prog->funcs[i]);
    }
}
