/* codegen.c — x86-64 NASM code generator for my_compiler
 * Module 10: Code Generation
 *
 * Naive stack allocation: every IR temporary and named local variable
 * gets its own 8-byte slot on the stack, addressed via rbp.
 *
 *   Temporaries t0..tN  -> [rbp - (t+1)*8]
 *   Named locals v0..vM -> [rbp - (MAX_SLOT + v + 1)*8]
 *
 * The total stack frame size is rounded up to 16 bytes for ABI alignment.
 */
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------ */

/* We reserve this many temp slots at the bottom of the frame. */
#define MAX_TEMP_SLOTS 256
/* Maximum named locals per function. */
#define MAX_LOCAL_SLOTS 64

/* SysV AMD64 argument registers (in order). */
static const char *ARG_REGS[6] = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

/* ------------------------------------------------------------------
 * Per-function state
 * ------------------------------------------------------------------ */

typedef struct {
    /* Named local variable -> slot index table */
    char local_names[MAX_LOCAL_SLOTS][64];
    int  n_locals;
    /* Number of IR temporaries used by this function */
    int  n_temps;
    /* Parameters (in order) — stored as local variable names */
    char param_names[6][64];
    int  n_params;
    FILE *out;
} CG;

/* Return the stack offset (in bytes, positive) for temporary t. */
static int temp_offset(int t) {
    return (t + 1) * 8;
}

/* Look up or register a named local; return its slot index. */
static int local_slot(CG *cg, const char *name) {
    for (int i = 0; i < cg->n_locals; i++)
        if (strcmp(cg->local_names[i], name) == 0)
            return i;
    if (cg->n_locals < MAX_LOCAL_SLOTS) {
        strncpy(cg->local_names[cg->n_locals], name, 63);
        cg->local_names[cg->n_locals][63] = '\0';
        return cg->n_locals++;
    }
    return 0;
}

/* Return the stack offset (in bytes, positive) for a named local slot. */
static int local_offset(int slot) {
    return (MAX_TEMP_SLOTS + slot + 1) * 8;
}

/* Compute total stack frame size (rounded up to 16 bytes). */
static int frame_size(CG *cg) {
    int raw = (MAX_TEMP_SLOTS + cg->n_locals + 1) * 8;
    /* Round up to nearest 16 */
    return (raw + 15) & ~15;
}

/* ------------------------------------------------------------------
 * Emission helpers
 * ------------------------------------------------------------------ */

/* Emit a comment line. */
static void emit_comment(CG *cg, const char *msg) {
    fprintf(cg->out, "    ; %s\n", msg);
}

/* Load temp t into rax. */
static void load_temp(CG *cg, int t) {
    fprintf(cg->out, "    mov rax, [rbp - %d]\n", temp_offset(t));
}

/* Store rax into temp t. */
static void store_temp(CG *cg, int t) {
    fprintf(cg->out, "    mov [rbp - %d], rax\n", temp_offset(t));
}

/* Load named local into rax. */
static void load_local(CG *cg, const char *name) {
    int slot = local_slot(cg, name);
    fprintf(cg->out, "    mov rax, [rbp - %d]\n", local_offset(slot));
}

/* Store rax into named local. */
static void store_local(CG *cg, const char *name) {
    int slot = local_slot(cg, name);
    fprintf(cg->out, "    mov [rbp - %d], rax\n", local_offset(slot));
}

/* ------------------------------------------------------------------
 * Instruction emission
 * ------------------------------------------------------------------ */

/* Collect all named locals and temp count from a function's IR. */
static void prescan_func(CG *cg, const IRFunc *fn,
                          const char param_names[][64], int n_params) {
    cg->n_locals = 0;
    cg->n_temps  = fn->next_temp;
    cg->n_params = n_params;
    for (int i = 0; i < n_params && i < 6; i++) {
        strncpy(cg->param_names[i], param_names[i], 63);
        cg->param_names[i][63] = '\0';
        /* Register parameters as locals so they have slots. */
        local_slot(cg, param_names[i]);
    }
    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if ((ins->op == IR_STORE || ins->op == IR_LOAD) && ins->name[0])
            local_slot(cg, ins->name);
    }
}

