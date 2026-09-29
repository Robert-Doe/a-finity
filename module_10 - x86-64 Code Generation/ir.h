/* ir.h — Intermediate Representation for my_compiler
 * Module 08: IR Generation
 * Prerequisites: ast.h
 */
#ifndef MY_COMPILER_IR_H
#define MY_COMPILER_IR_H

#include "ast.h"
#include <stdio.h>

/* Sentinel meaning "no temporary" (e.g., void return). */
#define IR_NO_TEMP (-1)

/* Every IR instruction opcode. */
typedef enum {
    IR_ICONST,  /* dst = ival */
    IR_COPY,    /* dst = src1 */
    IR_ADD,     /* dst = src1 + src2 */
    IR_SUB,     /* dst = src1 - src2 */
    IR_MUL,     /* dst = src1 * src2 */
    IR_DIV,     /* dst = src1 / src2 */
    IR_MOD,     /* dst = src1 % src2 */
    IR_NEG,     /* dst = -src1 */
    IR_LT,      /* dst = src1 < src2 */
    IR_GT,      /* dst = src1 > src2 */
    IR_LEQ,     /* dst = src1 <= src2 */
    IR_GEQ,     /* dst = src1 >= src2 */
    IR_EQ,      /* dst = src1 == src2 */
    IR_NEQ,     /* dst = src1 != src2 */
    IR_AND,     /* dst = src1 && src2 */
    IR_OR,      /* dst = src1 || src2 */
    IR_LABEL,   /* label name: */
    IR_JUMP,    /* jmp name */
    IR_JUMPZ,   /* if src1 == 0 jmp name */
    IR_PARAM,   /* push src1 as next argument */
    IR_CALL,    /* dst = call name(nargs args) */
    IR_RETURN,  /* return src1 (IR_NO_TEMP for void) */
    IR_STORE,   /* mem[name] = src1 (named variable) */
    IR_LOAD     /* dst = mem[name] (named variable) */
} IROp;

/* A single IR instruction (linked-list node). */
typedef struct IRInstr IRInstr;
struct IRInstr {
    IROp      op;
    int       dst;        /* destination temporary index, or IR_NO_TEMP */
    int       src1;       /* first source temporary, or IR_NO_TEMP */
    int       src2;       /* second source temporary, or IR_NO_TEMP */
    long      ival;       /* IR_ICONST: constant value */
    char      name[64];   /* IR_LABEL/JUMP/JUMPZ/CALL/STORE/LOAD: label or var name */
    int       nargs;      /* IR_CALL: number of arguments */
    IRInstr  *next;
};

/* IR for a single function. */
typedef struct {
    char      name[64];
    IRInstr  *head;
    IRInstr  *tail;
    int       next_temp;   /* next temporary index to allocate */
    int       next_label;  /* next label index to allocate */
    int       n_locals;    /* number of named local variables */
} IRFunc;

/* IR for the whole program. */
typedef struct {
    IRFunc funcs[32];
    int    n_funcs;
} IRProg;

/* Generate IR from a parsed AST_PROGRAM node.
 * Returns a heap-allocated IRProg (caller frees with ir_prog_free). */
IRProg *ir_gen(const Node *prog);

/* Print IR in human-readable form to fp. */
void ir_prog_print(const IRProg *p, FILE *fp);

/* Free all memory owned by prog. */
void ir_prog_free(IRProg *p);

#endif /* MY_COMPILER_IR_H */
