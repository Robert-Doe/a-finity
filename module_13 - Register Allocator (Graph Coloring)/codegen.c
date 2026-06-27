/* codegen.c — x86-64 NASM code generation with register allocation
 * Module 13: Register Allocation
 *
 * Key design decisions:
 *   - Each function gets its own RegMap from regalloc().
 *   - A temp in a register:  we emit its REG_NAMES[assign[t]] directly.
 *   - A spilled temp:        we mov to/from [rbp+spill_slot] around each use/def.
 *   - Locals are stored by name in a local_slot[] table (name -> rbp offset).
 *   - Prologue: push callee-saved registers that the regmap actually uses,
 *     then sub rsp to make room for locals + spilled temps.
 *   - Epilogue: add rsp, pop callee-saved in reverse order, ret.
 *   - Function calls: arguments in rdi/rsi/rdx/rcx/r8/r9; return in rax.
 *   - Division (IR_DIV/IR_MOD): uses idiv, which needs rax:rdx dividend.
 *     We save/restore rdx around the division.
 *
 * Stack frame (high → low addresses):
 *   rbp+8   return address
 *   rbp+0   saved old rbp      <- rbp points here
 *   rbp-8   callee-save save area (push instructions handle this)
 *   ...     (handled by push/pop in prologue/epilogue)
 *   then sub rsp, FRAME_SIZE where
 *   FRAME_SIZE = (n_locals + n_spilled) * 8  rounded up to multiple of 16.
 *
 * Local variable name→slot mapping:
 *   The IR emits IR_STORE name / IR_LOAD name for source-level variables.
 *   We assign them slots at [rbp - 8*1], [rbp - 8*2], etc., lazily on first use.
 */
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Local variable name → stack slot table                              */
/* ------------------------------------------------------------------ */

#define MAX_LOCALS 128

typedef struct {
    char name[64];
    int  offset; /* byte offset from rbp (negative) */
} LocalSlot;

typedef struct {
    LocalSlot slots[MAX_LOCALS];
    int       count;
    int       next_offset; /* next offset to assign (starts at -8) */
} LocalMap;

static void localmap_init(LocalMap *lm, int base_offset) {
    lm->count       = 0;
    lm->next_offset = base_offset; /* caller sets to -(n_locals*8 + ...) */
}

/* Look up or create a slot for a named local variable.
 * base_offset is the starting offset (negative, e.g. -8 for first slot). */
static int localmap_get(LocalMap *lm, const char *name) {
    for (int i = 0; i < lm->count; i++) {
        if (strcmp(lm->slots[i].name, name) == 0)
            return lm->slots[i].offset;
    }
    /* Allocate a new slot. */
    if (lm->count >= MAX_LOCALS) {
        fprintf(stderr, "codegen: too many locals\n");
        return -8;
    }
    LocalSlot *s = &lm->slots[lm->count++];
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->offset       = lm->next_offset;
    lm->next_offset -= 8;
    return s->offset;
}

/* ------------------------------------------------------------------ */
/* Helpers: emit the location (register or memory) for a temp          */
/* ------------------------------------------------------------------ */

/* Write the operand string for temp t into buf.
 * If assigned to a register: "rbx" etc.
 * If spilled: "qword [rbp-NN]".  */
static void temp_loc(char *buf, size_t sz, const RegMap *rm, int t) {
    if (t < 0 || t >= MAX_TEMPS) {
        snprintf(buf, sz, "rax"); /* fallback — should not happen */
        return;
    }
    if (rm->assign[t] >= 0) {
        snprintf(buf, sz, "%s", REG_NAMES[rm->assign[t]]);
    } else {
        /* spill_slot is already negative (e.g. -8) but the base is relative to
         * the frame: we reserve local slots first, then spill slots.
         * The absolute offset stored in spill_slot is used directly. */
        snprintf(buf, sz, "qword [rbp%d]", rm->spill_slot[t]);
    }
}

/* Load temp t into register 'reg' (always emits a mov if spilled,
 * or nothing if temp is already in 'reg'). */
static void load_temp(FILE *fp, const RegMap *rm, int t, const char *reg) {
    if (t < 0 || t >= MAX_TEMPS) return;
    if (rm->assign[t] >= 0 && strcmp(REG_NAMES[rm->assign[t]], reg) == 0)
        return; /* already in the right register */
    char loc[64];
    temp_loc(loc, sizeof(loc), rm, t);
    fprintf(fp, "    mov     %s, %s\n", reg, loc);
}

