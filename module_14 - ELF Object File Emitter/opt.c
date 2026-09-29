/* opt.c — IR optimisation passes for my_compiler */
#include "opt.h"

#include <stdlib.h>
#include <string.h>

#define MAX_TEMPS 1024

void opt_fold_constants(IRFunc *f) {
    long val[MAX_TEMPS]; int known[MAX_TEMPS];
    memset(known, 0, sizeof(known));
    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if (ins->op == IR_ICONST && ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            val[ins->dst] = ins->ival; known[ins->dst] = 1;
        }
        int hs1 = (ins->src1 >= 0 && ins->src1 < MAX_TEMPS && known[ins->src1]);
        int hs2 = (ins->src2 >= 0 && ins->src2 < MAX_TEMPS && known[ins->src2]);
        if (hs1 && hs2 && ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            long a = val[ins->src1], b = val[ins->src2], r = 0; int folded = 1;
            switch (ins->op) {
                case IR_ADD: r = a + b; break; case IR_SUB: r = a - b; break;
                case IR_MUL: r = a * b; break;
                case IR_DIV: if (b) r = a / b; else folded = 0; break;
                case IR_MOD: if (b) r = a % b; else folded = 0; break;
                case IR_LT:  r = a <  b; break; case IR_GT:  r = a >  b; break;
                case IR_LEQ: r = a <= b; break; case IR_GEQ: r = a >= b; break;
                case IR_EQ:  r = a == b; break; case IR_NEQ: r = a != b; break;
                case IR_AND: r = (a && b); break; case IR_OR: r = (a || b); break;
                default: folded = 0; break;
            }
            if (folded) {
                ins->op = IR_ICONST; ins->ival = r;
                ins->src1 = ins->src2 = IR_NO_TEMP;
                val[ins->dst] = r; known[ins->dst] = 1;
            }
        }
        if (ins->op == IR_NEG && ins->src1 >= 0 && ins->src1 < MAX_TEMPS &&
            known[ins->src1] && ins->dst >= 0 && ins->dst < MAX_TEMPS) {
            ins->op = IR_ICONST; ins->ival = -val[ins->src1];
            ins->src1 = IR_NO_TEMP;
            val[ins->dst] = ins->ival; known[ins->dst] = 1;
        }
    }
}

void opt_copy_prop(IRFunc *f) {
    int copy_src[MAX_TEMPS];
    for (int i = 0; i < MAX_TEMPS; i++) copy_src[i] = IR_NO_TEMP;
    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if (ins->src1 >= 0 && ins->src1 < MAX_TEMPS && copy_src[ins->src1] != IR_NO_TEMP)
            ins->src1 = copy_src[ins->src1];
        if (ins->src2 >= 0 && ins->src2 < MAX_TEMPS && copy_src[ins->src2] != IR_NO_TEMP)
            ins->src2 = copy_src[ins->src2];
        if (ins->op == IR_COPY && ins->dst >= 0 && ins->dst < MAX_TEMPS)
            copy_src[ins->dst] = ins->src1;
    }
}

void opt_dead_code(IRFunc *f) {
    int used[MAX_TEMPS]; memset(used, 0, sizeof(used));
    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if (ins->src1 >= 0 && ins->src1 < MAX_TEMPS) used[ins->src1] = 1;
        if (ins->src2 >= 0 && ins->src2 < MAX_TEMPS) used[ins->src2] = 1;
    }
    IRInstr *prev = NULL, *ins = f->head;
    while (ins) {
        int removable = 0;
        if (ins->dst >= 0 && ins->dst < MAX_TEMPS && !used[ins->dst]) {
            switch (ins->op) {
                case IR_ICONST: case IR_COPY:
                case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
                case IR_NEG: case IR_LT: case IR_GT: case IR_LEQ: case IR_GEQ:
                case IR_EQ: case IR_NEQ: case IR_AND: case IR_OR: case IR_LOAD:
                    removable = 1; break;
                default: break;
            }
        }
        if (removable) {
            IRInstr *dead = ins;
            if (prev) prev->next = ins->next; else f->head = ins->next;
            if (f->tail == ins) f->tail = prev;
            ins = ins->next; free(dead);
        } else { prev = ins; ins = ins->next; }
    }
}

void opt_all(IRProg *prog) {
    for (int i = 0; i < prog->n_funcs; i++) {
        opt_fold_constants(&prog->funcs[i]);
        opt_copy_prop(&prog->funcs[i]);
        opt_dead_code(&prog->funcs[i]);
    }
}
