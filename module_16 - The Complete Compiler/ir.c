/*
 * ir.c — IR generation and utilities for mycc
 * Module 16: The Complete Compiler
 */
#include "ir.h"
#include "token.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================ construction */

IRProgram *ir_program_new(void)
{
    IRProgram *p = calloc(1, sizeof *p);
    return p;
}

IRFunc *ir_func_new(const char *name, int param_count)
{
    IRFunc *fn = calloc(1, sizeof *fn);
    fn->name        = strdup(name);
    fn->param_count = param_count;
    fn->next_temp   = 0;
    fn->next_label  = 0;
    return fn;
}

void ir_program_add_func(IRProgram *prog, IRFunc *fn)
{
    if (prog->func_count == prog->func_cap) {
        prog->func_cap = prog->func_cap ? prog->func_cap * 2 : 4;
        prog->funcs = realloc(prog->funcs,
                              (size_t)prog->func_cap * sizeof(IRFunc));
    }
    /* copy by value; fn was heap-allocated but we'll copy its fields */
    prog->funcs[prog->func_count++] = *fn;
    free(fn); /* the copy is in the array; free the shell */
}

IRInstr *ir_emit(IRFunc *fn, IROp op)
{
    if (fn->instr_count == fn->instr_cap) {
        fn->instr_cap = fn->instr_cap ? fn->instr_cap * 2 : 16;
        fn->instrs = realloc(fn->instrs,
                             (size_t)fn->instr_cap * sizeof(IRInstr));
    }
    IRInstr *ins = &fn->instrs[fn->instr_count++];
    memset(ins, 0, sizeof *ins);
    ins->op = op;
    return ins;
}

int ir_new_temp(IRFunc *fn)  { return fn->next_temp++;  }
int ir_new_label(IRFunc *fn) { return fn->next_label++; }

/* ============================================================ operand helpers */

IROperand ir_temp(int t)
{
    IROperand o; o.kind = IR_TEMP; o.u.temp = t; return o;
}
IROperand ir_var(const char *name)
{
    IROperand o; o.kind = IR_VAR; o.u.var = (char*)name; return o;
}
IROperand ir_imm(long v)
{
    IROperand o; o.kind = IR_IMM; o.u.imm = v; return o;
}
IROperand ir_label_ref(int label)
{
    IROperand o; o.kind = IR_LABEL_REF; o.u.label = label; return o;
}
IROperand ir_none(void)
{
    IROperand o; memset(&o, 0, sizeof o); o.kind = IR_IMM; return o;
}

/* ============================================================ IR generation */

/* Returns the temp holding the result of the expression */
static int gen_expr(IRFunc *fn, ASTNode *node);

