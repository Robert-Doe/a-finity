/* ir.c — IR generation from AST for my_compiler
 * Module 08: IR Generation
 */
#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Builder helpers                                                       */
/* ------------------------------------------------------------------ */

typedef struct {
    IRFunc *fn;        /* current function being built */
    /* local variable name -> slot index mapping */
    char   local_names[128][64];
    int    n_locals;
} IRBuilder;

static IRInstr *emit(IRBuilder *b, IROp op) {
    IRInstr *ins = calloc(1, sizeof(IRInstr));
    ins->dst  = IR_NO_TEMP;
    ins->src1 = IR_NO_TEMP;
    ins->src2 = IR_NO_TEMP;
    ins->op   = op;
    if (!b->fn->head) b->fn->head = ins;
    else              b->fn->tail->next = ins;
    b->fn->tail = ins;
    return ins;
}

static int new_temp(IRBuilder *b) {
    return b->fn->next_temp++;
}

static int new_label(IRBuilder *b) {
    return b->fn->next_label++;
}

static void make_label(char *buf, int idx) {
    snprintf(buf, 64, ".L%d", idx);
}

/* Register a local variable name and return its slot index (0-based). */
static int local_slot(IRBuilder *b, const char *name) {
    for (int i = 0; i < b->n_locals; i++)
        if (strcmp(b->local_names[i], name) == 0)
            return i;
    if (b->n_locals < 128) {
        strncpy(b->local_names[b->n_locals], name, 63);
        b->local_names[b->n_locals][63] = '\0';
        b->fn->n_locals++;
        return b->n_locals++;
    }
    return 0; /* should not happen in simple programs */
}

/* ------------------------------------------------------------------ */
/* Expression codegen -> returns temp holding result                     */
/* ------------------------------------------------------------------ */

static int gen_expr(IRBuilder *b, const Node *n);

static int gen_binary_op(IRBuilder *b, const Node *n) {
    int l = gen_expr(b, n->left);
    int r = gen_expr(b, n->right);
    int d = new_temp(b);
    IRInstr *ins = NULL;
    switch (n->op) {
        case '+': ins = emit(b, IR_ADD); break;
        case '-': ins = emit(b, IR_SUB); break;
        case '*': ins = emit(b, IR_MUL); break;
        case '/': ins = emit(b, IR_DIV); break;
        case '%': ins = emit(b, IR_MOD); break;
        case '<': ins = emit(b, IR_LT);  break;
        case '>': ins = emit(b, IR_GT);  break;
        case 'L': ins = emit(b, IR_LEQ); break;
        case 'G': ins = emit(b, IR_GEQ); break;
        case 'E': ins = emit(b, IR_EQ);  break;
        case 'N': ins = emit(b, IR_NEQ); break;
        case '&': ins = emit(b, IR_AND); break;
        case '|': ins = emit(b, IR_OR);  break;
        default:  ins = emit(b, IR_ADD); break;
    }
    ins->dst  = d;
    ins->src1 = l;
    ins->src2 = r;
    return d;
}

