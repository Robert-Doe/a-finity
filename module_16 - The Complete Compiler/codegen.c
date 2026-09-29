/*
 * codegen.c — NASM x86-64 code generator for mycc
 * Module 16: The Complete Compiler
 *
 * Strategy:
 *   - Each IR temp and named variable gets a slot on the stack.
 *   - We use a simple greedy allocator: temps live in [rbp - offset].
 *   - System V AMD64 ABI calling convention for function calls.
 *   - print_int is declared extern and provided by runtime.asm.
 */
#include "codegen.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ======================================================= helpers */

/* System V AMD64 argument registers (first 6 integer args) */
static const char *ARG_REGS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
#define MAX_ARG_REGS 6

/* Map a temp id to a stack offset (from rbp).
   Offsets start at -8, grow downward. */
typedef struct {
    char  *name;        /* variable name (for IR_VAR) or NULL for temps */
    int    temp_id;     /* -1 for named vars */
    int    offset;      /* bytes from rbp, negative */
} SlotEntry;

typedef struct {
    SlotEntry *slots;
    int        count;
    int        cap;
    int        next_offset;  /* next free offset (starts at -8, decrements by 8) */
} SlotMap;

static void slot_init(SlotMap *m)
{
    m->slots       = NULL;
    m->count       = 0;
    m->cap         = 0;
    m->next_offset = -8;
}

static void slot_free(SlotMap *m)
{
    for (int i = 0; i < m->count; i++) free(m->slots[i].name);
    free(m->slots);
}

static int slot_for_temp(SlotMap *m, int temp_id)
{
    for (int i = 0; i < m->count; i++)
        if (m->slots[i].temp_id == temp_id)
            return m->slots[i].offset;

    if (m->count == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 16;
        m->slots = realloc(m->slots, (size_t)m->cap * sizeof(SlotEntry));
    }
    SlotEntry *e = &m->slots[m->count++];
    e->name    = NULL;
    e->temp_id = temp_id;
    e->offset  = m->next_offset;
    m->next_offset -= 8;
    return e->offset;
}

static int slot_for_var(SlotMap *m, const char *name)
{
    for (int i = 0; i < m->count; i++)
        if (m->slots[i].name && strcmp(m->slots[i].name, name) == 0)
            return m->slots[i].offset;

    if (m->count == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 16;
        m->slots = realloc(m->slots, (size_t)m->cap * sizeof(SlotEntry));
    }
    SlotEntry *e = &m->slots[m->count++];
    e->name    = strdup(name);
    e->temp_id = -1;
    e->offset  = m->next_offset;
    m->next_offset -= 8;
    return e->offset;
}

/* Load an IROperand into rax */
static void load_op_to_rax(FILE *out, SlotMap *m, const IROperand *op)
{
    switch (op->kind) {
        case IR_IMM:
            fprintf(out, "    mov     rax, %ld\n", op->u.imm);
            break;
        case IR_TEMP:
            fprintf(out, "    mov     rax, [rbp%+d]\n", slot_for_temp(m, op->u.temp));
            break;
        case IR_VAR:
            fprintf(out, "    mov     rax, [rbp%+d]\n", slot_for_var(m, op->u.var));
            break;
        default:
            break;
    }
}

/* Store rax into IROperand destination */
static void store_rax_to_dst(FILE *out, SlotMap *m, const IROperand *dst)
{
    switch (dst->kind) {
        case IR_TEMP:
            fprintf(out, "    mov     [rbp%+d], rax\n", slot_for_temp(m, dst->u.temp));
            break;
        case IR_VAR:
            fprintf(out, "    mov     [rbp%+d], rax\n", slot_for_var(m, dst->u.var));
            break;
        default:
            break;
    }
}

/* ======================================================= first pass: compute stack size */

