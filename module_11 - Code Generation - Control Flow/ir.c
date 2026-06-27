/*
 * ir.c — IR generation from AST
 * Module 11: Code Generation — Control Flow
 *
 * KEY DESIGN — control flow lowering:
 *
 *   if (cond) { then } else { else }
 *   ─────────────────────────────────
 *     t0 = <cond>
 *     JUMPZ t0, Lelse      ; jump OVER then-block if cond is false (zero)
 *     <then stmts>
 *     JUMP  Lend
 *   Lelse:
 *     <else stmts>
 *   Lend:
 *
 *   while (cond) { body }
 *   ─────────────────────
 *   Ltest:
 *     t0 = <cond>
 *     JUMPZ t0, Lend       ; exit loop if cond is false
 *     <body>
 *     JUMP Ltest
 *   Lend:
 *
 * Note: IR_JUMPZ jumps when the value IS zero (false), which is the
 * opposite of the C "if" semantic. The irgen code handles this inversion
 * by emitting JUMPZ to skip the then-branch, not to enter it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "ast.h"

/* ------------------------------------------------------------------ state */

typedef struct {
    IRFunc *fn;          /* current function being generated */
    /* local variable name → slot mapping */
    char  local_names[128][64];
    int   local_slots[128];
    int   n_locals;
} Gen;

/* ----------------------------------------------------------------- helpers */

static IRInstr *emit(Gen *g, IROp op) {
    IRInstr *ins = calloc(1, sizeof(IRInstr));
    ins->op  = op;
    ins->dst = ins->src1 = ins->src2 = -1;
    if (!g->fn->head) g->fn->head = ins;
    else              g->fn->tail->next = ins;
    g->fn->tail = ins;
    return ins;
}

static int new_temp(Gen *g) {
    return g->fn->next_temp++;
}

static int new_label(Gen *g) {
    return g->fn->next_label++;
}

/* Build the label string "L<id>" into buf (must be >= 16 bytes). */
static void label_str(int id, char *buf) {
    snprintf(buf, 16, "L%d", id);
}

/* Find or allocate a slot for a local variable name. */
static int local_slot(Gen *g, const char *name) {
    for (int i = 0; i < g->n_locals; i++) {
        if (strcmp(g->local_names[i], name) == 0)
            return g->local_slots[i];
    }
    int slot = g->fn->next_temp++;
    strncpy(g->local_names[g->n_locals], name, 63);
    g->local_slots[g->n_locals] = slot;
    g->n_locals++;
    g->fn->n_locals++;
    return slot;
}

/* --------------------------------------------------------- expression codegen */
/* Returns the temp slot holding the result. */
static int gen_expr(Gen *g, Node *n);

