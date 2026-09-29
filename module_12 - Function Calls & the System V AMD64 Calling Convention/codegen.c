/* codegen.c — x86-64 code generator for my_compiler
 * Module 12: Function Calls and the System V AMD64 Calling Convention
 *
 * DESIGN OVERVIEW
 * ===============
 * We use a simple "everything on the stack" strategy:
 *
 *   - Every IR temporary tN lives at a fixed stack slot: -8*(N+1)(%rbp)
 *   - Every named local variable lives at a fixed stack slot looked up by
 *     name in a small local table built during a pre-pass.
 *   - Function parameters are copied from their incoming registers
 *     (rdi/rsi/rdx/rcx/r8/r9) into their named stack slots at function
 *     entry, so the arg registers are free for use immediately afterward.
 *
 * STACK FRAME LAYOUT (for a function with P params, T temporaries, L locals)
 * ===========================================================================
 *
 *   Higher addresses
 *   ...
 *   [return address]         <- pushed by call instruction (8 bytes)
 *   [saved rbp]              <- push rbp at entry (8 bytes)   <-- rbp points here
 *   [callee-saved regs]      <- rbx, r12-r15 if used (8 bytes each)
 *   [named local slots]      <- one 8-byte slot per named variable
 *   [temporary slots]        <- one 8-byte slot per IR temporary tN
 *   ...
 *   Lower addresses          <-- rsp after prologue sub
 *
 * We keep things simple: one flat allocation of N_TOTAL slots where
 *   N_TOTAL = (next_temp + n_locals)  rounded up to a multiple of 2
 * to keep the 16-byte alignment rule satisfied.
 *
 * 16-BYTE ALIGNMENT RULE
 * =======================
 * The System V ABI requires that rsp is 16-byte aligned BEFORE a call
 * instruction executes (at the point of the call, rsp must be a multiple
 * of 16; the call then pushes 8 bytes making it 8-byte aligned at callee
 * entry, which the callee's prologue fixes with push rbp).
 *
 * After our prologue:
 *   push rbp   -> rsp -= 8
 *   sub  rsp, N -> rsp -= N
 * We need (N + 8) % 16 == 0, i.e. N % 16 == 8.
 * We compute N as a multiple of 8, then add 8 if needed.
 *
 * CALLEE-SAVED REGISTERS
 * =======================
 * The ABI requires a callee to preserve rbx, r12, r13, r14, r15.
 * In this simple compiler we do not use those registers for temporaries,
 * so we never need to save/restore them.  If a future pass allocates
 * temporaries to callee-saved registers it must push them in the prologue
 * and pop them before ret.
 *
 * CALLING ANOTHER FUNCTION
 * =========================
 * IR_PARAM instructions accumulate arguments in an array.  When IR_CALL
 * is reached we:
 *   1. Load each accumulated argument from its stack slot into the
 *      appropriate argument register (rdi, rsi, rdx, rcx, r8, r9).
 *   2. Ensure rsp is 16-byte aligned (it already is if our prologue is
 *      correct and no variadic args are used; we emit an alignment check
 *      via an inline comment for documentation).
 *   3. Emit: call funcname
 *   4. Move rax (the return value) into the destination stack slot.
 *   5. Clear the parameter accumulator.
 */

#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constants                                                             */
/* ------------------------------------------------------------------ */

/* System V AMD64 argument registers in order. */
static const char *ARG_REGS[6] = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

#define MAX_LOCALS  256  /* max named variables per function */
#define MAX_TEMPS   512  /* max IR temporaries per function  */
#define MAX_PARAMS    6  /* max supported function arguments  */

/* ------------------------------------------------------------------ */
/* Local-variable name-to-slot mapping                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[64]; /* variable name */
    int  slot;     /* stack slot index (0-based; address = -8*(slot+1)(%rbp)) */
} LocalEntry;

typedef struct {
    LocalEntry entries[MAX_LOCALS];
    int        count;
    int        next_slot; /* next available slot index */
} LocalMap;

/* Find or create a named-variable slot. */
static int local_slot(LocalMap *lm, const char *name) {
    for (int i = 0; i < lm->count; i++) {
        if (strcmp(lm->entries[i].name, name) == 0)
            return lm->entries[i].slot;
    }
    /* Not found — allocate a new slot. */
    if (lm->count >= MAX_LOCALS) {
        fprintf(stderr, "codegen: too many locals\n");
        return 0;
    }
    LocalEntry *e = &lm->entries[lm->count++];
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->name[sizeof(e->name) - 1] = '\0';
    e->slot = lm->next_slot++;
    return e->slot;
}