static int compute_stack_size(const IRFunc *fn)
{
    SlotMap m;
    slot_init(&m);

    /* Pre-assign parameter slots */
    for (int i = 0; i < fn->instr_count; i++) {
        const IRInstr *ins = &fn->instrs[i];
        if (ins->op == IR_PARAM)
            slot_for_var(&m, ins->dst.u.var);
    }

    for (int i = 0; i < fn->instr_count; i++) {
        const IRInstr *ins = &fn->instrs[i];
        if (ins->dst.kind == IR_TEMP)
            slot_for_temp(&m, ins->dst.u.temp);
        if (ins->src1.kind == IR_TEMP)
            slot_for_temp(&m, ins->src1.u.temp);
        if (ins->src2.kind == IR_TEMP)
            slot_for_temp(&m, ins->src2.u.temp);
        /* Named var writes */
        if (ins->dst.kind == IR_VAR)
            slot_for_var(&m, ins->dst.u.var);
        if (ins->src1.kind == IR_VAR)
            slot_for_var(&m, ins->src1.u.var);
    }

    int size = -m.next_offset; /* next_offset is negative */
    /* Align to 16 bytes */
    if (size % 16 != 0) size += 16 - (size % 16);
    slot_free(&m);
    return size;
}

/* ======================================================= codegen for one function */