static int gen_expr(Gen *g, Node *n) {
    switch (n->kind) {

    case AST_INT_LIT: {
        int t = new_temp(g);
        IRInstr *ins = emit(g, IR_ICONST);
        ins->dst  = t;
        ins->ival = n->ival;
        return t;
    }

    case AST_IDENT: {
        int t = new_temp(g);
        IRInstr *ins = emit(g, IR_LOAD);
        ins->dst = t;
        strncpy(ins->name, n->sval, 63);
        return t;
    }

    case AST_ASSIGN: {
        int src = gen_expr(g, n->right);
        IRInstr *ins = emit(g, IR_STORE);
        ins->src1 = src;
        strncpy(ins->name, n->sval, 63);
        /* Return the value so assignments can be used in expressions. */
        int t = new_temp(g);
        IRInstr *ld = emit(g, IR_LOAD);
        ld->dst = t;
        strncpy(ld->name, n->sval, 63);
        return t;
    }

    case AST_UNARY: {
        int src = gen_expr(g, n->left);
        int t   = new_temp(g);
        if (n->op == '-') {
            IRInstr *ins = emit(g, IR_NEG);
            ins->dst  = t;
            ins->src1 = src;
        } else { /* '!' */
            /* !x  →  t = (x == 0) */
            int zero = new_temp(g);
            IRInstr *ci = emit(g, IR_ICONST);
            ci->dst = zero; ci->ival = 0;
            IRInstr *ins = emit(g, IR_EQ);
            ins->dst  = t;
            ins->src1 = src;
            ins->src2 = zero;
        }
        return t;
    }

    case AST_BINARY: {
        /* Short-circuit &&  ||  handled specially */
        if (n->op == '&') { /* && */
            /*
             * t = left; if t == 0, skip right; t = right
             * Emit: t=left; jumpz t, Lfalse; t=right; Lfalse:
             * result = (t != 0)
             */
            int t    = new_temp(g);
            int left = gen_expr(g, n->left);
            IRInstr *cp1 = emit(g, IR_COPY);
            cp1->dst = t; cp1->src1 = left;
            int lfalse = new_label(g);
            char lf[16]; label_str(lfalse, lf);
            IRInstr *jz = emit(g, IR_JUMPZ);
            jz->src1 = t;
            strncpy(jz->name, lf, 63);
            int right = gen_expr(g, n->right);
            IRInstr *cp2 = emit(g, IR_COPY);
            cp2->dst = t; cp2->src1 = right;
            IRInstr *lbl = emit(g, IR_LABEL);
            strncpy(lbl->name, lf, 63);
            return t;
        }
        if (n->op == '|') { /* || */
            int t    = new_temp(g);
            int left = gen_expr(g, n->left);
            IRInstr *cp1 = emit(g, IR_COPY);
            cp1->dst = t; cp1->src1 = left;
            int ltrue = new_label(g);
            char lt[16]; label_str(ltrue, lt);
            /* jump past right eval if already true (non-zero) */
            /* JUMPZ skips when zero; we want to skip when NON-zero:
               negate: tmp2 = (t == 0); JUMPZ tmp2, Ltrue */
            int inv = new_temp(g);
            int zero = new_temp(g);
            IRInstr *ci = emit(g, IR_ICONST); ci->dst = zero; ci->ival = 0;
            IRInstr *eq = emit(g, IR_EQ); eq->dst = inv; eq->src1 = t; eq->src2 = zero;
            IRInstr *jz = emit(g, IR_JUMPZ);
            jz->src1 = inv; strncpy(jz->name, lt, 63);
            int right = gen_expr(g, n->right);
            IRInstr *cp2 = emit(g, IR_COPY); cp2->dst = t; cp2->src1 = right;
            IRInstr *lbl = emit(g, IR_LABEL); strncpy(lbl->name, lt, 63);
            return t;
        }

        int left  = gen_expr(g, n->left);
        int right = gen_expr(g, n->right);
        int t     = new_temp(g);
        IROp op;
        switch (n->op) {
        case '+': op = IR_ADD; break;
        case '-': op = IR_SUB; break;
        case '*': op = IR_MUL; break;
        case '/': op = IR_DIV; break;
        case '%': op = IR_MOD; break;
        case '<': op = IR_LT;  break;
        case '>': op = IR_GT;  break;
        case 'L': op = IR_LEQ; break;
        case 'G': op = IR_GEQ; break;
        case 'E': op = IR_EQ;  break;
        case 'N': op = IR_NEQ; break;
        default:
            fprintf(stderr, "ir: unknown binary op '%c'\n", n->op);
            op = IR_ADD;
        }
        IRInstr *ins = emit(g, op);
        ins->dst  = t;
        ins->src1 = left;
        ins->src2 = right;
        return t;
    }

    case AST_CALL: {
        /* Emit IR_PARAM for each argument, then IR_CALL */
        for (int i = 0; i < n->nargs; i++) {
            int arg = gen_expr(g, n->args[i]);
            IRInstr *p = emit(g, IR_PARAM);
            p->src1 = arg;
        }
        int t = new_temp(g);
        IRInstr *c = emit(g, IR_CALL);
        c->dst   = t;
        c->nargs = n->nargs;
        strncpy(c->name, n->sval, 63);
        return t;
    }

    default:
        fprintf(stderr, "ir: unhandled expression kind %d\n", n->kind);
        return new_temp(g);
    }
}

