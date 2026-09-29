/*
 * ir.h — Three-address IR for mycc
 * Module 16: The Complete Compiler
 */
#ifndef IR_H
#define IR_H

#include "ast.h"
#include <stddef.h>

/* ------------------------------------------------------------------ IR ops */
typedef enum {
    IR_CONST,       /* dst = imm */
    IR_COPY,        /* dst = src1 */
    IR_ADD,         /* dst = src1 + src2 */
    IR_SUB,         /* dst = src1 - src2 */
    IR_MUL,         /* dst = src1 * src2 */
    IR_DIV,         /* dst = src1 / src2 */
    IR_MOD,         /* dst = src1 % src2 */
    IR_NEG,         /* dst = -src1 */
    IR_NOT,         /* dst = !src1 */
    IR_LT,          /* dst = src1 < src2 */
    IR_LE,          /* dst = src1 <= src2 */
    IR_GT,          /* dst = src1 > src2 */
    IR_GE,          /* dst = src1 >= src2 */
    IR_EQ,          /* dst = src1 == src2 */
    IR_NEQ,         /* dst = src1 != src2 */
    IR_AND,         /* dst = src1 && src2 */
    IR_OR,          /* dst = src1 || src2 */
    IR_LABEL,       /* label: */
    IR_JUMP,        /* goto label */
    IR_JUMPZ,       /* if src1 == 0 goto label */
    IR_CALL,        /* dst = call(func, argc) */
    IR_ARG,         /* push arg for upcoming call */
    IR_RETURN,      /* return src1 */
    IR_PARAM,       /* function parameter declaration */
    IR_PRINT        /* print_int(src1) */
} IROp;

/* ------------------------------------------------------------------ IR operand */
typedef enum { IR_TEMP, IR_VAR, IR_IMM, IR_LABEL_REF } IROpKind;

typedef struct {
    IROpKind kind;
    union {
        int  temp;   /* IR_TEMP:      t0, t1, ... */
        char *var;   /* IR_VAR:       named variable */
        long  imm;   /* IR_IMM:       integer constant */
        int  label;  /* IR_LABEL_REF: label id */
    } u;
} IROperand;

/* ------------------------------------------------------------------ IR instruction */
typedef struct {
    IROp      op;
    IROperand dst;
    IROperand src1;
    IROperand src2;
    /* For IR_CALL: func name stored in src1.u.var (with kind IR_VAR), argc in src2.u.imm */
    /* For IR_LABEL/IR_JUMP/IR_JUMPZ: label id in dst.u.label */
} IRInstr;

/* ------------------------------------------------------------------ IR function */
typedef struct {
    char      *name;
    int        param_count;
    IRInstr   *instrs;
    int        instr_count;
    int        instr_cap;
    int        next_temp;
    int        next_label;
} IRFunc;

/* ------------------------------------------------------------------ IR program */
typedef struct {
    IRFunc   *funcs;
    int       func_count;
    int       func_cap;
} IRProgram;

/* ---- construction ---- */
IRProgram *ir_program_new(void);
IRFunc    *ir_func_new(const char *name, int param_count);
void       ir_program_add_func(IRProgram *prog, IRFunc *fn);
IRInstr   *ir_emit(IRFunc *fn, IROp op);
int        ir_new_temp(IRFunc *fn);
int        ir_new_label(IRFunc *fn);

/* ---- operand helpers ---- */
IROperand ir_temp(int t);
IROperand ir_var(const char *name);
IROperand ir_imm(long v);
IROperand ir_label_ref(int label);
IROperand ir_none(void);

/* ---- codegen from AST ---- */
IRProgram *irgen(ASTNode *program);

/* ---- dump ---- */
void ir_dump(const IRProgram *prog);

/* ---- free ---- */
void ir_program_free(IRProgram *prog);

#endif /* IR_H */
