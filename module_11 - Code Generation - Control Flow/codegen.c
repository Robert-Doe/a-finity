/*
 * codegen.c — x86-64 NASM code generator
 * Module 11: Code Generation — Control Flow
 *
 * ── Stack Frame Layout ────────────────────────────────────────────────────
 *
 *   rbp+16  ... caller's args spilled beyond r9 (not handled here)
 *   rbp+8   return address          (pushed by CALL instruction)
 *   rbp     saved rbp               (push rbp / mov rbp,rsp)
 *   rbp-8   slot 0  (first temp or local)
 *   rbp-16  slot 1
 *   ...
 *   rbp-N*8 slot N-1
 *
 *   Total frame size = next_temp * 8, rounded UP to a multiple of 16
 *   (ABI requires rsp to be 16-byte aligned before CALL).
 *
 * ── Slot Allocation ───────────────────────────────────────────────────────
 *   Every temp and every local variable gets a unique "slot" number assigned
 *   by ir.c (via IRFunc.next_temp).  The slot n lives at [rbp - (n+1)*8].
 *
 *   Locals are also identified by name; ir.c emits IR_STORE/IR_LOAD with
 *   the variable name in ins->name.  The codegen re-derives the slot by
 *   scanning earlier IR_STORE instructions with ins->nargs == -1 (parameter
 *   sentinel) and tracking a name→slot table.
 *
 * ── Control Flow ──────────────────────────────────────────────────────────
 *
 *   IR_LABEL  name    →   name:
 *
 *   IR_JUMP   name    →   jmp name
 *
 *   IR_JUMPZ  src1, name →
 *     mov rax, [rbp - (src1+1)*8]
 *     test rax, rax          ; sets ZF if rax == 0
 *     jz  name               ; jump if ZF set (i.e., src1 was zero / false)
 *
 *   Why test rax,rax instead of cmp rax,0?
 *   test is one byte shorter (opcode + ModRM vs opcode + ModRM + imm32) and
 *   on modern CPUs it executes in the same cycle.
 *
 *   Why jz instead of je?
 *   jz (jump-if-zero) and je (jump-if-equal) are the same opcode (0x74 /
 *   0x0F 84).  jz is more expressive here: we're testing for ZERO, not
 *   for equality after a cmp — using jz communicates the intent clearly.
 *
 * ── Comparisons ───────────────────────────────────────────────────────────
 *   For IR_LT (and siblings):
 *     mov rax, [rbp-slot(src1)]
 *     cmp rax, [rbp-slot(src2)]
 *     setl al            ; al = 1 if less-than, 0 otherwise
 *     movzx rax, al      ; zero-extend byte result to 64-bit
 *     mov [rbp-slot(dst)], rax
 *
 * ── Calling Convention (System V AMD64) ───────────────────────────────────
 *   First 6 integer args: rdi rsi rdx rcx r8 r9 (in that order)
 *   Return value: rax
 *   Callee-saved: rbx rbp r12-r15 (we only use rax/rcx/rdx/r8/r9/rdi/rsi)
 *
 * ── GNU Stack Note ────────────────────────────────────────────────────────
 *   On Linux, the linker marks the stack executable unless every object
 *   file includes a .note.GNU-stack section with no executable flag.
 *   We emit this at the end of the file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* ---------------------------------------------------------------- helpers */

/* Return the memory operand string for a given slot number.
   slot n → [rbp - (n+1)*8]
   We write into a static buffer — callers use it immediately, not stored. */
static char slot_buf[32];
static const char *slot(int n) {
    snprintf(slot_buf, sizeof(slot_buf), "qword [rbp - %d]", (n + 1) * 8);
    return slot_buf;
}

/* System V integer argument registers, in order. */
static const char *argregs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
#define N_ARGREGS 6

/* ---------------------------------------------------------- local var map */
/*
 * We need to map variable names to slot numbers.
 * The IRFunc already assigns slots in ir.c; we recover the mapping by
 * scanning the IR for IR_STORE with nargs==-1 (parameter sentinel) and
 * regular IR_STORE instructions combined with IR_LOAD instructions.
 *
 * Actually, since ir.c calls local_slot() which monotonically increments
 * next_temp, each unique name gets a unique slot that is fixed once.
 * We rebuild the table here by scanning IR_STORE / IR_LOAD instructions.
 */

#define MAX_LOCALS 128

typedef struct {
    char name[64];
    int  slot_idx;
} LocalEntry;

typedef struct {
    LocalEntry entries[MAX_LOCALS];
    int        n;
} LocalMap;

static int localmap_get(LocalMap *m, const char *name) {
    for (int i = 0; i < m->n; i++) {
        if (strcmp(m->entries[i].name, name) == 0)
            return m->entries[i].slot_idx;
    }
    return -1; /* not found */
}

