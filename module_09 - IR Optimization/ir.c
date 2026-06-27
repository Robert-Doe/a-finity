/* ir.c — IR generation implementation for my_compiler
 * Module 08: IR Generation (carried forward to Module 09)
 * Prerequisites: source.h, token.h, lexer.h, symtab.h, ast.h, parser.h, sema.h, ir.h
 */
#include "ir.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helper: ir_new_instr                                         */
/* ------------------------------------------------------------------ */

static IRInstr *ir_new_instr(IROp op) {
    IRInstr *instr = malloc(sizeof(IRInstr));
    if (!instr) { fprintf(stderr, "ir: out of memory\n"); exit(1); }
    memset(instr, 0, sizeof(IRInstr));
    instr->op   = op;
    instr->dst  = IR_NO_TEMP;
    instr->src1 = IR_NO_TEMP;
    instr->src2 = IR_NO_TEMP;
    return instr;
}

/* ------------------------------------------------------------------ */
/* Internal helper: ir_append                                            */
/* ------------------------------------------------------------------ */

static void ir_append(IRFunc *f, IRInstr *instr) {
    if (!f->head) {
        f->head = f->tail = instr;
    } else {
        f->tail->next = instr;
        f->tail       = instr;
    }
    instr->next = NULL;
}

/* ------------------------------------------------------------------ */
/* Internal helper: ir_new_temp                                          */
/* ------------------------------------------------------------------ */

static int ir_new_temp(IRFunc *f) {
    return f->next_temp++;
}

/* ------------------------------------------------------------------ */
/* Internal helper: ir_new_label                                         */
/* ------------------------------------------------------------------ */

static int ir_new_label(IRFunc *f) {
    return f->next_label++;
}

/* ------------------------------------------------------------------ */
/* gen_expr — generate IR for an expression node                         */
/* ------------------------------------------------------------------ */
static int gen_expr(IRFunc *f, Node *n);
static void gen_stmt(IRFunc *f, Node *n);

static int gen_expr(IRFunc *f, Node *n) {
    if (!n) return IR_NO_TEMP;

    switch (n->kind) {

    case AST_INT_LIT: {
        int dst = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(IR_ICONST);
        instr->dst  = dst;
        instr->ival = n->ival;
        ir_append(f, instr);
        return dst;
    }

    case AST_IDENT: {
        int dst = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(IR_LOAD);
        instr->dst = dst;
        strncpy(instr->name, n->name, sizeof(instr->name) - 1);
        ir_append(f, instr);
        return dst;
    }

    case AST_UNARY: {
        int operand = gen_expr(f, n->args[0]);
        int dst = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(IR_NEG);
        instr->dst  = dst;
        instr->src1 = operand;
        ir_append(f, instr);
        return dst;
    }

    case AST_BINARY: {
        IROp op;
        switch (n->op) {
            case TOK_PLUS:    op = IR_ADD; break;
            case TOK_MINUS:   op = IR_SUB; break;
            case TOK_STAR:    op = IR_MUL; break;
            case TOK_SLASH:   op = IR_DIV; break;
            case TOK_PERCENT: op = IR_MOD; break;
            case TOK_LT:      op = IR_LT;  break;
            case TOK_GT:      op = IR_GT;  break;
            case TOK_LEQ:     op = IR_LEQ; break;
            case TOK_GEQ:     op = IR_GEQ; break;
            case TOK_EQEQ:    op = IR_EQ;  break;
            case TOK_NEQ:     op = IR_NEQ; break;
            case TOK_AND:     op = IR_AND; break;
            case TOK_OR:      op = IR_OR;  break;
            default:
                fprintf(stderr, "ir: unknown binary op %d\n", n->op);
                return IR_NO_TEMP;
        }
        int left  = gen_expr(f, n->args[0]);
        int right = gen_expr(f, n->args[1]);
        int dst   = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(op);
        instr->dst  = dst;
        instr->src1 = left;
        instr->src2 = right;
        ir_append(f, instr);
        return dst;
    }

    case AST_ASSIGN: {
        int val = gen_expr(f, n->args[0]);
        IRInstr *store = ir_new_instr(IR_STORE);
        store->src1 = val;
        strncpy(store->name, n->name, sizeof(store->name) - 1);
        ir_append(f, store);
        return val;
    }

    case AST_CALL: {
        for (int i = 0; i < n->n_args; i++) {
            int arg = gen_expr(f, n->args[i]);
            IRInstr *param = ir_new_instr(IR_PARAM);
            param->src1 = arg;
            ir_append(f, param);
        }
        int dst = ir_new_temp(f);
        IRInstr *call = ir_new_instr(IR_CALL);
        call->dst   = dst;
        call->nargs = n->n_args;
        strncpy(call->name, n->name, sizeof(call->name) - 1);
        ir_append(f, call);
        return dst;
    }

    default:
        fprintf(stderr, "ir: gen_expr: unhandled node kind %d\n", n->kind);
        return IR_NO_TEMP;
    }
}

