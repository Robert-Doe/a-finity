/* opt.c — IR optimisation passes for my_compiler
 * Module 15: Standard Library Shim
 *
 * Passes implemented:
 *   1. Constant folding  — evaluate binary/unary ops on known constants
 *      at compile time and replace the instruction with IR_ICONST.
 *   2. Copy propagation  — when a temp is defined by IR_COPY t_src,
 *      replace all later uses of that temp with t_src and remove the copy.
 *
 * IR_PRINT: treated as a use of src1 only (no dst definition).
 * The known[] table is not invalidated by IR_PRINT, so if the argument
 * to print() is a compile-time constant it will have been folded already.
 */
#include "opt.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constant-value tracking                                               */
/* ------------------------------------------------------------------ */

#define MAX_TEMPS 512

static int  known[MAX_TEMPS];
static long val[MAX_TEMPS];

/* ------------------------------------------------------------------ */
/* Pass 1: Constant folding                                              */
/* ------------------------------------------------------------------ */

static void fold_constants(IRFunc *f) {
    memset(known, 0, sizeof(known));

    for (IRInstr *in = f->head; in; in = in->next) {
        switch (in->op) {

        case IR_ICONST:
            if (in->dst >= 0 && in->dst < MAX_TEMPS) {
                known[in->dst] = 1;
                val[in->dst]   = in->ival;
            }
            break;

        case IR_NEG:
            if (in->src1 >= 0 && in->src1 < MAX_TEMPS && known[in->src1]) {
                long result = -val[in->src1];
                in->op   = IR_ICONST;
                in->ival = result;
                in->src1 = IR_NO_TEMP;
                if (in->dst >= 0 && in->dst < MAX_TEMPS) {
                    known[in->dst] = 1;
                    val[in->dst]   = result;
                }
            }
            break;

        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
        case IR_LT:  case IR_GT:  case IR_LEQ: case IR_GEQ:
        case IR_EQ:  case IR_NEQ: case IR_AND: case IR_OR: {
            int s1ok = (in->src1 >= 0 && in->src1 < MAX_TEMPS && known[in->src1]);
            int s2ok = (in->src2 >= 0 && in->src2 < MAX_TEMPS && known[in->src2]);
            if (s1ok && s2ok) {
                long a = val[in->src1], b = val[in->src2];
                long result = 0;
                switch (in->op) {
                    case IR_ADD: result = a + b; break;
                    case IR_SUB: result = a - b; break;
                    case IR_MUL: result = a * b; break;
                    case IR_DIV: result = (b != 0) ? a / b : 0; break;
                    case IR_MOD: result = (b != 0) ? a % b : 0; break;
                    case IR_LT:  result = (a <  b) ? 1 : 0; break;
                    case IR_GT:  result = (a >  b) ? 1 : 0; break;
                    case IR_LEQ: result = (a <= b) ? 1 : 0; break;
                    case IR_GEQ: result = (a >= b) ? 1 : 0; break;
                    case IR_EQ:  result = (a == b) ? 1 : 0; break;
                    case IR_NEQ: result = (a != b) ? 1 : 0; break;
                    case IR_AND: result = (a && b) ? 1 : 0; break;
                    case IR_OR:  result = (a || b) ? 1 : 0; break;
                    default: break;
                }
                in->op   = IR_ICONST;
                in->ival = result;
                in->src1 = IR_NO_TEMP;
                in->src2 = IR_NO_TEMP;
                if (in->dst >= 0 && in->dst < MAX_TEMPS) {
                    known[in->dst] = 1;
                    val[in->dst]   = result;
                }
            }
            break;
        }

        /* IR_PRINT: uses src1, produces no dst — do not invalidate anything. */
        case IR_PRINT:
            break;

        default:
            /* Any instruction that writes an unknown value invalidates dst. */
            if (in->dst >= 0 && in->dst < MAX_TEMPS)
                known[in->dst] = 0;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Pass 2: Copy propagation                                              */
/* ------------------------------------------------------------------ */

static int copy_src[MAX_TEMPS];

static void propagate_copies(IRFunc *f) {
    for (int i = 0; i < MAX_TEMPS; i++) copy_src[i] = -1;

    for (IRInstr *in = f->head; in; in = in->next) {
        /* Replace uses of copied temps in src1 and src2 */
        if (in->src1 >= 0 && in->src1 < MAX_TEMPS && copy_src[in->src1] >= 0)
            in->src1 = copy_src[in->src1];
        if (in->src2 >= 0 && in->src2 < MAX_TEMPS && copy_src[in->src2] >= 0)
            in->src2 = copy_src[in->src2];

        if (in->op == IR_COPY && in->dst >= 0 && in->dst < MAX_TEMPS) {
            int src = in->src1;
            while (src >= 0 && src < MAX_TEMPS && copy_src[src] >= 0)
                src = copy_src[src];
            copy_src[in->dst] = src;
        } else if (in->dst >= 0 && in->dst < MAX_TEMPS) {
            copy_src[in->dst] = -1;
        }
        /* IR_PRINT: src1 is already propagated above; dst = IR_NO_TEMP, no action. */
    }
}

/* ------------------------------------------------------------------ */
/* opt_run — run all passes on every function                            */
/* ------------------------------------------------------------------ */
void opt_run(IRProg *prog) {
    if (!prog) return;
    for (int i = 0; i < prog->n_funcs; i++) {
        fold_constants(&prog->funcs[i]);
        propagate_copies(&prog->funcs[i]);
    }
}