static void localmap_set(LocalMap *m, const char *name, int slot_idx) {
    if (localmap_get(m, name) >= 0) return; /* already mapped */
    if (m->n >= MAX_LOCALS) return;
    strncpy(m->entries[m->n].name, name, 63);
    m->entries[m->n].slot_idx = slot_idx;
    m->n++;
}

/* Build the local map by pre-scanning IR for STORE/LOAD with names. */
static void build_localmap(LocalMap *m, const IRFunc *fn) {
    memset(m, 0, sizeof(*m));
    /* Parameter stores come first (nargs == -1), src1 == param_index */
    int param_slot_counter = 0;
    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->op == IR_STORE && ins->nargs == -1) {
            /* Parameter: src1 is param index, map name to a fresh slot */
            /* Params are stored into the first n_param slots */
            if (localmap_get(m, ins->name) < 0) {
                localmap_set(m, ins->name, param_slot_counter);
                param_slot_counter++;
            }
        }
    }
    /* Now handle regular STORE and LOAD — the slot is ins->dst for LOAD,
       and for STORE the dst written is tracked by ir.c in next_temp.
       Since we don't have a direct name→slot mapping in IRInstr for STORE
       (STORE has no dst, only name + src1), we rebuild by observing that
       ir.c calls local_slot() which increments next_temp each time a NEW
       name appears.  We can simulate that by walking in order. */
    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->op == IR_LOAD && ins->dst >= 0) {
            /* IR_LOAD: dst = load name — the dst slot IS the temp result,
               but the "home slot" of the variable 'name' might differ.
               We find it by name from what we already know, OR we infer
               it from IR_STORE instructions. */
            (void)ins; /* handled below */
        }
    }
    /* Better approach: scan all STORE instructions (regular, nargs==0),
       use the src1 value — wait, that's the VALUE temp, not the slot.
       The variable's home slot is recorded in local_slot() inside ir.c.
       Since we can't call that here, we use the following observation:
       the IR_LOAD instruction always has dst = new_temp(), which is a
       fresh temp.  The VARIABLE'S slot is NOT the same as dst of LOAD.
       ir.c keeps the variable in a named slot; codegen must emit
       [rbp - (slot+1)*8] using the slot returned by local_slot().

       We recover this by: for each IR_STORE with nargs==0, we know
       src1 is a value temp, and name is the variable.  We need the
       variable's slot.  Since ir.c assigns locals in the order they
       first appear in local_slot(), and all locals come AFTER the
       params in next_temp ordering, we replay local_slot() here. */
    int next = param_slot_counter; /* locals start after params */
    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->op == IR_STORE && ins->nargs == 0) {
            if (localmap_get(m, ins->name) < 0) {
                localmap_set(m, ins->name, next++);
            }
        }
        if (ins->op == IR_LOAD) {
            if (localmap_get(m, ins->name) < 0) {
                localmap_set(m, ins->name, next++);
            }
        }
    }
}

/* ----------------------------------------------------------- pending params */
/*
 * IR_PARAM instructions accumulate arguments; IR_CALL consumes them.
 * We buffer up to N_ARGREGS params and load them into registers before CALL.
 */
#define MAX_PARAMS 32

typedef struct {
    int slots[MAX_PARAMS]; /* temp slot of each argument */
    int count;
} ParamBuf;

/* ------------------------------------------------------- per-function emit */