static void gen_stmt(IRFunc *f, Node *n) {
    if (!n) return;

    switch (n->kind) {

    case AST_RETURN: {
        int val = IR_NO_TEMP;
        if (n->n_args > 0) {
            val = gen_expr(f, n->args[0]);
        }
        IRInstr *ret = ir_new_instr(IR_RETURN);
        ret->src1 = val;
        ir_append(f, ret);
        break;
    }

    case AST_IF: {
        int else_lbl = ir_new_label(f);
        int end_lbl  = ir_new_label(f);
        char else_name[64], end_name[64];
        snprintf(else_name, sizeof(else_name), "L%d", else_lbl);
        snprintf(end_name,  sizeof(end_name),  "L%d", end_lbl);

        int cond = gen_expr(f, n->args[0]);

        IRInstr *jumpz = ir_new_instr(IR_JUMPZ);
        jumpz->src1 = cond;
        strncpy(jumpz->name, else_name, sizeof(jumpz->name) - 1);
        ir_append(f, jumpz);

        gen_stmt(f, n->args[1]);

        IRInstr *jump = ir_new_instr(IR_JUMP);
        strncpy(jump->name, end_name, sizeof(jump->name) - 1);
        ir_append(f, jump);

        IRInstr *lbl_else = ir_new_instr(IR_LABEL);
        strncpy(lbl_else->name, else_name, sizeof(lbl_else->name) - 1);
        ir_append(f, lbl_else);

        if (n->n_args > 2) {
            gen_stmt(f, n->args[2]);
        }

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

        int cond = gen_expr(f, n->args[0]);
        IRInstr *jumpz = ir_new_instr(IR_JUMPZ);
        jumpz->src1 = cond;
        strncpy(jumpz->name, end_name, sizeof(jumpz->name) - 1);
        ir_append(f, jumpz);

        gen_stmt(f, n->args[1]);

        IRInstr *jump = ir_new_instr(IR_JUMP);
        strncpy(jump->name, loop_name, sizeof(jump->name) - 1);
        ir_append(f, jump);

        IRInstr *lbl_end = ir_new_instr(IR_LABEL);
        strncpy(lbl_end->name, end_name, sizeof(lbl_end->name) - 1);
        ir_append(f, lbl_end);
        break;
    }

    case AST_BLOCK:
        for (int i = 0; i < n->n_args; i++) {
            gen_stmt(f, n->args[i]);
        }
        break;

    case AST_VAR_DECL:
        f->n_locals++;
        if (n->n_args > 0) {
            int val = gen_expr(f, n->args[0]);
            IRInstr *store = ir_new_instr(IR_STORE);
            store->src1 = val;
            strncpy(store->name, n->name, sizeof(store->name) - 1);
            ir_append(f, store);
        }
        break;

    case AST_EXPR_STMT:
        gen_expr(f, n->args[0]);
        break;

    case AST_ASSIGN:
        gen_expr(f, n);
        break;

    default:
        fprintf(stderr, "ir: gen_stmt: unhandled node kind %d\n", n->kind);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* gen_function                                                          */
/* ------------------------------------------------------------------ */
static void gen_function(IRProg *prog, Node *func_node) {
    if (prog->n_funcs >= 32) {
        fprintf(stderr, "ir: too many functions (max 32)\n");
        return;
    }

    IRFunc *f = &prog->funcs[prog->n_funcs++];
    memset(f, 0, sizeof(IRFunc));
    strncpy(f->name, func_node->name, sizeof(f->name) - 1);

    int body_idx = func_node->n_args - 1;
    gen_stmt(f, func_node->args[body_idx]);
}

/* ------------------------------------------------------------------ */
/* irgen — entry point                                                   */
/* ------------------------------------------------------------------ */
IRProg *irgen(Node *program) {
    IRProg *prog = malloc(sizeof(IRProg));
    if (!prog) { fprintf(stderr, "ir: out of memory\n"); exit(1); }
    memset(prog, 0, sizeof(IRProg));

    for (int i = 0; i < program->n_args; i++) {
        if (program->args[i]->kind == AST_FUNC) {
            gen_function(prog, program->args[i]);
        }
    }
    return prog;
}

/* ------------------------------------------------------------------ */
/* ir_print                                                              */
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
                    printf("t%d = ICONST %ld\n", instr->dst, instr->ival);
                    break;
                case IR_COPY:
                    printf("t%d = COPY t%d\n", instr->dst, instr->src1);
                    break;
                case IR_NEG:
                    printf("t%d = NEG t%d\n", instr->dst, instr->src1);
                    break;
                case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
                case IR_LT:  case IR_GT:  case IR_LEQ: case IR_GEQ:
                case IR_EQ:  case IR_NEQ: case IR_AND: case IR_OR:
                    printf("t%d = %s t%d t%d\n",
                           instr->dst, op_name(instr->op), instr->src1, instr->src2);
                    break;
                case IR_LOAD:
                    printf("t%d = LOAD %s\n", instr->dst, instr->name);
                    break;
                case IR_STORE:
                    printf("STORE %s = t%d\n", instr->name, instr->src1);
                    break;
                case IR_LABEL:
                    printf("LABEL %s\n", instr->name);
                    break;
                case IR_JUMP:
                    printf("JUMP %s\n", instr->name);
                    break;
                case IR_JUMPZ:
                    printf("JUMPZ t%d %s\n", instr->src1, instr->name);
                    break;
                case IR_PARAM:
                    printf("PARAM t%d\n", instr->src1);
                    break;
                case IR_CALL:
                    printf("t%d = CALL %s (%d args)\n",
                           instr->dst, instr->name, instr->nargs);
                    break;
                case IR_RETURN:
                    if (instr->src1 == IR_NO_TEMP)
                        printf("RETURN\n");
                    else
                        printf("RETURN t%d\n", instr->src1);
                    break;
                default:
                    printf("%s\n", op_name(instr->op));
                    break;
            }
        }
        printf("\n");
    }
}

/* ------------------------------------------------------------------ */
/* ir_free                                                               */
/* ------------------------------------------------------------------ */
void ir_free(IRProg *prog) {
    if (!prog) return;
    for (int fi = 0; fi < prog->n_funcs; fi++) {
        IRInstr *cur = prog->funcs[fi].head;
        while (cur) {
            IRInstr *next = cur->next;
            free(cur);
            cur = next;
        }
    }
    free(prog);
}