/* --------------------------------------------------------- statement codegen */

static void gen_stmt(Gen *g, Node *n) {
    if (!n) return;
    switch (n->kind) {

    case AST_VAR_DECL: {
        /* Allocate a slot for this local */
        (void)local_slot(g, n->sval);
        /* Make the IR_STORE/LOAD use the name (codegen will map it to a slot) */
        if (n->right) {
            int val = gen_expr(g, n->right);
            IRInstr *s = emit(g, IR_STORE);
            s->src1 = val;
            strncpy(s->name, n->sval, 63);
        }
        break;
    }

    case AST_EXPR_STMT:
        gen_expr(g, n->left);
        break;

    case AST_RETURN: {
        if (n->left) {
            int val = gen_expr(g, n->left);
            IRInstr *r = emit(g, IR_RETURN);
            r->src1 = val;
        } else {
            int zero = new_temp(g);
            IRInstr *ci = emit(g, IR_ICONST); ci->dst = zero; ci->ival = 0;
            IRInstr *r  = emit(g, IR_RETURN);  r->src1 = zero;
        }
        break;
    }

    case AST_IF: {
        /*
         * Translate:
         *   if (cond) { then } else { else }
         *
         *   t = cond
         *   JUMPZ t, Lelse     ; jump to else when condition is FALSE (zero)
         *   <then stmts>
         *   JUMP Lend
         * Lelse:
         *   <else stmts>
         * Lend:
         */
        int lelse_id = new_label(g);
        int lend_id  = new_label(g);
        char lelse[16], lend[16];
        label_str(lelse_id, lelse);
        label_str(lend_id,  lend);

        int cond = gen_expr(g, n->left);
        IRInstr *jz = emit(g, IR_JUMPZ);
        jz->src1 = cond;
        strncpy(jz->name, lelse, 63);

        /* then block */
        Node *then_blk = n->right;
        for (int i = 0; i < then_blk->nargs; i++) gen_stmt(g, then_blk->args[i]);

        IRInstr *jmp = emit(g, IR_JUMP);
        strncpy(jmp->name, lend, 63);

        IRInstr *lbl_else = emit(g, IR_LABEL);
        strncpy(lbl_else->name, lelse, 63);

        /* else block (may be NULL) */
        if (n->extra) {
            Node *else_blk = n->extra;
            for (int i = 0; i < else_blk->nargs; i++) gen_stmt(g, else_blk->args[i]);
        }

        IRInstr *lbl_end = emit(g, IR_LABEL);
        strncpy(lbl_end->name, lend, 63);
        break;
    }

    case AST_WHILE: {
        /*
         * Translate:
         *   while (cond) { body }
         *
         * Ltest:
         *   t = cond
         *   JUMPZ t, Lend   ; exit loop when condition is FALSE (zero)
         *   <body stmts>
         *   JUMP Ltest
         * Lend:
         */
        int ltest_id = new_label(g);
        int lend_id  = new_label(g);
        char ltest[16], lend[16];
        label_str(ltest_id, ltest);
        label_str(lend_id,  lend);

        IRInstr *lbl_test = emit(g, IR_LABEL);
        strncpy(lbl_test->name, ltest, 63);

        int cond = gen_expr(g, n->left);
        IRInstr *jz = emit(g, IR_JUMPZ);
        jz->src1 = cond;
        strncpy(jz->name, lend, 63);

        /* body */
        Node *body = n->right;
        for (int i = 0; i < body->nargs; i++) gen_stmt(g, body->args[i]);

        IRInstr *jmp = emit(g, IR_JUMP);
        strncpy(jmp->name, ltest, 63);

        IRInstr *lbl_end = emit(g, IR_LABEL);
        strncpy(lbl_end->name, lend, 63);
        break;
    }

    case AST_BLOCK:
        for (int i = 0; i < n->nargs; i++) gen_stmt(g, n->args[i]);
        break;

    default:
        fprintf(stderr, "ir: unhandled statement kind %d\n", n->kind);
        break;
    }
}