static int gen_expr(IRBuilder *b, const Node *n) {
    if (!n) return IR_NO_TEMP;
    switch (n->kind) {
        case AST_INT_LIT: {
            int d = new_temp(b);
            IRInstr *ins = emit(b, IR_ICONST);
            ins->dst  = d;
            ins->ival = n->ival;
            return d;
        }
        case AST_IDENT: {
            int d = new_temp(b);
            IRInstr *ins = emit(b, IR_LOAD);
            ins->dst = d;
            strncpy(ins->name, n->sval, 63);
            return d;
        }
        case AST_UNARY: {
            int src = gen_expr(b, n->left);
            int d   = new_temp(b);
            IRInstr *ins = emit(b, IR_NEG);
            ins->dst  = d;
            ins->src1 = src;
            return d;
        }
        case AST_BINARY:
            return gen_binary_op(b, n);
        case AST_ASSIGN: {
            int src = gen_expr(b, n->right);
            IRInstr *st = emit(b, IR_STORE);
            st->src1 = src;
            strncpy(st->name, n->left->sval, 63);
            local_slot(b, n->left->sval); /* ensure registered */
            /* Also load back so expression has a value */
            int d = new_temp(b);
            IRInstr *ld = emit(b, IR_LOAD);
            ld->dst = d;
            strncpy(ld->name, n->left->sval, 63);
            return d;
        }
        case AST_CALL: {
            /* Emit IR_PARAM for each argument */
            for (int i = 0; i < n->nargs; i++) {
                int at = gen_expr(b, n->args[i]);
                IRInstr *p = emit(b, IR_PARAM);
                p->src1 = at;
            }
            int d = new_temp(b);
            IRInstr *call = emit(b, IR_CALL);
            call->dst   = d;
            call->nargs = n->nargs;
            strncpy(call->name, n->sval, 63);
            return d;
        }
        default:
            return IR_NO_TEMP;
    }
}

/* ------------------------------------------------------------------ */
/* Statement codegen                                                     */
/* ------------------------------------------------------------------ */

static void gen_stmt(IRBuilder *b, const Node *n);