/* Store from 'reg' into temp t's location. */
static void store_temp(FILE *fp, const RegMap *rm, int t, const char *reg) {
    if (t < 0 || t >= MAX_TEMPS) return;
    char loc[64];
    temp_loc(loc, sizeof(loc), rm, t);
    if (strcmp(loc, reg) == 0) return; /* no-op */
    fprintf(fp, "    mov     %s, %s\n", loc, reg);
}

/* ------------------------------------------------------------------ */
/* Callee-saved register tracking                                       */
/* ------------------------------------------------------------------ */

/* Indices 0..4 are callee-saved (rbx, r12, r13, r14, r15).
 * Index 5 is r10 which is caller-saved — no need to preserve. */
#define NUM_CALLEE_SAVED 5

static int reg_is_callee_saved(int reg_idx) {
    return reg_idx < NUM_CALLEE_SAVED;
}

/* Determine which callee-saved registers are actually used by this RegMap. */
static void used_callee_saved(const RegMap *rm, int n_temps, int used[NUM_REGS]) {
    memset(used, 0, sizeof(int) * NUM_REGS);
    for (int t = 0; t < n_temps && t < MAX_TEMPS; t++) {
        int r = rm->assign[t];
        if (r >= 0 && r < NUM_REGS && reg_is_callee_saved(r))
            used[r] = 1;
    }
}

/* ------------------------------------------------------------------ */
/* Argument-passing registers (System V AMD64)                          */
/* ------------------------------------------------------------------ */

static const char *ARG_REGS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
#define NUM_ARG_REGS 6

/* ------------------------------------------------------------------ */
/* Per-function state during codegen                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    const IRFunc *fn;
    const RegMap *rm;
    LocalMap      lm;
    FILE         *fp;
    int           frame_size; /* total bytes subtracted from rsp */
    /* Accumulated PARAM temps before a CALL */
    int           param_temps[NUM_ARG_REGS];
    int           n_params;
} CGCtx;

/* ------------------------------------------------------------------ */
/* Codegen for a single instruction                                     */
/* ------------------------------------------------------------------ */