/* --------------------------------------------------------- function codegen */

static void gen_func(IRProg *prog, Node *fn) {
    if (prog->n_funcs >= 32) {
        fprintf(stderr, "ir: too many functions\n");
        return;
    }
    IRFunc *irfn = &prog->funcs[prog->n_funcs++];
    memset(irfn, 0, sizeof(*irfn));
    strncpy(irfn->name, fn->sval, 63);

    Gen g;
    memset(&g, 0, sizeof(g));
    g.fn = irfn;

    /* Allocate slots for parameters and emit LOAD stubs */
    for (int i = 0; i < fn->nargs; i++) {
        const char *pname = fn->args[i]->sval;
        (void)local_slot(&g, pname);
        /* The parameters arrive in registers; we store them into locals */
        IRInstr *s = emit(&g, IR_STORE);
        s->src1 = i; /* param index — codegen will map to register */
        s->nargs = -1; /* sentinel: this is a param store */
        strncpy(s->name, pname, 63);
    }

    /* Generate body */
    Node *body = fn->right;
    if (body && body->kind == AST_BLOCK) {
        for (int i = 0; i < body->nargs; i++) gen_stmt(&g, body->args[i]);
    }
}

/* --------------------------------------------------------- public API */

int ir_gen(IRProg *prog, Node *ast_prog) {
    memset(prog, 0, sizeof(*prog));
    for (int i = 0; i < ast_prog->nargs; i++) {
        gen_func(prog, ast_prog->args[i]);
    }
    return 1;
}

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
    default:        return "???";
    }
}

void ir_print(const IRProg *prog) {
    for (int f = 0; f < prog->n_funcs; f++) {
        const IRFunc *fn = &prog->funcs[f];
        printf("FUNC %s:\n", fn->name);
        for (IRInstr *ins = fn->head; ins; ins = ins->next) {
            printf("  %-8s", op_name(ins->op));
            switch (ins->op) {
            case IR_ICONST: printf("t%d = %ld\n",       ins->dst, ins->ival); break;
            case IR_COPY:   printf("t%d = t%d\n",       ins->dst, ins->src1); break;
            case IR_NEG:    printf("t%d = -t%d\n",      ins->dst, ins->src1); break;
            case IR_ADD: case IR_SUB: case IR_MUL:
            case IR_DIV: case IR_MOD:
            case IR_LT:  case IR_GT:  case IR_LEQ:
            case IR_GEQ: case IR_EQ:  case IR_NEQ:
            case IR_AND: case IR_OR:
                printf("t%d = t%d, t%d\n", ins->dst, ins->src1, ins->src2); break;
            case IR_LABEL:  printf("%s:\n",            ins->name); break;
            case IR_JUMP:   printf("-> %s\n",          ins->name); break;
            case IR_JUMPZ:  printf("t%d -> %s\n",      ins->src1, ins->name); break;
            case IR_PARAM:  printf("t%d\n",            ins->src1); break;
            case IR_CALL:   printf("t%d = %s(%d)\n",   ins->dst, ins->name, ins->nargs); break;
            case IR_RETURN: printf("t%d\n",            ins->src1); break;
            case IR_STORE:  printf("%s = t%d\n",       ins->name, ins->src1); break;
            case IR_LOAD:   printf("t%d = %s\n",       ins->dst, ins->name); break;
            default:        printf("\n"); break;
            }
        }
        printf("\n");
    }
}

void ir_free(IRProg *prog) {
    for (int f = 0; f < prog->n_funcs; f++) {
        IRInstr *ins = prog->funcs[f].head;
        while (ins) {
            IRInstr *next = ins->next;
            free(ins);
            ins = next;
        }
    }
}