static void gen_stmt(IRBuilder *b, const Node *n) {
    if (!n) return;
    switch (n->kind) {
        case AST_VAR_DECL: {
            local_slot(b, n->sval); /* register the variable */
            if (n->left) {
                int src = gen_expr(b, n->left);
                IRInstr *st = emit(b, IR_STORE);
                st->src1 = src;
                strncpy(st->name, n->sval, 63);
            }
            break;
        }
        case AST_RETURN: {
            if (n->left) {
                int src = gen_expr(b, n->left);
                IRInstr *ret = emit(b, IR_RETURN);
                ret->src1 = src;
            } else {
                IRInstr *ret = emit(b, IR_RETURN);
                ret->src1 = IR_NO_TEMP;
            }
            break;
        }
        case AST_IF: {
            int cond = gen_expr(b, n->left);
            int lelse = new_label(b);
            int lend  = new_label(b);
            IRInstr *jz = emit(b, IR_JUMPZ);
            jz->src1 = cond;
            make_label(jz->name, lelse);

            gen_stmt(b, n->right); /* then */

            if (n->extra) {
                IRInstr *jmp = emit(b, IR_JUMP);
                make_label(jmp->name, lend);
            }

            IRInstr *lbl_else = emit(b, IR_LABEL);
            make_label(lbl_else->name, lelse);

            if (n->extra) {
                gen_stmt(b, n->extra); /* else */
                IRInstr *lbl_end = emit(b, IR_LABEL);
                make_label(lbl_end->name, lend);
            }
            break;
        }
        case AST_WHILE: {
            int lstart = new_label(b);
            int lend   = new_label(b);

            IRInstr *lbl_start = emit(b, IR_LABEL);
            make_label(lbl_start->name, lstart);

            int cond = gen_expr(b, n->left);
            IRInstr *jz = emit(b, IR_JUMPZ);
            jz->src1 = cond;
            make_label(jz->name, lend);

            gen_stmt(b, n->right);

            IRInstr *jmp = emit(b, IR_JUMP);
            make_label(jmp->name, lstart);

            IRInstr *lbl_end = emit(b, IR_LABEL);
            make_label(lbl_end->name, lend);
            break;
        }
        case AST_BLOCK:
            for (int i = 0; i < n->nargs; i++)
                gen_stmt(b, n->args[i]);
            break;
        case AST_EXPR_STMT:
            gen_expr(b, n->left);
            break;
        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Function and program codegen                                          */
/* ------------------------------------------------------------------ */

static void gen_func(IRProg *prog, const Node *fn) {
    if (prog->n_funcs >= 32) return;
    IRFunc *f = &prog->funcs[prog->n_funcs++];
    memset(f, 0, sizeof(*f));
    strncpy(f->name, fn->sval, 63);

    IRBuilder b;
    memset(&b, 0, sizeof(b));
    b.fn = f;

    /* Parameters: register them as locals first so they get the lowest
     * slot indices (0..n_params-1).  Codegen uses this ordering to
     * know which slots correspond to incoming argument registers. */
    for (int i = 0; i < fn->nargs; i++) {
        local_slot(&b, fn->args[i]->sval);
    }

    gen_stmt(&b, fn->extra);

    /* If last instruction is not a RETURN, emit one */
    if (!f->tail || f->tail->op != IR_RETURN) {
        IRInstr *ret = emit(&b, IR_RETURN);
        ret->src1 = IR_NO_TEMP;
    }

    /* Store n_params in next_label AFTER body generation is complete.
     * next_label was used during generation for control-flow labels;
     * it is not needed post-generation so we repurpose it to carry
     * the parameter count to codegen.c without altering ir.h. */
    f->next_label = fn->nargs;
}

IRProg *ir_gen(const Node *prog) {
    IRProg *p = calloc(1, sizeof(IRProg));
    for (int i = 0; i < prog->nargs; i++)
        gen_func(p, prog->args[i]);
    return p;
}

/* ------------------------------------------------------------------ */
/* Printing                                                              */
/* ------------------------------------------------------------------ */

static const char *op_name(IROp op) {
    switch (op) {
        case IR_ICONST: return "ICONST";
        case IR_COPY:   return "COPY";
        case IR_ADD:    return "ADD";
        case IR_SUB:    return "SUB";
        case IR_MUL:    return "MUL";
        case IR_DIV:    return "DIV";
        case IR_MOD:    return "MOD";
        case IR_NEG:    return "NEG";
        case IR_LT:     return "LT";
        case IR_GT:     return "GT";
        case IR_LEQ:    return "LEQ";
        case IR_GEQ:    return "GEQ";
        case IR_EQ:     return "EQ";
        case IR_NEQ:    return "NEQ";
        case IR_AND:    return "AND";
        case IR_OR:     return "OR";
        case IR_LABEL:  return "LABEL";
        case IR_JUMP:   return "JUMP";
        case IR_JUMPZ:  return "JUMPZ";
        case IR_PARAM:  return "PARAM";
        case IR_CALL:   return "CALL";
        case IR_RETURN: return "RETURN";
        case IR_STORE:  return "STORE";
        case IR_LOAD:   return "LOAD";
        default:        return "?";
    }
}

void ir_prog_print(const IRProg *p, FILE *fp) {
    for (int fi = 0; fi < p->n_funcs; fi++) {
        const IRFunc *f = &p->funcs[fi];
        fprintf(fp, "func %s:\n", f->name);
        for (IRInstr *ins = f->head; ins; ins = ins->next) {
            fprintf(fp, "  %-8s", op_name(ins->op));
            if (ins->dst  != IR_NO_TEMP) fprintf(fp, " t%d", ins->dst);
            if (ins->src1 != IR_NO_TEMP) fprintf(fp, " t%d", ins->src1);
            if (ins->src2 != IR_NO_TEMP) fprintf(fp, " t%d", ins->src2);
            if (ins->op == IR_ICONST)    fprintf(fp, " %ld", ins->ival);
            if (ins->name[0])            fprintf(fp, " %s", ins->name);
            if (ins->op == IR_CALL)      fprintf(fp, " (%d args)", ins->nargs);
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }
}

/* ------------------------------------------------------------------ */
/* Freeing                                                               */
/* ------------------------------------------------------------------ */

void ir_prog_free(IRProg *p) {
    if (!p) return;
    for (int fi = 0; fi < p->n_funcs; fi++) {
        IRInstr *ins = p->funcs[fi].head;
        while (ins) {
            IRInstr *next = ins->next;
            free(ins);
            ins = next;
        }
    }
    free(p);
}
