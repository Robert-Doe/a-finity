/*
 * ir.h — Three-address Intermediate Representation
 * Module 11: Code Generation — Control Flow
 *
 * Each instruction: dst = src1 OP src2
 * Temps and locals are identified by integer "slot" numbers.
 * Labels are integer IDs printed as "L<id>".
 */

#ifndef IR_H
#define IR_H

/* All IR operation codes */
typedef enum {
    IR_ICONST,  /* dst = ival                    */
    IR_COPY,    /* dst = src1                    */
    IR_ADD,     /* dst = src1 + src2             */
    IR_SUB,     /* dst = src1 - src2             */
    IR_MUL,     /* dst = src1 * src2             */
    IR_DIV,     /* dst = src1 / src2             */
    IR_MOD,     /* dst = src1 % src2             */
    IR_NEG,     /* dst = -src1                   */
    IR_LT,      /* dst = src1 < src2             */
    IR_GT,      /* dst = src1 > src2             */
    IR_LEQ,     /* dst = src1 <= src2            */
    IR_GEQ,     /* dst = src1 >= src2            */
    IR_EQ,      /* dst = src1 == src2            */
    IR_NEQ,     /* dst = src1 != src2            */
    IR_AND,     /* dst = src1 && src2            */
    IR_OR,      /* dst = src1 || src2            */
    IR_LABEL,   /* define label: name[]          */
    IR_JUMP,    /* unconditional jump: name[]    */
    IR_JUMPZ,   /* jump if src1 == 0: name[]     */
    IR_PARAM,   /* push argument: src1           */
    IR_CALL,    /* dst = call name(nargs params) */
    IR_RETURN,  /* return src1                   */
    IR_STORE,   /* store src1 into local name[]  */
    IR_LOAD     /* dst = load local name[]       */
} IROp;

typedef struct IRInstr {
    IROp          op;
    int           dst;      /* destination temp slot */
    int           src1;     /* first source temp slot */
    int           src2;     /* second source temp slot */
    long          ival;     /* for IR_ICONST */
    char          name[64]; /* for IR_LABEL/JUMP/JUMPZ/CALL/STORE/LOAD */
    int           nargs;    /* for IR_CALL */
    struct IRInstr *next;
} IRInstr;

typedef struct {
    char     name[64];
    IRInstr *head;
    IRInstr *tail;
    int      next_temp;  /* next available temp slot number */
    int      next_label; /* next available label number */
    int      n_locals;   /* number of distinct locals declared */
} IRFunc;

typedef struct {
    IRFunc funcs[32];
    int    n_funcs;
} IRProg;

/* --------------------------------------------------- IR builder API */

/* Generate IR for the AST; returns 1 on success. */
int ir_gen(IRProg *prog, struct Node *ast_prog);

/* Print all IR instructions (for debugging). */
void ir_print(const IRProg *prog);

/* Free IR resources. */
void ir_free(IRProg *prog);

/* Forward reference for ast.h types without a circular include */
struct Node;

#endif /* IR_H */