/* ------------------------------------------------------------------ */
/* Stack-slot address helpers                                            */
/* ------------------------------------------------------------------ */

/*
 * We lay out all slots in one contiguous region below rbp:
 *
 *   Slot 0 -> -8(%rbp)
 *   Slot 1 -> -16(%rbp)
 *   Slot N -> -8*(N+1)(%rbp)
 *
 * Temporaries use slots [0 .. next_temp-1].
 * Named locals use slots [next_temp .. next_temp+n_locals-1].
 * The LocalMap's next_slot starts at next_temp.
 */

/* Return the rbp-relative offset for a temporary. */
static int temp_offset(int t) {
    return -8 * (t + 1);
}

/* Return the rbp-relative offset for a named local (slot index). */
static int local_offset(int slot) {
    return -8 * (slot + 1);
}

/* Emit "movq src, %rax" then "movq %rax, offset(%rbp)" — used to
 * move a value from one stack location to another (no mem-to-mem). */
static void emit_stack_copy(FILE *out, int src_off, int dst_off) {
    fprintf(out, "    movq    %d(%%rbp), %%rax\n", src_off);
    fprintf(out, "    movq    %%rax, %d(%%rbp)\n",  dst_off);
}

/* ------------------------------------------------------------------ */
/* Pre-pass: count temporaries and named locals                          */
/* ------------------------------------------------------------------ */

/*
 * Walk the instruction list to discover:
 *   - The maximum temporary index used (= next_temp set by IR generator).
 *   - All named local variables (IR_STORE / IR_LOAD).
 * This lets us compute the total stack frame size before emitting anything.
 */
static void prepass(const IRFunc *f, int *out_n_temps, LocalMap *lm) {
    *out_n_temps = f->next_temp; /* IR generator already tracked this */

    lm->count     = 0;
    lm->next_slot = f->next_temp; /* named locals start after temps */

    for (IRInstr *in = f->head; in; in = in->next) {
        if (in->op == IR_STORE || in->op == IR_LOAD) {
            local_slot(lm, in->name); /* ensure slot exists */
        }
    }
}

/* ------------------------------------------------------------------ */
/* Compute the frame size (bytes to subtract from rsp)                   */
/* ------------------------------------------------------------------ */

/*
 * Total slots = temporaries + named locals.
 * Each slot is 8 bytes.
 * We need (frame_size + 8) % 16 == 0  (the +8 accounts for push rbp).
 * So frame_size must satisfy frame_size % 16 == 8.
 *
 * Algorithm:
 *   raw = total_slots * 8
 *   if raw == 0, use 8 (minimum frame to keep alignment)
 *   if (raw + 8) % 16 != 0, add 8
 */
static int compute_frame_size(int total_slots) {
    int raw = total_slots * 8;
    if (raw == 0) raw = 8; /* at least 8 bytes for alignment */
    if ((raw + 8) % 16 != 0) raw += 8;
    return raw;
}

/* ------------------------------------------------------------------ */
/* Parameter accumulator                                                 */
/* ------------------------------------------------------------------ */

/* During codegen we accumulate IR_PARAM source temporaries and flush
 * them into argument registers when we see IR_CALL. */
typedef struct {
    int temps[MAX_PARAMS]; /* source temporary for each argument */
    int count;             /* number of accumulated arguments */
} ParamBuf;

/* ------------------------------------------------------------------ */
/* Emit a single function                                                */
/* ------------------------------------------------------------------ */