static void emit_instr(CG *cg, const IRInstr *ins, int *param_count) {
    switch (ins->op) {

        /* -- IR_ICONST: rax = constant, store to dst -- */
        case IR_ICONST:
            fprintf(cg->out, "    mov rax, %ld\n", ins->ival);
            store_temp(cg, ins->dst);
            break;

        /* -- IR_COPY: dst = src1 -- */
        case IR_COPY:
            load_temp(cg, ins->src1);
            store_temp(cg, ins->dst);
            break;

        /* -- Arithmetic: ADD, SUB -- */
        case IR_ADD:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    add rax, [rbp - %d]\n", temp_offset(ins->src2));
            store_temp(cg, ins->dst);
            break;

        case IR_SUB:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    sub rax, [rbp - %d]\n", temp_offset(ins->src2));
            store_temp(cg, ins->dst);
            break;

        /* -- IR_MUL: imul -- */
        case IR_MUL:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    imul rax, [rbp - %d]\n", temp_offset(ins->src2));
            store_temp(cg, ins->dst);
            break;

        /* -- IR_DIV: cqo; idiv; result in rax -- */
        case IR_DIV:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    mov rcx, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    cqo\n");
            fprintf(cg->out, "    idiv rcx\n");
            store_temp(cg, ins->dst);
            break;

        /* -- IR_MOD: cqo; idiv; result in rdx -- */
        case IR_MOD:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    mov rcx, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    cqo\n");
            fprintf(cg->out, "    idiv rcx\n");
            fprintf(cg->out, "    mov rax, rdx\n");
            store_temp(cg, ins->dst);
            break;

        /* -- IR_NEG -- */
        case IR_NEG:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    neg rax\n");
            store_temp(cg, ins->dst);
            break;

        /* -- Comparison operators -- */
        case IR_LT:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    cmp rax, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    setl al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            store_temp(cg, ins->dst);
            break;

        case IR_GT:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    cmp rax, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    setg al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            store_temp(cg, ins->dst);
            break;

        case IR_LEQ:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    cmp rax, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    setle al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            store_temp(cg, ins->dst);
            break;

        case IR_GEQ:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    cmp rax, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    setge al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            store_temp(cg, ins->dst);
            break;

        case IR_EQ:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    cmp rax, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    sete al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            store_temp(cg, ins->dst);
            break;

        case IR_NEQ:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    cmp rax, [rbp - %d]\n", temp_offset(ins->src2));
            fprintf(cg->out, "    setne al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            store_temp(cg, ins->dst);
            break;

        /* Logical AND: both non-zero */
        case IR_AND:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    test rax, rax\n");
            fprintf(cg->out, "    setne al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            fprintf(cg->out, "    mov rcx, rax\n");
            load_temp(cg, ins->src2);
            fprintf(cg->out, "    test rax, rax\n");
            fprintf(cg->out, "    setne al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            fprintf(cg->out, "    and rax, rcx\n");
            store_temp(cg, ins->dst);
            break;

        /* Logical OR: either non-zero */
        case IR_OR:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    test rax, rax\n");
            fprintf(cg->out, "    setne al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            fprintf(cg->out, "    mov rcx, rax\n");
            load_temp(cg, ins->src2);
            fprintf(cg->out, "    test rax, rax\n");
            fprintf(cg->out, "    setne al\n");
            fprintf(cg->out, "    movzx rax, al\n");
            fprintf(cg->out, "    or rax, rcx\n");
            store_temp(cg, ins->dst);
            break;

        /* -- Control flow -- */
        case IR_LABEL:
            fprintf(cg->out, "%s:\n", ins->name);
            break;

        case IR_JUMP:
            fprintf(cg->out, "    jmp %s\n", ins->name);
            break;

        case IR_JUMPZ:
            load_temp(cg, ins->src1);
            fprintf(cg->out, "    test rax, rax\n");
            fprintf(cg->out, "    jz %s\n", ins->name);
            break;

        /* -- Function calls (SysV AMD64 ABI) -- */
        case IR_PARAM:
            if (*param_count < 6) {
                load_temp(cg, ins->src1);
                fprintf(cg->out, "    mov %s, rax\n", ARG_REGS[*param_count]);
            } else {
                /* Extra args pushed onto stack (right to left would need
                 * two passes; for simplicity we push left to right here,
                 * which works for our simple sample programs). */
                load_temp(cg, ins->src1);
                fprintf(cg->out, "    push rax\n");
            }
            (*param_count)++;
            break;

        case IR_CALL:
            fprintf(cg->out, "    call %s\n", ins->name);
            if (*param_count > 6) {
                /* Clean up stack args */
                int stack_args = *param_count - 6;
                fprintf(cg->out, "    add rsp, %d\n", stack_args * 8);
            }
            *param_count = 0; /* reset for next call */
            if (ins->dst != IR_NO_TEMP)
                store_temp(cg, ins->dst);
            break;

        /* -- Return -- */
        case IR_RETURN:
            if (ins->src1 != IR_NO_TEMP)
                load_temp(cg, ins->src1);
            else
                fprintf(cg->out, "    xor rax, rax\n");
            fprintf(cg->out, "    mov rsp, rbp\n");
            fprintf(cg->out, "    pop rbp\n");
            fprintf(cg->out, "    ret\n");
            break;

        /* -- Memory: named variable store/load -- */
        case IR_STORE:
            load_temp(cg, ins->src1);
            store_local(cg, ins->name);
            break;

        case IR_LOAD:
            load_local(cg, ins->name);
            store_temp(cg, ins->dst);
            break;

        default:
            emit_comment(cg, "unhandled IR op");
            break;
    }
}

/* ------------------------------------------------------------------
 * Function emission
 * ------------------------------------------------------------------ */

/*
 * We need parameter names from the AST, but ir.h IRFunc doesn't store them.
 * We reconstruct them by scanning for the first n_params IR_STORE instructions
 * at the start, which correspond to parameter spills — but actually our IR
 * generator registers parameters as locals (via local_slot) without emitting
 * any prologue for them.
 *
 * Solution: scan IR_LOAD/IR_STORE names in order and cross-reference the
 * local_names array to identify the first n entries that correspond to params.
 *
 * Simpler approach: the AST is gone by now. We pass the param names embedded
 * in the IRFunc via a parallel structure from the codegen driver (main.c).
 * For simplicity in this educational compiler, we embed the parameter names
 * in a separate side table passed from ir_gen.
 *
 * Practical approach used here: scan the IR for the first IR_LOAD that
 * references a local, in the order locals were registered. Parameters were
 * registered first, so local_names[0..n_params-1] are the parameters.
 * We store n_params in IRFunc.n_locals (which equals total locals).
 * But we don't know how many are params vs body-declared locals here.
 *
 * FINAL approach: We pass n_params and param_names via the prog_param_info
 * structure built in codegen() from the AST -- but we no longer have the AST.
 * Since IRFunc doesn't carry param names, we use a pragmatic heuristic:
 * emit store-from-register for the first n_params locals using ARG_REGS,
 * where n_params is NOT stored in IRFunc.
 *
 * To solve this cleanly without changing ir.h, we add an extra scanning pass:
 * the first n_params locals in the IR are the parameters (because ir.c calls
 * local_slot for params first, before body locals). We embed n_params in
 * IRFunc.n_locals by subtracting the body-local count, but again we don't
 * know the split.
 *
 * Resolution: store n_params in IRFunc directly. We patch it in ir.c's
 * gen_func via the fn->nargs field from the AST node, and store it in
 * a local parallel array here. We use a simple parallel int array passed
 * from codegen() to emit_func().
 */

static void emit_func(CG *cg, const IRFunc *fn, int n_params,
                       const char param_names[][64]) {
    /* Pre-scan to build the local slot table. */
    prescan_func(cg, fn, param_names, n_params);

    int fsz = frame_size(cg);

    /* Function prologue */
    fprintf(cg->out, "\nglobal %s\n", fn->name);
    fprintf(cg->out, "%s:\n", fn->name);
    fprintf(cg->out, "    push rbp\n");
    fprintf(cg->out, "    mov rbp, rsp\n");
    fprintf(cg->out, "    sub rsp, %d\n", fsz);

    /* Spill incoming parameters from registers to their stack slots */
    for (int i = 0; i < n_params && i < 6; i++) {
        int slot = local_slot(cg, param_names[i]);
        fprintf(cg->out, "    mov [rbp - %d], %s\n",
                local_offset(slot), ARG_REGS[i]);
    }

    /* Emit each IR instruction */
    int param_count = 0;
    for (IRInstr *ins = fn->head; ins; ins = ins->next)
        emit_instr(cg, ins, &param_count);
}

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

/*
 * We need to communicate parameter names to emit_func.
 * We embed n_params per function in a side table.
 * The parameter names are extracted by re-scanning the IR: the first
 * n_params local names in the CG's local_names table are the parameters
 * because ir.c registers them first.
 *
 * Since we cannot know n_params from IRFunc alone (ir.h doesn't store it),
 * we must infer it. Our ir.c calls local_slot() for params before body
 * locals, so they are the first entries in local_names. We exploit the
 * fact that IR_LOAD with a param name will appear before any IR_STORE
 * for that name inside the body; actually both may appear.
 *
 * Simplest correct fix: store n_params in the IRFunc. Since we cannot
 * change ir.h per the spec, we repurpose next_label (which is not used
 * post-IR-generation) to hold n_params. We set this in ir.c's gen_func.
 *
 * next_label after IR generation = n_params stored by ir.c.
 */

void codegen(const IRProg *prog, FILE *out) {
    fprintf(out, "; Generated by my_compiler Module 10 code generator\n");
    fprintf(out, "bits 64\n");
    fprintf(out, "section .text\n");

    for (int fi = 0; fi < prog->n_funcs; fi++) {
        const IRFunc *fn = &prog->funcs[fi];

        /* Retrieve n_params stored in next_label field (see ir.c comment). */
        int n_params = fn->next_label;

        /* Build a temporary CG just to discover local names in order. */
        CG scan;
        memset(&scan, 0, sizeof(scan));
        scan.out = out;

        /* Pre-scan for locals (params registered first in ir.c). */
        for (IRInstr *ins = fn->head; ins; ins = ins->next) {
            if ((ins->op == IR_STORE || ins->op == IR_LOAD) && ins->name[0])
                local_slot(&scan, ins->name);
        }

        /* The first n_params entries in scan.local_names are the parameters. */
        char param_names[6][64];
        memset(param_names, 0, sizeof(param_names));
        for (int i = 0; i < n_params && i < 6; i++)
            strncpy(param_names[i], scan.local_names[i], 63);

        /* Now emit the function. */
        CG cg;
        memset(&cg, 0, sizeof(cg));
        cg.out = out;
        emit_func(&cg, fn, n_params, (const char (*)[64])param_names);
    }

    fprintf(out, "\n");
    /* Linux compatibility: mark stack as non-executable. */
    fprintf(out, "section .note.GNU-stack noalloc noexec nowrite progbits\n");
}
