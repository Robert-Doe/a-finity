/* ir.h — Intermediate representation for my_compiler
 * Module 08: IR Generation
 * Prerequisites: source.h, token.h, lexer.h, symtab.h, ast.h, parser.h, sema.h
 *
 * This file defines the three-address-code IR used by my_compiler.
 * Every instruction performs at most one operation and writes its result
 * into a numbered "temporary" register (t0, t1, t2, ...).
 */
#ifndef MY_COMPILER_IR_H
#define MY_COMPILER_IR_H

#include "ast.h"

/* Sentinel value meaning "no temporary" — used for instructions that
 * do not produce or consume a value (IR_JUMP, IR_LABEL, etc.).
 * Note: actual temporaries are non-negative integers (0, 1, 2, ...). */
#define IR_NO_TEMP (-1)

/* ------------------------------------------------------------------ */
/* IROp — the operation performed by one IR instruction                  */
/* ------------------------------------------------------------------ */
typedef enum {
    IR_ICONST,  /* dst = ival                              load an integer constant */
    IR_COPY,    /* dst = src1                              copy one temp to another */
    IR_ADD,     /* dst = src1 + src2                       integer addition */
    IR_SUB,     /* dst = src1 - src2                       integer subtraction */
    IR_MUL,     /* dst = src1 * src2                       integer multiplication */
    IR_DIV,     /* dst = src1 / src2                       integer division */
    IR_MOD,     /* dst = src1 % src2                       integer modulo */
    IR_NEG,     /* dst = -src1                             unary negation */
    IR_LT,      /* dst = src1 <  src2  (1 or 0)           less-than comparison */
    IR_GT,      /* dst = src1 >  src2                      greater-than comparison */
    IR_LEQ,     /* dst = src1 <= src2                      less-or-equal comparison */
    IR_GEQ,     /* dst = src1 >= src2                      greater-or-equal comparison */
    IR_EQ,      /* dst = src1 == src2                      equality comparison */
    IR_NEQ,     /* dst = src1 != src2                      inequality comparison */
    IR_AND,     /* dst = src1 && src2  (short-circuit NOT implemented) */
    IR_OR,      /* dst = src1 || src2                      logical or */
    IR_LABEL,   /* define label: name                      target for jumps */
    IR_JUMP,    /* goto name                               unconditional branch */
    IR_JUMPZ,   /* if src1 == 0 goto name                  conditional branch on zero */
    IR_PARAM,   /* push argument: src1  (call arg in order) pass one argument to next call */
    IR_CALL,    /* dst = call name, nargs args              call a function */
    IR_RETURN,  /* return src1  (IR_NO_TEMP = void return)  return from function */
    IR_STORE,   /* memory[name] = src1  (store to named local) write a local variable */
    IR_LOAD,    /* dst = memory[name]   (load from named local) read a local variable */
} IROp;

/* ------------------------------------------------------------------ */
/* IRInstr — a single three-address-code instruction                     */
/* ------------------------------------------------------------------ */
typedef struct IRInstr IRInstr;
struct IRInstr {
    IROp     op;        /* which operation this instruction performs */
    int      dst;       /* destination temporary (IR_NO_TEMP if none) */
    int      src1;      /* first source temporary (IR_NO_TEMP if unused) */
    int      src2;      /* second source temporary (IR_NO_TEMP if unused) */
    long     ival;      /* immediate integer value — used by IR_ICONST */
    char     name[64];  /* label name (IR_LABEL/JUMP/JUMPZ), function name (IR_CALL),
                         * or local variable name (IR_STORE/IR_LOAD) */
    int      nargs;     /* number of arguments for IR_CALL */
    IRInstr *next;      /* singly-linked list: pointer to the next instruction */
};

/* ------------------------------------------------------------------ */
/* IRFunc — IR instructions for one function                             */
/* ------------------------------------------------------------------ */
typedef struct {
    char     name[64];   /* the function's source-level name */
    IRInstr *head;       /* first instruction in the linked list */
    IRInstr *tail;       /* last instruction — kept for O(1) append */
    int      next_temp;  /* counter: next temporary number to allocate */
    int      next_label; /* counter: next label number, used to build "L0", "L1", ... */
    int      n_locals;   /* number of local variables declared in this function */
} IRFunc;

/* ------------------------------------------------------------------ */
/* IRProg — the whole program expressed as IR                            */
/* ------------------------------------------------------------------ */
typedef struct {
    IRFunc funcs[32]; /* one entry per compiled function (max 32) */
    int    n_funcs;   /* number of functions stored */
} IRProg;

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

/* Generate IR from a fully parsed and semantically checked AST.
 * Returns a heap-allocated IRProg that the caller must free with ir_free. */
IRProg *irgen(Node *program);

/* Print all IR instructions to stdout in human-readable form.
 * Useful for debugging and the tutorial output. */
void ir_print(const IRProg *prog);

/* Free all memory allocated by irgen (instruction nodes + the IRProg). */
void ir_free(IRProg *prog);

#endif /* MY_COMPILER_IR_H */