static void codegen_func(const IRFunc *fn, FILE *out)
{
    int stack_size = compute_stack_size(fn);
    /* Ensure stack is 16-byte aligned after push rbp */
    if ((stack_size % 16) != 0) stack_size += 8;

    fprintf(out, "global %s\n", fn->name);
    fprintf(out, "%s:\n", fn->name);
    fprintf(out, "    push    rbp\n");
    fprintf(out, "    mov     rbp, rsp\n");
    fprintf(out, "    sub     rsp, %d\n\n", stack_size);

    /* Build slot map again for actual codegen */
    SlotMap m;
    slot_init(&m);

    /* Store incoming arguments from registers to their stack slots */
    int param_idx = 0;
    for (int i = 0; i < fn->instr_count; i++) {
        const IRInstr *ins = &fn->instrs[i];
        if (ins->op == IR_PARAM) {
            int off = slot_for_var(&m, ins->dst.u.var);
            if (param_idx < MAX_ARG_REGS)
                fprintf(out, "    mov     [rbp%+d], %s   ; param %s\n",
                        off, ARG_REGS[param_idx], ins->dst.u.var);
            else
                fprintf(out, "    ; param %s is stack-passed (not implemented)\n",
                        ins->dst.u.var);
            param_idx++;
        }
    }
    if (param_idx > 0) fprintf(out, "\n");

    /* Track pending args for calls */
    int pending_args[16];
    int n_pending = 0;

    for (int i = 0; i < fn->instr_count; i++) {
        const IRInstr *ins = &fn->instrs[i];

        switch (ins->op) {
            case IR_PARAM:
                /* already handled above */
                break;

            case IR_CONST:
                fprintf(out, "    mov     rax, %ld\n", ins->src1.u.imm);
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_COPY:
                load_op_to_rax(out, &m, &ins->src1);
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_ADD:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     rcx, rax\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    add     rax, rcx\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_SUB:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     rcx, rax\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    sub     rcx, rax\n");
                fprintf(out, "    mov     rax, rcx\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_MUL:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     rcx, rax\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    imul    rax, rcx\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_DIV:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     r10, rax\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    mov     r11, rax\n");
                fprintf(out, "    mov     rax, r10\n");
                fprintf(out, "    cqo\n");
                fprintf(out, "    idiv    r11\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_MOD:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     r10, rax\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    mov     r11, rax\n");
                fprintf(out, "    mov     rax, r10\n");
                fprintf(out, "    cqo\n");
                fprintf(out, "    idiv    r11\n");
                fprintf(out, "    mov     rax, rdx\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_NEG:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    neg     rax\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_NOT:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    test    rax, rax\n");
                fprintf(out, "    setz    al\n");
                fprintf(out, "    movzx   rax, al\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_LT: case IR_LE: case IR_GT: case IR_GE:
            case IR_EQ: case IR_NEQ: {
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     rcx, rax\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    cmp     rcx, rax\n");
                const char *setcc =
                    ins->op == IR_LT  ? "setl"  :
                    ins->op == IR_LE  ? "setle" :
                    ins->op == IR_GT  ? "setg"  :
                    ins->op == IR_GE  ? "setge" :
                    ins->op == IR_EQ  ? "sete"  : "setne";
                fprintf(out, "    %s    al\n", setcc);
                fprintf(out, "    movzx   rax, al\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;
            }

            case IR_AND:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    test    rax, rax\n");
                fprintf(out, "    setnz   cl\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    test    rax, rax\n");
                fprintf(out, "    setnz   al\n");
                fprintf(out, "    and     al, cl\n");
                fprintf(out, "    movzx   rax, al\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_OR:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    test    rax, rax\n");
                fprintf(out, "    setnz   cl\n");
                load_op_to_rax(out, &m, &ins->src2);
                fprintf(out, "    test    rax, rax\n");
                fprintf(out, "    setnz   al\n");
                fprintf(out, "    or      al, cl\n");
                fprintf(out, "    movzx   rax, al\n");
                store_rax_to_dst(out, &m, &ins->dst);
                break;

            case IR_LABEL:
                fprintf(out, ".L%d:\n", ins->dst.u.label);
                break;

            case IR_JUMP:
                fprintf(out, "    jmp     .L%d\n", ins->dst.u.label);
                break;

            case IR_JUMPZ:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    test    rax, rax\n");
                fprintf(out, "    jz      .L%d\n", ins->dst.u.label);
                break;

            case IR_ARG:
                if (n_pending < 16)
                    pending_args[n_pending++] = ins->src1.u.temp;
                break;

            case IR_CALL: {
                int argc = (int)ins->src2.u.imm;
                /* Move args from pending_args into argument registers */
                for (int j = 0; j < argc && j < MAX_ARG_REGS; j++) {
                    int arg_t = pending_args[j];
                    fprintf(out, "    mov     %s, [rbp%+d]\n",
                            ARG_REGS[j], slot_for_temp(&m, arg_t));
                }
                n_pending = 0;
                /* Align stack to 16 before call */
                fprintf(out, "    call    %s\n", ins->src1.u.var);
                store_rax_to_dst(out, &m, &ins->dst);
                break;
            }

            case IR_RETURN:
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     rsp, rbp\n");
                fprintf(out, "    pop     rbp\n");
                fprintf(out, "    ret\n");
                break;

            case IR_PRINT:
                /* call print_int(value) — value goes in rdi */
                load_op_to_rax(out, &m, &ins->src1);
                fprintf(out, "    mov     rdi, rax\n");
                fprintf(out, "    call    print_int\n");
                break;

            default:
                fprintf(out, "    ; unhandled IR op %d\n", ins->op);
                break;
        }
    }

    /* Implicit return 0 at end */
    fprintf(out, "    xor     rax, rax\n");
    fprintf(out, "    mov     rsp, rbp\n");
    fprintf(out, "    pop     rbp\n");
    fprintf(out, "    ret\n\n");

    slot_free(&m);
}

/* ======================================================= top-level entry */

int codegen(const IRProgram *prog, FILE *out)
{
    /* Check whether print_int is used anywhere */
    int uses_print = 0;
    for (int f = 0; f < prog->func_count; f++) {
        const IRFunc *fn = &prog->funcs[f];
        for (int i = 0; i < fn->instr_count; i++)
            if (fn->instrs[i].op == IR_PRINT) { uses_print = 1; break; }
        if (uses_print) break;
    }

    fprintf(out, "; Generated by mycc — Module 16\n");
    fprintf(out, "bits 64\n");
    fprintf(out, "default rel\n\n");
    if (uses_print)
        fprintf(out, "extern print_int\n\n");
    fprintf(out, "section .text\n\n");

    for (int f = 0; f < prog->func_count; f++)
        codegen_func(&prog->funcs[f], out);

    return 0;
}
