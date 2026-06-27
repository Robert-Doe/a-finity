/* ir.h — Intermediate Representation for my_compiler */
#ifndef MY_COMPILER_IR_H
#define MY_COMPILER_IR_H

#include "ast.h"
#include <stdio.h>

#define IR_NO_TEMP (-1)

typedef enum {
    IR_ICONST, IR_COPY,
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD, IR_NEG,
    IR_LT, IR_GT, IR_LEQ, IR_GEQ, IR_EQ, IR_NEQ, IR_AND, IR_OR,
    IR_LABEL, IR_JUMP, IR_JUMPZ,
    IR_PARAM, IR_CALL, IR_RETURN,
    IR_STORE, IR_LOAD
} IROp;

typedef struct IRInstr IRInstr;
struct IRInstr {
    IROp     op;
    int      dst;
    int      src1;
    int      src2;
    long     ival;
    char     name[64];
    int      nargs;
    IRInstr *next;
};

typedef struct {
    char     name[64];
    IRInstr *head;
    IRInstr *tail;
    int      next_temp;
    int      next_label;
    int      n_locals;
} IRFunc;

typedef struct {
    IRFunc funcs[32];
    int    n_funcs;
} IRProg;

IRProg *ir_gen(const Node *prog);
void    ir_prog_print(const IRProg *p, FILE *fp);
void    ir_prog_free(IRProg *p);

#endif /* MY_COMPILER_IR_H */
