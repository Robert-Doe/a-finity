/* ir.c — IR generation for bob_compiler
 * Module 08: IR Generation
 */
#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static IRInstr *ir_new_instr(IROp op) {
    IRInstr *i = malloc(sizeof(IRInstr));
    if (!i) { fprintf(stderr, "ir: OOM\n"); exit(1); }
    memset(i, 0, sizeof(IRInstr));
    i->op   = op;
    i->dst  = IR_NO_TEMP;
    i->src1 = IR_NO_TEMP;
    i->src2 = IR_NO_TEMP;
    return i;
}

static void ir_append(IRFunc *f, IRInstr *i) {
    if (!f->head) { f->head = f->tail = i; }
    else          { f->tail->next = i; f->tail = i; }
    i->next = NULL;
}

static int ir_new_temp(IRFunc *f)  { return f->next_temp++; }
static int ir_new_label(IRFunc *f) { return f->next_label++; }

/* ------------------------------------------------------------------ */
/* gen_expr — generate IR for an expression                             */
/* ------------------------------------------------------------------ */
static int gen_expr(IRFunc *f, Node *n);
static void gen_stmt(IRFunc *f, Node *n);

static int gen_expr(IRFunc *f, Node *n) {
    if (!n) return IR_NO_TEMP;

    switch (n->kind) {

    case AST_INT_LIT: {
        int dst = ir_new_temp(f);
        IRInstr *i = ir_new_instr(IR_ICONST);
        i->dst  = dst;
        i->ival = n->ival;
        ir_append(f, i);
        return dst;
    }

    case AST_IDENT: {
        int dst = ir_new_temp(f);
        IRInstr *i = ir_new_instr(IR_LOAD);
        i->dst = dst;
        strncpy(i->name, n->sval, sizeof(i->name) - 1);
        ir_append(f, i);
        return dst;
    }

    case AST_UNARY: {
        int operand = gen_expr(f, n->left);
        int dst = ir_new_temp(f);
        IRInstr *i = ir_new_instr(IR_NEG);
        i->dst  = dst;
        i->src1 = operand;
        ir_append(f, i);
        return dst;
    }

    case AST_BINARY: {
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
            case '&': op = IR_AND; break;
            case '|': op = IR_OR;  break;
            default:
                fprintf(stderr, "ir: unknown binary op '%c'\n", n->op);
                return IR_NO_TEMP;
        }
        int left  = gen_expr(f, n->left);
        int right = gen_expr(f, n->right);
        int dst   = ir_new_temp(f);
        IRInstr *i = ir_new_instr(op);
        i->dst  = dst;
        i->src1 = left;
        i->src2 = right;
        ir_append(f, i);
        return dst;
    }

    case AST_ASSIGN: {
        int val = gen_expr(f, n->right);
        IRInstr *st = ir_new_instr(IR_STORE);
        st->src1 = val;
        strncpy(st->name, n->left->sval, sizeof(st->name) - 1);
        ir_append(f, st);
        return val;
    }

    case AST_CALL: {
        for (int i = 0; i < n->nargs; i++) {
            int arg = gen_expr(f, n->args[i]);
            IRInstr *p = ir_new_instr(IR_PARAM);
            p->src1 = arg;
            ir_append(f, p);
        }
        int dst = ir_new_temp(f);
        IRInstr *c = ir_new_instr(IR_CALL);
        c->dst   = dst;
        c->nargs = n->nargs;
        strncpy(c->name, n->sval, sizeof(c->name) - 1);
        ir_append(f, c);
        return dst;
    }

    default:
        fprintf(stderr, "ir: gen_expr: unhandled node kind %d\n", n->kind);
        return IR_NO_TEMP;
    }
}

/* ------------------------------------------------------------------ */
/* gen_stmt                                                             */
/* ------------------------------------------------------------------ */