static int gen_expr(IRFunc *fn, ASTNode *node)
{
    if (!node) return -1;

    switch (node->kind) {
        case AST_INT_LIT: {
            int t = ir_new_temp(fn);
            IRInstr *ins = ir_emit(fn, IR_CONST);
            ins->dst  = ir_temp(t);
            ins->src1 = ir_imm(node->u.ival);
            return t;
        }
        case AST_IDENT: {
            int t = ir_new_temp(fn);
            IRInstr *ins = ir_emit(fn, IR_COPY);
            ins->dst  = ir_temp(t);
            ins->src1 = ir_var(node->u.name);
            return t;
        }
        case AST_ASSIGN: {
            int val = gen_expr(fn, node->u.assign.value);
            IRInstr *ins = ir_emit(fn, IR_COPY);
            ins->dst  = ir_var(node->u.assign.name);
            ins->src1 = ir_temp(val);
            return val;
        }
        case AST_UNOP: {
            int src = gen_expr(fn, node->u.binop.left);
            int t   = ir_new_temp(fn);
            IROp op = (node->u.binop.op == (int)TOK_MINUS) ? IR_NEG : IR_NOT;
            IRInstr *ins = ir_emit(fn, op);
            ins->dst  = ir_temp(t);
            ins->src1 = ir_temp(src);
            return t;
        }
        case AST_BINOP: {
            int l = gen_expr(fn, node->u.binop.left);
            int r = gen_expr(fn, node->u.binop.right);
            int t = ir_new_temp(fn);
            IROp op;
            switch (node->u.binop.op) {
                case TOK_PLUS:    op = IR_ADD; break;
                case TOK_MINUS:   op = IR_SUB; break;
                case TOK_STAR:    op = IR_MUL; break;
                case TOK_SLASH:   op = IR_DIV; break;
                case TOK_PERCENT: op = IR_MOD; break;
                case TOK_LT:      op = IR_LT;  break;
                case TOK_LE:      op = IR_LE;  break;
                case TOK_GT:      op = IR_GT;  break;
                case TOK_GE:      op = IR_GE;  break;
                case TOK_EQ:      op = IR_EQ;  break;
                case TOK_NEQ:     op = IR_NEQ; break;
                case TOK_AND:     op = IR_AND; break;
                case TOK_OR:      op = IR_OR;  break;
                default:          op = IR_ADD; break;
            }
            IRInstr *ins = ir_emit(fn, op);
            ins->dst  = ir_temp(t);
            ins->src1 = ir_temp(l);
            ins->src2 = ir_temp(r);
            return t;
        }
        case AST_CALL: {
            /* emit args first */
            for (int i = 0; i < node->u.call.argc; i++) {
                int at = gen_expr(fn, node->u.call.args[i]);
                IRInstr *arg = ir_emit(fn, IR_ARG);
                arg->src1 = ir_temp(at);
            }
            int t = ir_new_temp(fn);
            IRInstr *ins = ir_emit(fn, IR_CALL);
            ins->dst  = ir_temp(t);
            ins->src1 = ir_var(node->u.call.name);
            ins->src2 = ir_imm(node->u.call.argc);
            return t;
        }
        default:
            return -1;
    }
}

static void gen_stmt(IRFunc *fn, ASTNode *node)
{
    if (!node) return;
    switch (node->kind) {
        case AST_VAR_DECL:
            /* Variables are implicit in the IR; no instruction needed */
            break;
        case AST_EXPR_STMT:
            gen_expr(fn, node->u.expr);
            break;
        case AST_RETURN: {
            int t = node->u.expr ? gen_expr(fn, node->u.expr) : -1;
            IRInstr *ins = ir_emit(fn, IR_RETURN);
            if (t >= 0) ins->src1 = ir_temp(t);
            else        ins->src1 = ir_imm(0);
            break;
        }
        case AST_PRINT: {
            int t = gen_expr(fn, node->u.expr);
            IRInstr *ins = ir_emit(fn, IR_PRINT);
            ins->src1 = ir_temp(t);
            break;
        }
        case AST_IF: {
            int cond = gen_expr(fn, node->u.if_stmt.cond);
            int lbl_else = ir_new_label(fn);
            int lbl_end  = ir_new_label(fn);

            IRInstr *jz = ir_emit(fn, IR_JUMPZ);
            jz->src1 = ir_temp(cond);
            jz->dst  = ir_label_ref(lbl_else);

            gen_stmt(fn, node->u.if_stmt.then_branch);

            if (node->u.if_stmt.else_branch) {
                IRInstr *jmp = ir_emit(fn, IR_JUMP);
                jmp->dst = ir_label_ref(lbl_end);
            }

            IRInstr *lbl = ir_emit(fn, IR_LABEL);
            lbl->dst = ir_label_ref(lbl_else);

            if (node->u.if_stmt.else_branch)
                gen_stmt(fn, node->u.if_stmt.else_branch);

            IRInstr *end = ir_emit(fn, IR_LABEL);
            end->dst = ir_label_ref(lbl_end);
            break;
        }
        case AST_WHILE: {
            int lbl_top  = ir_new_label(fn);
            int lbl_exit = ir_new_label(fn);

            IRInstr *top = ir_emit(fn, IR_LABEL);
            top->dst = ir_label_ref(lbl_top);

            int cond = gen_expr(fn, node->u.while_stmt.cond);
            IRInstr *jz = ir_emit(fn, IR_JUMPZ);
            jz->src1 = ir_temp(cond);
            jz->dst  = ir_label_ref(lbl_exit);

            gen_stmt(fn, node->u.while_stmt.body);

            IRInstr *back = ir_emit(fn, IR_JUMP);
            back->dst = ir_label_ref(lbl_top);

            IRInstr *ex = ir_emit(fn, IR_LABEL);
            ex->dst = ir_label_ref(lbl_exit);
            break;
        }
        case AST_BLOCK:
            for (int i = 0; i < node->u.block.count; i++)
                gen_stmt(fn, node->u.block.stmts[i]);
            break;
        default:
            gen_expr(fn, node);
            break;
    }
}