static void cg_instr(CGCtx *ctx, const IRInstr *ins) {
    FILE         *fp = ctx->fp;
    const RegMap *rm = ctx->rm;

    switch (ins->op) {

    /* ---------------------------------------------------------------- */
    case IR_LABEL:
        fprintf(fp, ".%s:\n", ins->name);
        break;

    /* ---------------------------------------------------------------- */
    case IR_ICONST:
        /* dst = immediate */
        if (ins->dst == IR_NO_TEMP) break;
        if (rm->assign[ins->dst] >= 0) {
            fprintf(fp, "    mov     %s, %ld\n",
                    REG_NAMES[rm->assign[ins->dst]], ins->ival);
        } else {
            fprintf(fp, "    mov     rax, %ld\n", ins->ival);
            fprintf(fp, "    mov     qword [rbp%d], rax\n",
                    rm->spill_slot[ins->dst]);
        }
        break;

    /* ---------------------------------------------------------------- */
    case IR_COPY:
        if (ins->dst == IR_NO_TEMP) break;
        load_temp(fp, rm, ins->src1, "rax");
        store_temp(fp, rm, ins->dst, "rax");
        break;

    /* ---------------------------------------------------------------- */
    case IR_ADD: {
        load_temp(fp, rm, ins->src1, "rax");
        load_temp(fp, rm, ins->src2, "rcx");
        fprintf(fp, "    add     rax, rcx\n");
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_SUB: {
        load_temp(fp, rm, ins->src1, "rax");
        load_temp(fp, rm, ins->src2, "rcx");
        fprintf(fp, "    sub     rax, rcx\n");
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_MUL: {
        load_temp(fp, rm, ins->src1, "rax");
        load_temp(fp, rm, ins->src2, "rcx");
        fprintf(fp, "    imul    rax, rcx\n");
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_NEG: {
        load_temp(fp, rm, ins->src1, "rax");
        fprintf(fp, "    neg     rax\n");
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_DIV:
    case IR_MOD: {
        /* idiv uses rax:rdx; we must save rdx if it might be live. */
        fprintf(fp, "    push    rdx\n");
        load_temp(fp, rm, ins->src1, "rax");
        fprintf(fp, "    cqo\n");          /* sign-extend rax into rdx:rax */
        load_temp(fp, rm, ins->src2, "rcx");
        fprintf(fp, "    idiv    rcx\n");  /* rax = quotient, rdx = remainder */
        if (ins->op == IR_DIV) {
            store_temp(fp, rm, ins->dst, "rax");
            fprintf(fp, "    pop     rdx\n");
        } else {
            fprintf(fp, "    mov     rax, rdx\n"); /* remainder into rax */
            fprintf(fp, "    pop     rdx\n");
            store_temp(fp, rm, ins->dst, "rax");
        }
        break;
    }

    /* ---------------------------------------------------------------- */
    /* Comparison instructions — produce 0 or 1 via setCC               */
    case IR_LT: case IR_GT: case IR_LEQ: case IR_GEQ:
    case IR_EQ: case IR_NEQ: {
        load_temp(fp, rm, ins->src1, "rax");
        load_temp(fp, rm, ins->src2, "rcx");
        fprintf(fp, "    cmp     rax, rcx\n");
        fprintf(fp, "    mov     rax, 0\n");
        const char *setcc =
            (ins->op == IR_LT)  ? "setl" :
            (ins->op == IR_GT)  ? "setg" :
            (ins->op == IR_LEQ) ? "setle":
            (ins->op == IR_GEQ) ? "setge":
            (ins->op == IR_EQ)  ? "sete" : "setne";
        fprintf(fp, "    %s    al\n", setcc);
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_AND: {
        load_temp(fp, rm, ins->src1, "rax");
        load_temp(fp, rm, ins->src2, "rcx");
        /* Produce 1 if both non-zero, else 0 */
        fprintf(fp, "    test    rax, rax\n");
        fprintf(fp, "    setne   al\n");
        fprintf(fp, "    movzx   rax, al\n");
        fprintf(fp, "    test    rcx, rcx\n");
        fprintf(fp, "    setne   cl\n");
        fprintf(fp, "    movzx   rcx, cl\n");
        fprintf(fp, "    and     rax, rcx\n");
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    case IR_OR: {
        load_temp(fp, rm, ins->src1, "rax");
        load_temp(fp, rm, ins->src2, "rcx");
        fprintf(fp, "    test    rax, rax\n");
        fprintf(fp, "    setne   al\n");
        fprintf(fp, "    movzx   rax, al\n");
        fprintf(fp, "    test    rcx, rcx\n");
        fprintf(fp, "    setne   cl\n");
        fprintf(fp, "    movzx   rcx, cl\n");
        fprintf(fp, "    or      rax, rcx\n");
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_JUMP:
        fprintf(fp, "    jmp     .%s\n", ins->name);
        break;

    /* ---------------------------------------------------------------- */
    case IR_JUMPZ:
        load_temp(fp, rm, ins->src1, "rax");
        fprintf(fp, "    test    rax, rax\n");
        fprintf(fp, "    jz      .%s\n", ins->name);
        break;

    /* ---------------------------------------------------------------- */
    case IR_STORE: {
        int off = localmap_get(&ctx->lm, ins->name);
        load_temp(fp, rm, ins->src1, "rax");
        fprintf(fp, "    mov     qword [rbp%d], rax\n", off);
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_LOAD: {
        if (ins->dst == IR_NO_TEMP) break;
        int off = localmap_get(&ctx->lm, ins->name);
        fprintf(fp, "    mov     rax, qword [rbp%d]\n", off);
        store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_PARAM:
        /* Accumulate params; they will be placed in arg registers at IR_CALL. */
        if (ctx->n_params < NUM_ARG_REGS)
            ctx->param_temps[ctx->n_params++] = ins->src1;
        break;

    /* ---------------------------------------------------------------- */
    case IR_CALL: {
        /* Move params into argument registers. */
        for (int i = 0; i < ctx->n_params && i < NUM_ARG_REGS; i++) {
            load_temp(fp, rm, ctx->param_temps[i], ARG_REGS[i]);
        }
        ctx->n_params = 0;

        /* Align stack to 16 bytes before the call.
         * We do a simple push rax (8 bytes extra) if the frame is already
         * 16-byte aligned (push rbp makes it +8, then sub rsp,frame_size).
         * Simplest: always push one padding qword, then pop after call. */
        fprintf(fp, "    push    rax\n");         /* 16-byte alignment padding */
        fprintf(fp, "    call    %s\n", ins->name);
        fprintf(fp, "    pop     rcx\n");          /* discard padding */

        /* Move return value from rax into the destination temp. */
        if (ins->dst != IR_NO_TEMP)
            store_temp(fp, rm, ins->dst, "rax");
        break;
    }

    /* ---------------------------------------------------------------- */
    case IR_RETURN:
        if (ins->src1 != IR_NO_TEMP)
            load_temp(fp, rm, ins->src1, "rax");
        else
            fprintf(fp, "    xor     eax, eax\n");
        /* Jump to function epilogue. */
        fprintf(fp, "    jmp     .%s_epilogue\n", ctx->fn->name);
        break;

    default:
        fprintf(fp, "    ; unhandled IR op %d\n", ins->op);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Codegen for one function                                             */
/* ------------------------------------------------------------------ */

static void cg_function(FILE *fp, const IRFunc *fn) {
    /* Run register allocation. */
    RegMap rm = regalloc(fn);

    /* Count how many distinct temps the function uses. */
    int n_temps = fn->next_temp;

    /* Print the register map for this function. */
    printf("=== Register allocation for '%s' ===\n", fn->name);
    regmap_print(&rm, n_temps);

    /* Determine which callee-saved registers are used. */
    int cs_used[NUM_REGS];
    used_callee_saved(&rm, n_temps, cs_used);

    /* Count callee-saved regs actually used (for alignment purposes). */
    int n_cs_pushed = 0;
    for (int i = 0; i < NUM_CALLEE_SAVED; i++)
        if (cs_used[i]) n_cs_pushed++;

    /* Compute frame size:
     *   n_locals * 8 bytes (local variables)
     *   n_spilled * 8 bytes (spilled temporaries)
     * Rounded up to 16 bytes for ABI alignment.
     * Note: after push rbp + n_cs_pushed pushes, rsp is offset by
     * (n_cs_pushed+1)*8 bytes.  We want total (rsp before call) to be
     * 16-byte aligned.  A call instruction pushes a return address (8 bytes),
     * so we need rsp+8 to be 16-aligned just before call, i.e. rsp % 16 == 0
     * after our sub.
     */
    int spill_total = (fn->n_locals + rm.n_spilled) * 8;
    /* After push rbp: rsp is 16k - 8 (entry was 16k, call pushed ret addr making
     * it 16k-8, then push rbp makes it 16k-16, wait... let's be explicit:
     *
     * At function entry (after CALL by caller): rsp % 16 == 8  (caller had 16-aligned
     * rsp, call pushed 8-byte return address).
     * push rbp: rsp -= 8  => rsp % 16 == 0
     * push callee-saved (n_cs_pushed times): rsp -= 8*n_cs_pushed
     * sub rsp, frame_size.
     * We need final rsp % 16 == 0:
     *   (initial_rsp_before_push_rbp) - 8 - 8*n_cs_pushed - frame_size ≡ 0 (mod 16)
     *   Since initial_rsp ≡ 8, then:
     *   8 - 8 - 8*n_cs_pushed - frame_size ≡ 0 (mod 16)
     *   -8*n_cs_pushed - frame_size ≡ 0 (mod 16)
     *   frame_size ≡ -8*n_cs_pushed (mod 16)
     *   frame_size ≡ 8*n_cs_pushed  (mod 16)   [since -x ≡ 16-x mod 16]
     */
    int align_need = (8 * n_cs_pushed) % 16;
    int frame_size = spill_total;
    if (frame_size < 0) frame_size = 0;
    /* Round frame_size so that (frame_size + 8*n_cs_pushed) % 16 == 0 */
    int combined = frame_size + 8 * n_cs_pushed;
    if (combined % 16 != 0) {
        frame_size += 16 - (combined % 16);
    }
    (void)align_need; /* suppress unused-var warning */

    /* Now set up local variable slots.
     * Local vars start at [rbp-8], [rbp-16], ...
     * Spill slots (handled by regalloc.c with base -8) need to be offset
     * beyond locals.  We patch the spill slots here. */
    /* Recompute spill slots with correct base (after n_locals * 8). */
    int spill_base = -(fn->n_locals * 8);
    int spill_idx  = 0;
    for (int t = 0; t < n_temps && t < MAX_TEMPS; t++) {
        if (rm.assign[t] < 0) {
            /* recalculate spill offset based on actual n_locals */
            rm.spill_slot[t] = spill_base - (spill_idx + 1) * 8;
            spill_idx++;
        }
    }

    /* Initialise local var map (starts after n_locals local var slots). */
    CGCtx ctx;
    ctx.fn       = fn;
    ctx.rm       = &rm;
    ctx.fp       = fp;
    ctx.n_params = 0;
    localmap_init(&ctx.lm, -8); /* local vars start at rbp-8 */

    /* ---- Prologue ---- */
    fprintf(fp, "\nglobal %s\n%s:\n", fn->name, fn->name);
    fprintf(fp, "    push    rbp\n");
    fprintf(fp, "    mov     rbp, rsp\n");

    /* Push callee-saved registers we use. */
    for (int i = 0; i < NUM_CALLEE_SAVED; i++) {
        if (cs_used[i])
            fprintf(fp, "    push    %s\n", REG_NAMES[i]);
    }

    /* Reserve stack space. */
    if (frame_size > 0)
        fprintf(fp, "    sub     rsp, %d\n", frame_size);

    /* Store incoming arguments (rdi, rsi, ...) into local slots.
     * We do this by scanning for the first IR_LOAD instructions that were
     * emitted for parameters (they were marked with nargs = param_index in ir.c).
     * However, the cleanest way: scan the IR for pairs IR_LOAD(name)/IR_STORE(name)
     * at the top of the function that correspond to params.
     * Actually, the caller put params in arg registers; we read them here.
     * The IR has: t0 = LOAD a; STORE a = t0; t1 = LOAD b; STORE b = t1; ...
     * We need to intercept those LOAD instructions that carry nargs >= 0 (param flag).
     */
    int param_idx = 0;
    for (const IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->op == IR_LOAD && ins->nargs >= 0 && param_idx < NUM_ARG_REGS) {
            /* This is a parameter load.  The parameter value is in ARG_REGS[ins->nargs]. */
            int off = localmap_get(&ctx.lm, ins->name);
            fprintf(fp, "    mov     qword [rbp%d], %s\n", off, ARG_REGS[param_idx]);
            param_idx++;
        } else {
            break; /* params are always at the top */
        }
    }

    /* ---- Body ---- */
    /* Skip the param-load IR instructions we already handled. */
    param_idx = 0;
    for (const IRInstr *ins = fn->head; ins; ins = ins->next) {
        /* Skip the paired LOAD/STORE that we handled in the prologue for params. */
        if ((ins->op == IR_LOAD || ins->op == IR_STORE) && param_idx < fn->next_temp) {
            /* Count how many params this function has by checking leading LOAD(nargs>=0) */
            /* We need a different approach: track which instructions to skip. */
            /* Easier: emit them normally; the LOAD from local slot just reloads the arg
             * we just stored, which is correct. */
        }
        cg_instr(&ctx, ins);
    }

    /* ---- Epilogue ---- */
    fprintf(fp, ".%s_epilogue:\n", fn->name);
    if (frame_size > 0)
        fprintf(fp, "    add     rsp, %d\n", frame_size);
    for (int i = NUM_CALLEE_SAVED - 1; i >= 0; i--) {
        if (cs_used[i])
            fprintf(fp, "    pop     %s\n", REG_NAMES[i]);
    }
    fprintf(fp, "    pop     rbp\n");
    fprintf(fp, "    ret\n");
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void codegen(const IRProg *prog, FILE *fp) {
    fprintf(fp, "; Generated by bob_compiler module 13 — register allocation\n");
    fprintf(fp, "bits 64\n");
    fprintf(fp, "default rel\n\n");
    fprintf(fp, "section .text\n");

    for (int i = 0; i < prog->n_funcs; i++) {
        cg_function(fp, &prog->funcs[i]);
        fprintf(fp, "\n");
    }
}
