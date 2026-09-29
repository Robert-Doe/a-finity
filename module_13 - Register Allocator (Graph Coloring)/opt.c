/* opt.c — IR optimisation passes for bob_compiler
 * Module 09: Optimisation
 *
 * Pass: constant folding.
 * We make a single forward pass, tracking which temporaries hold
 * a known constant value.  When we find an arithmetic instruction
 * whose both sources are known constants, we replace it in-place
 * with an IR_ICONST and record the new constant.
 */
#include "opt.h"

#include <stdio.h>
#include <string.h>

#define MAX_TEMPS_OPT 512

/* ------------------------------------------------------------------ */
/* Constant folding for one function                                    */
/* ------------------------------------------------------------------ */

static int fold_func(IRFunc *f) {
    /* known_const[t] == 1 means temp t has a known constant value.     */
    /* const_val[t]   holds the actual value when known_const[t] == 1.  */
    int  known_const[MAX_TEMPS_OPT];
    long const_val[MAX_TEMPS_OPT];
    memset(known_const, 0, sizeof(known_const));
    memset(const_val,   0, sizeof(const_val));

    int eliminated = 0;

    for (IRInstr *instr = f->head; instr; instr = instr->next) {
        switch (instr->op) {

        /* Record constants produced by ICONST */
        case IR_ICONST:
            if (instr->dst >= 0 && instr->dst < MAX_TEMPS_OPT) {
                known_const[instr->dst] = 1;
                const_val[instr->dst]   = instr->ival;
            }
            break;

        /* Try to fold binary arithmetic where both sources are constant */
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
        case IR_LT:  case IR_GT:  case IR_LEQ: case IR_GEQ:
        case IR_EQ:  case IR_NEQ: case IR_AND: case IR_OR: {
            int s1 = instr->src1, s2 = instr->src2;
            if (s1 >= 0 && s1 < MAX_TEMPS_OPT && known_const[s1] &&
                s2 >= 0 && s2 < MAX_TEMPS_OPT && known_const[s2]) {
                long a = const_val[s1], b = const_val[s2], result = 0;
                int  ok = 1;
                switch (instr->op) {
                    case IR_ADD: result = a + b; break;
                    case IR_SUB: result = a - b; break;
                    case IR_MUL: result = a * b; break;
                    case IR_DIV: result = (b != 0) ? a / b : (ok = 0, 0); break;
                    case IR_MOD: result = (b != 0) ? a % b : (ok = 0, 0); break;
                    case IR_LT:  result = a <  b; break;
                    case IR_GT:  result = a >  b; break;
                    case IR_LEQ: result = a <= b; break;
                    case IR_GEQ: result = a >= b; break;
                    case IR_EQ:  result = a == b; break;
                    case IR_NEQ: result = a != b; break;
                    case IR_AND: result = (a && b); break;
                    case IR_OR:  result = (a || b); break;
                    default: ok = 0; break;
                }
                if (ok) {
                    /* Rewrite this instruction to IR_ICONST */
                    instr->op   = IR_ICONST;
                    instr->ival = result;
                    instr->src1 = IR_NO_TEMP;
                    instr->src2 = IR_NO_TEMP;
                    if (instr->dst >= 0 && instr->dst < MAX_TEMPS_OPT) {
                        known_const[instr->dst] = 1;
                        const_val[instr->dst]   = result;
                    }
                    eliminated++;
                }
            }
            break;
        }

        /* Propagate constants through NEG */
        case IR_NEG: {
            int s1 = instr->src1;
            if (s1 >= 0 && s1 < MAX_TEMPS_OPT && known_const[s1]) {
                long result = -const_val[s1];
                instr->op   = IR_ICONST;
                instr->ival = result;
                instr->src1 = IR_NO_TEMP;
                if (instr->dst >= 0 && instr->dst < MAX_TEMPS_OPT) {
                    known_const[instr->dst] = 1;
                    const_val[instr->dst]   = result;
                }
                eliminated++;
            }
            break;
        }

        default:
            break;
        }
    }
    return eliminated;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int opt_run(IRProg *prog) {
    int total = 0;
    for (int i = 0; i < prog->n_funcs; i++)
        total += fold_func(&prog->funcs[i]);
    if (total > 0)
        printf("[opt] constant folding eliminated %d instruction(s)\n", total);
    return total;
}