IRProgram *irgen(ASTNode *program)
{
    IRProgram *prog = ir_program_new();
    for (int i = 0; i < program->u.program.count; i++) {
        ASTNode *fn_node = program->u.program.funcs[i];
        IRFunc *fn = ir_func_new(fn_node->u.func.name,
                                 fn_node->u.func.param_count);
        /* emit PARAM instructions */
        for (int j = 0; j < fn_node->u.func.param_count; j++) {
            IRInstr *ins = ir_emit(fn, IR_PARAM);
            ins->dst = ir_var(fn_node->u.func.params[j]);
        }
        gen_stmt(fn, fn_node->u.func.body);
        ir_program_add_func(prog, fn);
    }
    return prog;
}

/* ============================================================ dump */

static void print_op(const IROperand *o)
{
    switch (o->kind) {
        case IR_TEMP:      printf("t%d", o->u.temp); break;
        case IR_VAR:       printf("%s",  o->u.var);  break;
        case IR_IMM:       printf("%ld", o->u.imm);  break;
        case IR_LABEL_REF: printf("L%d", o->u.label);break;
    }
}

void ir_dump(const IRProgram *prog)
{
    for (int f = 0; f < prog->func_count; f++) {
        const IRFunc *fn = &prog->funcs[f];
        printf("func %s:\n", fn->name);
        for (int i = 0; i < fn->instr_count; i++) {
            const IRInstr *ins = &fn->instrs[i];
            printf("  ");
            switch (ins->op) {
                case IR_LABEL:  printf("L%d:", ins->dst.u.label); break;
                case IR_JUMP:   printf("JUMP L%d", ins->dst.u.label); break;
                case IR_JUMPZ:  printf("JUMPZ "); print_op(&ins->src1);
                                printf(" L%d", ins->dst.u.label); break;
                case IR_CONST:  print_op(&ins->dst); printf(" = %ld", ins->src1.u.imm); break;
                case IR_COPY:   print_op(&ins->dst); printf(" = "); print_op(&ins->src1); break;
                case IR_RETURN: printf("RETURN "); print_op(&ins->src1); break;
                case IR_PARAM:  printf("PARAM "); print_op(&ins->dst); break;
                case IR_ARG:    printf("ARG "); print_op(&ins->src1); break;
                case IR_CALL:   print_op(&ins->dst); printf(" = CALL %s/%ld",
                                    ins->src1.u.var, ins->src2.u.imm); break;
                case IR_PRINT:  printf("PRINT "); print_op(&ins->src1); break;
                case IR_NEG:    print_op(&ins->dst); printf(" = NEG "); print_op(&ins->src1); break;
                case IR_NOT:    print_op(&ins->dst); printf(" = NOT "); print_op(&ins->src1); break;
                default: {
                    static const char *names[] = {
                        "ADD","SUB","MUL","DIV","MOD","???","???",
                        "LT","LE","GT","GE","EQ","NEQ","AND","OR"
                    };
                    print_op(&ins->dst);
                    int idx = (int)ins->op - (int)IR_ADD;
                    printf(" = "); print_op(&ins->src1);
                    if (idx >= 0 && idx < 15)
                        printf(" %s ", names[idx]);
                    else printf(" OP%d ", ins->op);
                    print_op(&ins->src2);
                }
            }
            printf("\n");
        }
        printf("\n");
    }
}

/* ============================================================ free */

void ir_program_free(IRProgram *prog)
{
    if (!prog) return;
    for (int f = 0; f < prog->func_count; f++) {
        IRFunc *fn = &prog->funcs[f];
        free(fn->name);
        free(fn->instrs);
    }
    free(prog->funcs);
    free(prog);
}
