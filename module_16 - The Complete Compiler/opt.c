/*
 * opt.c — IR optimiser for mycc
 * Module 16: The Complete Compiler
 *
 * Passes:
 *  1. Constant folding: if both operands of an arithmetic/relational op
 *     are immediate, fold at compile time.
 *  2. Dead-code elimination: remove IR_CONST / IR_COPY whose destination
 *     temp is never read.
 */
#include "opt.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---------------------------------------------------------------- const fold */

/* Returns 1 if op is an IROperand we know the value of */
static int imm_val(const IRFunc *fn, const IROperand *op, long *out)
{
    if (op->kind == IR_IMM) { *out = op->u.imm; return 1; }
    if (op->kind == IR_TEMP) {
        /* scan earlier instrs for IR_CONST that defines this temp */
        for (int i = 0; i < fn->instr_count; i++) {
            const IRInstr *ins = &fn->instrs[i];
            if (ins->op == IR_CONST &&
                ins->dst.kind == IR_TEMP &&
                ins->dst.u.temp == op->u.temp) {
                *out = ins->src1.u.imm;
                return 1;
            }
        }
    }
    return 0;
}

int opt_const_fold(IRFunc *fn)
{
    int changed = 0;
    for (int i = 0; i < fn->instr_count; i++) {
        IRInstr *ins = &fn->instrs[i];
        long lv, rv;
        switch (ins->op) {
            case IR_ADD: case IR_SUB: case IR_MUL:
            case IR_DIV: case IR_MOD:
            case IR_LT:  case IR_LE:  case IR_GT: case IR_GE:
            case IR_EQ:  case IR_NEQ:
            case IR_AND: case IR_OR: {
                if (!imm_val(fn, &ins->src1, &lv)) break;
                if (!imm_val(fn, &ins->src2, &rv)) break;
                long result = 0;
                switch (ins->op) {
                    case IR_ADD: result = lv + rv; break;
                    case IR_SUB: result = lv - rv; break;
                    case IR_MUL: result = lv * rv; break;
                    case IR_DIV: result = rv ? lv / rv : 0; break;
                    case IR_MOD: result = rv ? lv % rv : 0; break;
                    case IR_LT:  result = lv <  rv; break;
                    case IR_LE:  result = lv <= rv; break;
                    case IR_GT:  result = lv >  rv; break;
                    case IR_GE:  result = lv >= rv; break;
                    case IR_EQ:  result = lv == rv; break;
                    case IR_NEQ: result = lv != rv; break;
                    case IR_AND: result = lv && rv; break;
                    case IR_OR:  result = lv || rv; break;
                    default: break;
                }
                ins->op   = IR_CONST;
                ins->src1 = ir_imm(result);
                ins->src2 = ir_none();
                changed++;
                break;
            }
            case IR_NEG:
                if (imm_val(fn, &ins->src1, &lv)) {
                    ins->op = IR_CONST; ins->src1 = ir_imm(-lv);
                    changed++;
                }
                break;
            case IR_NOT:
                if (imm_val(fn, &ins->src1, &lv)) {
                    ins->op = IR_CONST; ins->src1 = ir_imm(!lv);
                    changed++;
                }
                break;
            default:
                break;
        }
    }
    return changed;
}

/* ---------------------------------------------------------------- dead code */
/* Mark temps that are actually used as source operands */

int opt_dead_code(IRFunc *fn)
{
    /* Build use-set: which temps are used as src */
    int n = fn->next_temp;
    if (n <= 0) return 0;
    char *used = calloc((size_t)n, 1);
    if (!used) return 0;

    for (int i = 0; i < fn->instr_count; i++) {
        IRInstr *ins = &fn->instrs[i];
        if (ins->src1.kind == IR_TEMP) used[ins->src1.u.temp] = 1;
        if (ins->src2.kind == IR_TEMP) used[ins->src2.u.temp] = 1;
    }

    /* Remove IR_CONST / IR_COPY whose dst temp is never used,
       but only if they have no side-effects */
    int removed = 0;
    int out = 0;
    for (int i = 0; i < fn->instr_count; i++) {
        IRInstr *ins = &fn->instrs[i];
        int dead = 0;
        if ((ins->op == IR_CONST || ins->op == IR_COPY ||
             ins->op == IR_ADD   || ins->op == IR_SUB  ||
             ins->op == IR_MUL   || ins->op == IR_DIV  ||
             ins->op == IR_MOD   || ins->op == IR_NEG  ||
             ins->op == IR_NOT   || ins->op == IR_LT   ||
             ins->op == IR_LE    || ins->op == IR_GT   ||
             ins->op == IR_GE    || ins->op == IR_EQ   ||
             ins->op == IR_NEQ   || ins->op == IR_AND  ||
             ins->op == IR_OR) &&
            ins->dst.kind == IR_TEMP &&
            ins->dst.u.temp < n &&
            !used[ins->dst.u.temp]) {
            dead = 1;
            removed++;
        }
        if (!dead) fn->instrs[out++] = *ins;
    }
    fn->instr_count = out;
    free(used);
    return removed;
}

/* ---------------------------------------------------------------- opt_all */

int opt_all(IRProgram *prog)
{
    int total = 0;
    for (int f = 0; f < prog->func_count; f++) {
        IRFunc *fn = &prog->funcs[f];
        /* iterate to fixed point */
        int changed;
        do {
            changed  = opt_const_fold(fn);
            changed += opt_dead_code(fn);
            total   += changed;
        } while (changed > 0);
    }
    return total;
}