static void emit_func(const IRFunc *fn, FILE *out) {
    /*
     * Frame size = (number of slots used) * 8, aligned to 16 bytes.
     * fn->next_temp is the total number of temps allocated (including locals).
     */
    int n_slots     = fn->next_temp;
    int frame_bytes = n_slots * 8;
    /* Round UP to next multiple of 16 for ABI alignment */
    if (frame_bytes % 16 != 0) frame_bytes += (16 - frame_bytes % 16);
    if (frame_bytes == 0) frame_bytes = 16; /* always allocate something */

    /* Build the variable→slot map */
    LocalMap lmap;
    build_localmap(&lmap, fn);

    /* ---- Function prologue ---- */
    fprintf(out, "global %s\n", fn->name);
    fprintf(out, "%s:\n", fn->name);
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, %d\n", frame_bytes);

    /*
     * Store incoming parameters from registers into their stack slots.
     * We count how many IR_STORE with nargs==-1 (param sentinels) appear.
     */
    int param_idx = 0;
    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        if (ins->op == IR_STORE && ins->nargs == -1) {
            int pslot = localmap_get(&lmap, ins->name);
            if (pslot < 0) pslot = param_idx;
            if (param_idx < N_ARGREGS) {
                fprintf(out, "    mov %s, %s\n", slot(pslot), argregs[param_idx]);
            }
            /* else: stack argument, not handled (>6 params) */
            param_idx++;
        }
    }

    /* ---- Instruction emission ---- */
    ParamBuf pbuf;
    pbuf.count = 0;

    for (IRInstr *ins = fn->head; ins; ins = ins->next) {
        switch (ins->op) {

        /* Skip param sentinel stores (already handled in prologue) */
        case IR_STORE:
            if (ins->nargs == -1) break;
            /* Regular store: variable = temp */
            {
                int vslot = localmap_get(&lmap, ins->name);
                if (vslot < 0) {
                    fprintf(stderr, "codegen: unknown local '%s'\n", ins->name);
                    break;
                }
                fprintf(out, "    ; store %s = t%d\n", ins->name, ins->src1);
                fprintf(out, "    mov rax, %s\n", slot(ins->src1));
                fprintf(out, "    mov %s, rax\n", slot(vslot));
            }
            break;

        case IR_LOAD:
            /* temp = variable */
            {
                int vslot = localmap_get(&lmap, ins->name);
                if (vslot < 0) {
                    fprintf(stderr, "codegen: unknown local '%s'\n", ins->name);
                    break;
                }
                fprintf(out, "    ; load t%d = %s\n", ins->dst, ins->name);
                fprintf(out, "    mov rax, %s\n", slot(vslot));
                fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            }
            break;

        case IR_ICONST:
            fprintf(out, "    ; t%d = %ld\n", ins->dst, ins->ival);
            fprintf(out, "    mov rax, %ld\n", ins->ival);
            fprintf(out, "    mov %s, rax\n",  slot(ins->dst));
            break;

        case IR_COPY:
            fprintf(out, "    ; t%d = t%d\n", ins->dst, ins->src1);
            fprintf(out, "    mov rax, %s\n",  slot(ins->src1));
            fprintf(out, "    mov %s, rax\n",  slot(ins->dst));
            break;

        case IR_NEG:
            fprintf(out, "    ; t%d = -t%d\n", ins->dst, ins->src1);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    neg rax\n");
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

        case IR_ADD:
            fprintf(out, "    ; t%d = t%d + t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    add rax, %s\n", slot(ins->src2));
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

        case IR_SUB:
            fprintf(out, "    ; t%d = t%d - t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    sub rax, %s\n", slot(ins->src2));
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

        case IR_MUL:
            fprintf(out, "    ; t%d = t%d * t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    imul rax, %s\n", slot(ins->src2));
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

        case IR_DIV:
            /*
             * idiv requires rax:rdx as the dividend.
             * cqo sign-extends rax into rdx:rax (64-bit / 64-bit division).
             */
            fprintf(out, "    ; t%d = t%d / t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    cqo\n");               /* sign-extend rax into rdx */
            fprintf(out, "    idiv %s\n", slot(ins->src2));
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

        case IR_MOD:
            fprintf(out, "    ; t%d = t%d %% t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    cqo\n");
            fprintf(out, "    idiv %s\n", slot(ins->src2));
            fprintf(out, "    mov %s, rdx\n", slot(ins->dst)); /* remainder in rdx */
            break;

        /* ---- Comparisons ---- */
        /*
         * Pattern for all comparisons:
         *   cmp src1, src2      (sets flags)
         *   setX al             (al = 1 or 0)
         *   movzx rax, al       (zero-extend byte to qword)
         *   mov [dst slot], rax
         */
#define EMIT_CMP(setinstr) \
            fprintf(out, "    mov rax, %s\n", slot(ins->src1)); \
            fprintf(out, "    cmp rax, %s\n", slot(ins->src2)); \
            fprintf(out, "    " setinstr " al\n"); \
            fprintf(out, "    movzx rax, al\n"); \
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));

        case IR_LT:
            fprintf(out, "    ; t%d = t%d < t%d\n", ins->dst, ins->src1, ins->src2);
            EMIT_CMP("setl")
            break;
        case IR_GT:
            fprintf(out, "    ; t%d = t%d > t%d\n", ins->dst, ins->src1, ins->src2);
            EMIT_CMP("setg")
            break;
        case IR_LEQ:
            fprintf(out, "    ; t%d = t%d <= t%d\n", ins->dst, ins->src1, ins->src2);
            EMIT_CMP("setle")
            break;
        case IR_GEQ:
            fprintf(out, "    ; t%d = t%d >= t%d\n", ins->dst, ins->src1, ins->src2);
            EMIT_CMP("setge")
            break;
        case IR_EQ:
            fprintf(out, "    ; t%d = t%d == t%d\n", ins->dst, ins->src1, ins->src2);
            EMIT_CMP("sete")
            break;
        case IR_NEQ:
            fprintf(out, "    ; t%d = t%d != t%d\n", ins->dst, ins->src1, ins->src2);
            EMIT_CMP("setne")
            break;

        case IR_AND:
            /* Bitwise AND of boolean values (0 or 1) */
            fprintf(out, "    ; t%d = t%d && t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    and rax, %s\n", slot(ins->src2));
            fprintf(out, "    setne al\n");
            fprintf(out, "    movzx rax, al\n");
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

        case IR_OR:
            fprintf(out, "    ; t%d = t%d || t%d\n", ins->dst, ins->src1, ins->src2);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    or rax, %s\n", slot(ins->src2));
            fprintf(out, "    setne al\n");
            fprintf(out, "    movzx rax, al\n");
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            break;