static void gen_stmt(IRFunc *f, Node *n) {
    if (!n) return;

    switch (n->kind) {

    case AST_RETURN: {
        int val = (n->left) ? gen_expr(f, n->left) : IR_NO_TEMP;
        IRInstr *r = ir_new_instr(IR_RETURN);
        r->src1 = val;
        ir_append(f, r);
        break;
    }

    case AST_IF: {
        int else_lbl = ir_new_label(f);
        int end_lbl  = ir_new_label(f);
        char else_name[64], end_name[64];
        snprintf(else_name, sizeof(else_name), "L%d", else_lbl);
        snprintf(end_name,  sizeof(end_name),  "L%d", end_lbl);

        int cond = gen_expr(f, n->left);
        IRInstr *jz = ir_new_instr(IR_JUMPZ);
        jz->src1 = cond;
        strncpy(jz->name, else_name, sizeof(jz->name) - 1);
        ir_append(f, jz);

        gen_stmt(f, n->right);

        IRInstr *jmp = ir_new_instr(IR_JUMP);
        strncpy(jmp->name, end_name, sizeof(jmp->name) - 1);
        ir_append(f, jmp);

        IRInstr *lbl_e = ir_new_instr(IR_LABEL);
        strncpy(lbl_e->name, else_name, sizeof(lbl_e->name) - 1);
        ir_append(f, lbl_e);

        if (n->extra) gen_stmt(f, n->extra);

        IRInstr *lbl_end = ir_new_instr(IR_LABEL);
        strncpy(lbl_end->name, end_name, sizeof(lbl_end->name) - 1);
        ir_append(f, lbl_end);
        break;
    }

    case AST_WHILE: {
        int loop_lbl = ir_new_label(f);
        int end_lbl  = ir_new_label(f);
        char loop_name[64], end_name[64];
        snprintf(loop_name, sizeof(loop_name), "L%d", loop_lbl);
        snprintf(end_name,  sizeof(end_name),  "L%d", end_lbl);

        IRInstr *lbl_loop = ir_new_instr(IR_LABEL);
        strncpy(lbl_loop->name, loop_name, sizeof(lbl_loop->name) - 1);
        ir_append(f, lbl_loop);

        int cond = gen_expr(f, n->left);
        IRInstr *jz = ir_new_instr(IR_JUMPZ);
        jz->src1 = cond;
        strncpy(jz->name, end_name, sizeof(jz->name) - 1);
        ir_append(f, jz);

        gen_stmt(f, n->right);

        IRInstr *jmp = ir_new_instr(IR_JUMP);
        strncpy(jmp->name, loop_name, sizeof(jmp->name) - 1);
        ir_append(f, jmp);

        IRInstr *lbl_end = ir_new_instr(IR_LABEL);
        strncpy(lbl_end->name, end_name, sizeof(lbl_end->name) - 1);
        ir_append(f, lbl_end);
        break;
    }

    case AST_BLOCK:
        for (int i = 0; i < n->nargs; i++) gen_stmt(f, n->args[i]);
        break;

    case AST_VAR_DECL:
        f->n_locals++;
        if (n->left) {
            int val = gen_expr(f, n->left);
            IRInstr *st = ir_new_instr(IR_STORE);
            st->src1 = val;
            strncpy(st->name, n->sval, sizeof(st->name) - 1);
            ir_append(f, st);
        }
        break;

    case AST_EXPR_STMT:
        gen_expr(f, n->left);
        break;

    case AST_ASSIGN:
        gen_expr(f, n);
        break;

    default:
        fprintf(stderr, "ir: gen_stmt: unhandled kind %d\n", n->kind);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* gen_function                                                         */
/* ------------------------------------------------------------------ */

static void gen_function(IRProg *prog, Node *func_node) {
    if (prog->n_funcs >= 32) {
        fprintf(stderr, "ir: too many functions\n");
        return;
    }
    IRFunc *f = &prog->funcs[prog->n_funcs++];
    memset(f, 0, sizeof(IRFunc));
    strncpy(f->name, func_node->sval, sizeof(f->name) - 1);

    /* Parameters: emit IR_LOAD for each (they arrive via the calling convention).
     * We use LOAD with the param name so codegen can map them from registers. */
    for (int i = 0; i < func_node->nargs; i++) {
        const Node *param = func_node->args[i];
        int dst = ir_new_temp(f);
        IRInstr *ld = ir_new_instr(IR_LOAD);
        ld->dst = dst;
        strncpy(ld->name, param->sval, sizeof(ld->name) - 1);
        /* Mark as param load: nargs holds the param index */
        ld->nargs = i;
        ir_append(f, ld);
        /* Store immediately so subsequent LOAD/STORE by name work */
        IRInstr *st = ir_new_instr(IR_STORE);
        st->src1 = dst;
        strncpy(st->name, param->sval, sizeof(st->name) - 1);
        ir_append(f, st);
    }

    /* Generate the function body */
    gen_stmt(f, func_node->extra);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

IRProg *irgen(Node *program) {
    IRProg *prog = malloc(sizeof(IRProg));
    if (!prog) { fprintf(stderr, "ir: OOM\n"); exit(1); }
    memset(prog, 0, sizeof(IRProg));
    for (int i = 0; i < program->nargs; i++)
        if (program->args[i]->kind == AST_FUNC)
            gen_function(prog, program->args[i]);
    return prog;
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
    for (int fi = 0; fi < prog->n_funcs; fi++) {
        const IRFunc *f = &prog->funcs[fi];
        printf("=== IR: %s ===\n", f->name);
        for (IRInstr *instr = f->head; instr; instr = instr->next) {
            printf("  ");
            switch (instr->op) {
                case IR_ICONST:
                    printf("t%d = ICONST %ld\n", instr->dst, instr->ival); break;
                case IR_COPY:
                    printf("t%d = COPY t%d\n",   instr->dst, instr->src1); break;
                case IR_NEG:
                    printf("t%d = NEG t%d\n",    instr->dst, instr->src1); break;
                case IR_LOAD:
                    printf("t%d = LOAD %s\n",    instr->dst, instr->name); break;
                case IR_STORE:
                    printf("STORE %s = t%d\n",   instr->name, instr->src1); break;
                case IR_LABEL:
                    printf("LABEL %s\n",          instr->name); break;
                case IR_JUMP:
                    printf("JUMP %s\n",           instr->name); break;
                case IR_JUMPZ:
                    printf("JUMPZ t%d %s\n",      instr->src1, instr->name); break;
                case IR_PARAM:
                    printf("PARAM t%d\n",         instr->src1); break;
                case IR_CALL:
                    printf("t%d = CALL %s (%d args)\n",
                           instr->dst, instr->name, instr->nargs); break;
                case IR_RETURN:
                    if (instr->src1 == IR_NO_TEMP)
                        printf("RETURN\n");
                    else
                        printf("RETURN t%d\n", instr->src1);
                    break;
                default:
                    printf("t%d = %s t%d t%d\n",
                           instr->dst, op_name(instr->op),
                           instr->src1, instr->src2);
                    break;
            }
        }
        printf("\n");
    }
}

void ir_free(IRProg *prog) {
    if (!prog) return;
    for (int fi = 0; fi < prog->n_funcs; fi++) {
        IRInstr *cur = prog->funcs[fi].head;
        while (cur) {
            IRInstr *nxt = cur->next;
            free(cur);
            cur = nxt;
        }
    }
    free(prog);
}