static void emit_function(const IRFunc *f, FILE *out) {
    /* ---- Pre-pass ---- */
    int      n_temps;
    LocalMap lm;
    prepass(f, &n_temps, &lm);

    int total_slots = n_temps + lm.count;
    int frame_size  = compute_frame_size(total_slots);

    /* ---- Function label and prologue ---- */
    fprintf(out, "\n");
    fprintf(out, "    .globl  %s\n", f->name);
    fprintf(out, "%s:\n", f->name);
    fprintf(out, "    # Prologue\n");
    fprintf(out, "    pushq   %%rbp\n");
    fprintf(out, "    movq    %%rsp, %%rbp\n");
    fprintf(out, "    subq    $%d, %%rsp\n", frame_size);
    fprintf(out, "    # Frame: %d slots x 8 bytes = %d bytes (16-byte aligned)\nplkjokpppppppppppppppppppppppppppppppppppppppppppppppp",
            total_slots, frame_size);

    /*
     * Save incoming parameters from registers to named stack slots.
     *
     * The AST_FUNC node stores parameters as args[0..n_params-1].  The IR
     * generator does not emit those as IR instructions — instead we rely on
     * the function's name being looked up in the symbol table at parse time.
     *
     * We don't have access to the AST here.  Instead we walk the IR to find
     * the first IR_LOAD for each name that is also a parameter.
     *
     * A better approach: the IR generator could emit explicit IR_PARAM_RECV
     * instructions.  For Module 12 we use a simpler heuristic:
     *
     *   We scan the whole instruction stream for IR_LOAD instructions and
     *   track which named variables are ONLY loaded (never stored to) — those
     *   are parameters, because locals are always stored before being loaded.
     *
     * Actually, the cleanest approach for our IR: we count the number of
     * params from the nargs field of the first IR_CALL that calls this
     * function.  But we don't have that info in IRFunc either.
     *
     * SIMPLEST CORRECT APPROACH for Module 12:
     *   We detect parameters by finding names that appear in IR_LOAD but
     *   have no preceding IR_STORE in the same function.  These must be
     *   parameters — they arrive in registers at function entry.
     *
     * We then assign them argument register indices in the order they first
     * appear in the IR (which matches declaration order since the IR
     * generator visits parameters in order).
     */

    /* First: find all names that are loaded before any store (= parameters). */
    /* We do a simple two-pass scan within the IR. */
    char param_names[MAX_PARAMS][64];
    int  n_params = 0;
    {
        /* Track which names have been stored. */
        char stored[MAX_LOCALS][64];
        int  n_stored = 0;

        for (IRInstr *in = f->head; in; in = in->next) {
            if (in->op == IR_STORE) {
                /* Record this name as stored. */
                int found = 0;
                for (int i = 0; i < n_stored; i++) {
                    if (strcmp(stored[i], in->name) == 0) { found = 1; break; }
                }
                if (!found && n_stored < MAX_LOCALS) {
                    strncpy(stored[n_stored], in->name, 63);
                    stored[n_stored][63] = '\0';
                    n_stored++;
                }
            } else if (in->op == IR_LOAD) {
                /* Is this name stored yet? If not, it's a parameter. */
                int is_stored = 0;
                for (int i = 0; i < n_stored; i++) {
                    if (strcmp(stored[i], in->name) == 0) { is_stored = 1; break; }
                }
                if (!is_stored) {
                    /* Check if already recorded as param. */
                    int already = 0;
                    for (int i = 0; i < n_params; i++) {
                        if (strcmp(param_names[i], in->name) == 0) {
                            already = 1; break;
                        }
                    }
                    if (!already && n_params < MAX_PARAMS) {
                        strncpy(param_names[n_params], in->name, 63);
                        param_names[n_params][63] = '\0';
                        n_params++;
                    }
                }
            }
        }
    }

    /* Emit code to spill incoming argument registers to named stack slots. */
    if (n_params > 0) {
        fprintf(out, "    # Spill incoming parameters to stack slots\n");
        for (int i = 0; i < n_params; i++) {
            int slot = local_slot(&lm, param_names[i]);
            int off  = local_offset(slot);
            fprintf(out, "    movq    %s, %d(%%rbp)   # param '%s'\n",
                    ARG_REGS[i], off, param_names[i]);
        }
    }

    /* ---- Parameter accumulator for outgoing calls ---- */
    ParamBuf pb;
    pb.count = 0;

    /* ---- Emit each instruction ---- */
    for (IRInstr *in = f->head; in; in = in->next) {
        switch (in->op) {

        /* ---- IR_ICONST: dst = immediate ---- */
        case IR_ICONST:
            fprintf(out, "    movq    $%ld, %d(%%rbp)   # t%d = %ld\n",
                    in->ival, temp_offset(in->dst), in->dst, in->ival);
            break;

        /* ---- IR_COPY: dst = src1 ---- */
        case IR_COPY:
            emit_stack_copy(out, temp_offset(in->src1), temp_offset(in->dst));
            fprintf(out, "    # t%d = copy t%d\n", in->dst, in->src1);
            break;

        /* ---- IR_NEG: dst = -src1 ---- */
        case IR_NEG:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    negq    %%rax\n");
            fprintf(out, "    movq    %%rax, %d(%%rbp)   # t%d = -t%d\n",
                    temp_offset(in->dst), in->dst, in->src1);
            break;

        /* ---- IR_ADD ---- */
        case IR_ADD:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    addq    %d(%%rbp), %%rax\n", temp_offset(in->src2));
            fprintf(out, "    movq    %%rax, %d(%%rbp)   # t%d = t%d + t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_SUB ---- */
        case IR_SUB:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    subq    %d(%%rbp), %%rax\n", temp_offset(in->src2));
            fprintf(out, "    movq    %%rax, %d(%%rbp)   # t%d = t%d - t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_MUL ---- */
        case IR_MUL:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    imulq   %d(%%rbp), %%rax\n", temp_offset(in->src2));
            fprintf(out, "    movq    %%rax, %d(%%rbp)   # t%d = t%d * t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_DIV ---- */
        case IR_DIV:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    cqto\n");                /* sign-extend rax into rdx:rax */
            fprintf(out, "    idivq   %d(%%rbp)\n",   temp_offset(in->src2));
            fprintf(out, "    movq    %%rax, %d(%%rbp)   # t%d = t%d / t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_MOD ---- */
        case IR_MOD:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    cqto\n");
            fprintf(out, "    idivq   %d(%%rbp)\n",   temp_offset(in->src2));
            fprintf(out, "    movq    %%rdx, %d(%%rbp)   # t%d = t%d %% t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- Comparisons ---- */
        case IR_LT:
        case IR_GT:
        case IR_LEQ:
        case IR_GEQ:
        case IR_EQ:
        case IR_NEQ: {
            const char *setcc = "sete"; /* default; overridden below */
            switch (in->op) {
                case IR_LT:  setcc = "setl";  break;
                case IR_GT:  setcc = "setg";  break;
                case IR_LEQ: setcc = "setle"; break;
                case IR_GEQ: setcc = "setge"; break;
                case IR_EQ:  setcc = "sete";  break;
                case IR_NEQ: setcc = "setne"; break;
                default: break;
            }
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    cmpq    %d(%%rbp), %%rax\n", temp_offset(in->src2));
            fprintf(out, "    %s    %%al\n", setcc);
            fprintf(out, "    movzbq  %%al, %%rax\n");
            fprintf(out, "    movq    %%rax, %d(%%rbp)\n", temp_offset(in->dst));
            break;
        }

        /* ---- IR_AND: dst = (src1 != 0) & (src2 != 0) ---- */
        case IR_AND:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    testq   %%rax, %%rax\n");
            fprintf(out, "    setne   %%al\n");
            fprintf(out, "    movzbq  %%al, %%rax\n");
            fprintf(out, "    movq    %d(%%rbp), %%rcx\n", temp_offset(in->src2));
            fprintf(out, "    testq   %%rcx, %%rcx\n");
            fprintf(out, "    setne   %%cl\n");
            fprintf(out, "    movzbq  %%cl, %%rcx\n");
            fprintf(out, "    andq    %%rcx, %%rax\n");
            fprintf(out, "    movq    %%rax, %d(%%rbp)\n", temp_offset(in->dst));
            break;

        /* ---- IR_OR: dst = (src1 != 0) | (src2 != 0) ---- */
        case IR_OR:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    testq   %%rax, %%rax\n");
            fprintf(out, "    setne   %%al\n");
            fprintf(out, "    movzbq  %%al, %%rax\n");
            fprintf(out, "    movq    %d(%%rbp), %%rcx\n", temp_offset(in->src2));
            fprintf(out, "    testq   %%rcx, %%rcx\n");
            fprintf(out, "    setne   %%cl\n");
            fprintf(out, "    movzbq  %%cl, %%rcx\n");
            fprintf(out, "    orq     %%rcx, %%rax\n");
            fprintf(out, "    movq    %%rax, %d(%%rbp)\n", temp_offset(in->dst));
            break;

        /* ---- IR_STORE: named_var = src1 ---- */
        case IR_STORE: {
            int slot = local_slot(&lm, in->name);
            int off  = local_offset(slot);
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    movq    %%rax, %d(%%rbp)   # store '%s'\n",
                    off, in->name);
            break;
        }

        /* ---- IR_LOAD: dst = named_var ---- */
        case IR_LOAD: {
            int slot = local_slot(&lm, in->name);
            int off  = local_offset(slot);
            fprintf(out, "    movq    %d(%%rbp), %%rax   # load '%s'\n",
                    off, in->name);
            fprintf(out, "    movq    %%rax, %d(%%rbp)\n", temp_offset(in->dst));
            break;
        }

        /* ---- IR_LABEL: define a jump target ---- */
        case IR_LABEL:
            fprintf(out, ".L_%s_%s:\n", f->name, in->name);
            break;

        /* ---- IR_JUMP: unconditional branch ---- */
        case IR_JUMP:
            fprintf(out, "    jmp     .L_%s_%s\n", f->name, in->name);
            break;

        /* ---- IR_JUMPZ: branch if src1 == 0 ---- */
        case IR_JUMPZ:
            fprintf(out, "    movq    %d(%%rbp), %%rax\n", temp_offset(in->src1));
            fprintf(out, "    testq   %%rax, %%rax\n");
            fprintf(out, "    je      .L_%s_%s\n", f->name, in->name);
            break;

        /* ---- IR_PARAM: accumulate argument for next call ---- */
        case IR_PARAM:
            if (pb.count < MAX_PARAMS) {
                pb.temps[pb.count++] = in->src1;
            } else {
                fprintf(stderr, "codegen: more than %d arguments not supported\n",
                        MAX_PARAMS);
            }
            break;

        /* ---- IR_CALL: load args into registers and call ---- */
        case IR_CALL: {
            fprintf(out, "    # Call %s (%d args)\n", in->name, pb.count);
            /*
             * Load each accumulated argument into its register.
             * We use %r10 as scratch to avoid clobbering arg regs while
             * loading subsequent arguments.  Since all values live in
             * stack slots (not in registers), we can load directly.
             */
            for (int i = 0; i < pb.count; i++) {
                fprintf(out, "    movq    %d(%%rbp), %s   # arg %d\n",
                        temp_offset(pb.temps[i]), ARG_REGS[i], i);
            }
            /*
             * rsp must be 16-byte aligned here.
             * Our prologue guarantees this if the function does not push
             * anything else onto the stack.  For safety we keep the frame
             * size a multiple of 16 + 8 (see compute_frame_size), so rsp
             * is always 16-byte aligned within our function body.
             */
            fprintf(out, "    call    %s\n", in->name);
            /* Store return value (rax) into the destination slot. */
            if (in->dst != IR_NO_TEMP) {
                fprintf(out, "    movq    %%rax, %d(%%rbp)   # t%d = return value\n",
                        temp_offset(in->dst), in->dst);
            }
            /* Reset the parameter accumulator. */
            pb.count = 0;
            break;
        }

        /* ---- IR_RETURN ---- */
        case IR_RETURN:
            if (in->src1 != IR_NO_TEMP) {
                fprintf(out, "    movq    %d(%%rbp), %%rax   # return value\n",
                        temp_offset(in->src1));
            } else {
                fprintf(out, "    xorl    %%eax, %%eax   # void return\n");
            }
            /* Epilogue */
            fprintf(out, "    # Epilogue\n");
            fprintf(out, "    movq    %%rbp, %%rsp\n");
            fprintf(out, "    popq    %%rbp\n");
            fprintf(out, "    ret\n");
            break;

        default:
            fprintf(stderr, "codegen: unknown IROp %d\n", (int)in->op);
            break;
        }
    }

    /*
     * Safety net: if the function body did not end with IR_RETURN,
     * emit a default return of 0 so the function always has a valid exit.
     */
    fprintf(out, "    # (implicit return 0)\n");
    fprintf(out, "    xorl    %%eax, %%eax\n");
    fprintf(out, "    movq    %%rbp, %%rsp\n");
    fprintf(out, "    popq    %%rbp\n");
    fprintf(out, "    ret\n");
}

/* ------------------------------------------------------------------ */
/* codegen — public entry point                                          */
/* ------------------------------------------------------------------ */

void codegen(const IRProg *prog, FILE *out) {
    /* File header */
    fprintf(out, "    .text\n");
    fprintf(out, "    # Generated by my_compiler Module 12\n");
    fprintf(out, "    # System V AMD64 ABI-compliant x86-64 assembly\n");

    for (int i = 0; i < prog->n_funcs; i++) {
        emit_function(&prog->funcs[i], out);
    }
}