#undef EMIT_CMP

        /* ---- Control flow ---- */

        case IR_LABEL:
            /*
             * IR_LABEL emits a label.  We prefix user-generated labels with
             * "L" (already done by irgen: label_str() produces "L<id>").
             * This ensures they never collide with function names (which are
             * not prefixed) or external symbols.
             */
            fprintf(out, ".%s:\n", ins->name);
            break;

        case IR_JUMP:
            /*
             * Unconditional jump.  The target label uses the same ".L" prefix
             * so NASM treats it as a local label (relative to the function).
             */
            fprintf(out, "    ; unconditional jump to %s\n", ins->name);
            fprintf(out, "    jmp .%s\n", ins->name);
            break;

        case IR_JUMPZ:
            /*
             * Conditional jump: jump to target if src1 == 0 (false).
             *
             * We use:
             *   test rax, rax    -- computes rax AND rax, sets ZF if result=0
             *   jz   target      -- jump if ZF set (i.e., rax was zero)
             *
             * Alternatively we could write: cmp rax, 0 / jz target
             * But "test rax, rax" is shorter (no immediate operand) and
             * equally readable once you know the idiom.
             */
            fprintf(out, "    ; jump to %s if t%d == 0\n", ins->name, ins->src1);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    test rax, rax\n");
            fprintf(out, "    jz .%s\n", ins->name);
            break;

        /* ---- Function calls ---- */

        case IR_PARAM:
            /*
             * Buffer the parameter temp slot.  We emit the actual register
             * loads when we see the IR_CALL instruction.
             */
            if (pbuf.count < MAX_PARAMS) {
                pbuf.slots[pbuf.count++] = ins->src1;
            }
            break;

        case IR_CALL:
            /*
             * Load buffered arguments into registers rdi, rsi, rdx, ...
             * then emit CALL.  Result lands in rax; move to dst slot.
             */
            fprintf(out, "    ; call %s(%d args)\n", ins->name, pbuf.count);
            for (int i = 0; i < pbuf.count && i < N_ARGREGS; i++) {
                fprintf(out, "    mov %s, %s\n", argregs[i], slot(pbuf.slots[i]));
            }
            fprintf(out, "    call %s\n", ins->name);
            fprintf(out, "    mov %s, rax\n", slot(ins->dst));
            pbuf.count = 0;
            break;

        case IR_RETURN:
            /*
             * Move return value into rax, then restore the stack frame.
             *   mov rsp, rbp   -- deallocate locals (reverse of 'sub rsp, N')
             *   pop rbp        -- restore caller's base pointer
             *   ret            -- return to caller
             */
            fprintf(out, "    ; return t%d\n", ins->src1);
            fprintf(out, "    mov rax, %s\n", slot(ins->src1));
            fprintf(out, "    mov rsp, rbp\n");
            fprintf(out, "    pop rbp\n");
            fprintf(out, "    ret\n");
            break;

        default:
            fprintf(stderr, "codegen: unhandled IR op %d\n", ins->op);
            break;
        }
    }

    /* Emit a fallthrough return of 0 in case the function body has no
       explicit return (e.g. void functions, or missing return path). */
    fprintf(out, "    ; fallthrough return 0\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, "\n");
}

/* ---------------------------------------------------------- public entry */

void codegen_emit(const IRProg *prog, FILE *out) {
    /* NASM file header */
    fprintf(out, "; Generated by codegen11 — Module 11: Control Flow\n");
    fprintf(out, "; Assemble: nasm -f elf64 <file> -o <file>.o\n");
    fprintf(out, "; Link:     gcc -no-pie <file>.o -o <bin>\n");
    fprintf(out, "\n");
    fprintf(out, "section .text\n");
    fprintf(out, "\n");

    for (int f = 0; f < prog->n_funcs; f++) {
        emit_func(&prog->funcs[f], out);
    }

    /*
     * GNU stack note section.
     *
     * Without this, the Linux linker (ld) marks the stack EXECUTABLE
     * because it cannot prove no object file requires it.  An executable
     * stack is a serious security vulnerability (makes code-injection
     * attacks much easier).
     *
     * This section tells the linker:
     *   "This object does NOT need an executable stack."
     * When all object files say the same, the resulting binary gets a
     * non-executable stack (NX / W^X).
     */
    fprintf(out, "section .note.GNU-stack noalloc noexec nowrite progbits\n");
}
