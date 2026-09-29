/* ir.h — Three-address-code IR for bob_compiler
 * Module 08: IR Generation
 */
#ifndef BOB_IR_H
#define BOB_IR_H

#include "ast.h"

#define IR_NO_TEMP (-1)

typedef enum {
    IR_ICONST,  /* dst = ival */
    IR_COPY,    /* dst = src1 */
    IR_ADD,     /* dst = src1 + src2 */
    IR_SUB,     /* dst = src1 - src2 */
    IR_MUL,     /* dst = src1 * src2 */
    IR_DIV,     /* dst = src1 / src2 */
    IR_MOD,     /* dst = src1 % src2 */
    IR_NEG,     /* dst = -src1 */
    IR_LT,      /* dst = src1 <  src2 */
    IR_GT,      /* dst = src1 >  src2 */
    IR_LEQ,     /* dst = src1 <= src2 */
    IR_GEQ,     /* dst = src1 >= src2 */
    IR_EQ,      /* dst = src1 == src2 */
    IR_NEQ,     /* dst = src1 != src2 */
    IR_AND,     /* dst = src1 && src2 */
    IR_OR,      /* dst = src1 || src2 */
    IR_LABEL,   /* define label name */
    IR_JUMP,    /* goto name */
    IR_JUMPZ,   /* if src1==0 goto name */
    IR_PARAM,   /* push argument src1 */
    IR_CALL,    /* dst = call name(nargs args) */
    IR_RETURN,  /* return src1 (IR_NO_TEMP = void) */
    IR_STORE,   /* mem[name] = src1 */
    IR_LOAD,    /* dst = mem[name] */
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

IRProg *irgen(Node *program);
void    ir_print(const IRProg *prog);
void    ir_free(IRProg *prog);

#endif /* BOB_IR_H */
