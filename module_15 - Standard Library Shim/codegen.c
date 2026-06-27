/* codegen.c — NASM x86-64 text code generator for my_compiler
 * Module 15: Standard Library Shim
 *
 * DESIGN OVERVIEW
 * ===============
 * This code generator targets NASM Intel-syntax x86-64 assembly (Linux ELF64).
 * It uses the same "everything on the stack" strategy as modules 10-12:
 *
 *   - Every IR temporary tN lives at [rbp - 8*(N+1)]
 *   - Every named local variable lives in a slot looked up by name
 *   - Function parameters are spilled from argument registers at entry
 *
 * Module 15 addition: IR_PRINT lowers to:
 *   mov  rdi, [rbp - offset(src1)]
 *   call print_int
 *
 * The print_int and print_newline symbols are declared "extern" at the
 * top of the file and supplied by runtime.asm at link time.
 *
 * OUTPUT FORMAT
 * =============
 * The output is NASM syntax (Intel order: dst, src) assembled with:
 *   nasm -f elf64 output.asm -o output.o
 *   gcc  -no-pie  output.o runtime.o -o program
 */

#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constants                                                             */
/* ------------------------------------------------------------------ */

/* System V AMD64 argument registers (NASM names). */
static const char *ARG_REGS[6] = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

#define MAX_LOCALS  256
#define MAX_TEMPS   512
#define MAX_PARAMS    6

/* ------------------------------------------------------------------ */
/* Local-variable name-to-slot mapping                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[64];
    int  slot;
} LocalEntry;

typedef struct {
    LocalEntry entries[MAX_LOCALS];
    int        count;
    int        next_slot;
} LocalMap;

static int local_slot(LocalMap *lm, const char *name) {
    for (int i = 0; i < lm->count; i++) {
        if (strcmp(lm->entries[i].name, name) == 0)
            return lm->entries[i].slot;
    }
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

/* Slot N -> [rbp - 8*(N+1)] */
static int temp_offset(int t)    { return -8 * (t + 1); }
static int local_offset(int slot){ return -8 * (slot + 1); }

/* ------------------------------------------------------------------ */
/* Pre-pass: count temporaries and named locals                          */
/* ------------------------------------------------------------------ */

static void prepass(const IRFunc *f, int *out_n_temps, LocalMap *lm) {
    *out_n_temps  = f->next_temp;
    lm->count     = 0;
    lm->next_slot = f->next_temp;

    for (IRInstr *in = f->head; in; in = in->next) {
        if (in->op == IR_STORE || in->op == IR_LOAD) {
            local_slot(lm, in->name);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Frame size computation                                                */
/* ------------------------------------------------------------------ */

/*
 * We need (frame_size + 8) % 16 == 0 for 16-byte stack alignment.
 * (The +8 accounts for the saved rbp pushed in the prologue.)
 * So frame_size % 16 must equal 8.
 */
static int compute_frame_size(int total_slots) {
    int raw = total_slots * 8;
    if (raw == 0) raw = 8;
    if ((raw + 8) % 16 != 0) raw += 8;
    return raw;
}

/* ------------------------------------------------------------------ */
/* Parameter accumulator                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    int temps[MAX_PARAMS];
    int count;
} ParamBuf;

/* ------------------------------------------------------------------ */
/* Emit a single function in NASM syntax                                 */
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
    fprintf(out, "global %s\n", f->name);
    fprintf(out, "%s:\n", f->name);
    fprintf(out, "    ; Prologue\n");
    fprintf(out, "    push    rbp\n");
    fprintf(out, "    mov     rbp, rsp\n");
    fprintf(out, "    sub     rsp, %d\n", frame_size);
    fprintf(out, "    ; Frame: %d slot(s) x 8 bytes = %d bytes (16-byte aligned)\n",
            total_slots, frame_size);

    /* ---- Detect and spill incoming parameters ---- */
    char param_names[MAX_PARAMS][64];
    int  n_params = 0;
    {
        char stored[MAX_LOCALS][64];
        int  n_stored = 0;

        for (IRInstr *in = f->head; in; in = in->next) {
            if (in->op == IR_STORE) {
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
                int is_stored = 0;
                for (int i = 0; i < n_stored; i++) {
                    if (strcmp(stored[i], in->name) == 0) { is_stored = 1; break; }
                }
                if (!is_stored) {
                    int already = 0;
                    for (int i = 0; i < n_params; i++) {
                        if (strcmp(param_names[i], in->name) == 0) { already = 1; break; }
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

    if (n_params > 0) {
        fprintf(out, "    ; Spill incoming parameters\n");
        for (int i = 0; i < n_params; i++) {
            int slot = local_slot(&lm, param_names[i]);
            int off  = local_offset(slot);
            fprintf(out, "    mov     qword [rbp%+d], %s   ; param '%s'\n",
                    off, ARG_REGS[i], param_names[i]);
        }
    }

    /* ---- Parameter accumulator for outgoing calls ---- */
    ParamBuf pb;
    pb.count = 0;

    /* ---- Emit each instruction ---- */
    for (IRInstr *in = f->head; in; in = in->next) {
        switch (in->op) {

        /* ---- IR_ICONST: t_dst = immediate ---- */
        case IR_ICONST:
            fprintf(out, "    mov     qword [rbp%+d], %ld   ; t%d = %ld\n",
                    temp_offset(in->dst), in->ival, in->dst, in->ival);
            break;

        /* ---- IR_COPY: t_dst = t_src1 ---- */
        case IR_COPY:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    mov     [rbp%+d], rax   ; t%d = copy t%d\n",
                    temp_offset(in->dst), in->dst, in->src1);
            break;

        /* ---- IR_NEG: t_dst = -t_src1 ---- */
        case IR_NEG:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    neg     rax\n");
            fprintf(out, "    mov     [rbp%+d], rax   ; t%d = -t%d\n",
                    temp_offset(in->dst), in->dst, in->src1);
            break;

        /* ---- IR_ADD ---- */
        case IR_ADD:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    add     rax, [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    mov     [rbp%+d], rax   ; t%d = t%d + t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_SUB ---- */
        case IR_SUB:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    sub     rax, [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    mov     [rbp%+d], rax   ; t%d = t%d - t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_MUL ---- */
        case IR_MUL:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    imul    rax, [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    mov     [rbp%+d], rax   ; t%d = t%d * t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_DIV ---- */
        case IR_DIV:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    cqo\n");
            fprintf(out, "    idiv    qword [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    mov     [rbp%+d], rax   ; t%d = t%d / t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- IR_MOD ---- */
        case IR_MOD:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    cqo\n");
            fprintf(out, "    idiv    qword [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    mov     [rbp%+d], rdx   ; t%d = t%d %% t%d\n",
                    temp_offset(in->dst), in->dst, in->src1, in->src2);
            break;

        /* ---- Comparisons ---- */
        case IR_LT:
        case IR_GT:
        case IR_LEQ:
        case IR_GEQ:
        case IR_EQ:
        case IR_NEQ: {
            const char *setcc = "sete";
            switch (in->op) {
                case IR_LT:  setcc = "setl";  break;
                case IR_GT:  setcc = "setg";  break;
                case IR_LEQ: setcc = "setle"; break;
                case IR_GEQ: setcc = "setge"; break;
                case IR_EQ:  setcc = "sete";  break;
                case IR_NEQ: setcc = "setne"; break;
                default: break;
            }
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    cmp     rax, [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    %s    al\n", setcc);
            fprintf(out, "    movzx   rax, al\n");
            fprintf(out, "    mov     [rbp%+d], rax\n", temp_offset(in->dst));
            break;
        }

        /* ---- IR_AND ---- */
        case IR_AND:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    test    rax, rax\n");
            fprintf(out, "    setne   al\n");
            fprintf(out, "    movzx   rax, al\n");
            fprintf(out, "    mov     rcx, [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    test    rcx, rcx\n");
            fprintf(out, "    setne   cl\n");
            fprintf(out, "    movzx   rcx, cl\n");
            fprintf(out, "    and     rax, rcx\n");
            fprintf(out, "    mov     [rbp%+d], rax\n", temp_offset(in->dst));
            break;

        /* ---- IR_OR ---- */
        case IR_OR:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    test    rax, rax\n");
            fprintf(out, "    setne   al\n");
            fprintf(out, "    movzx   rax, al\n");
            fprintf(out, "    mov     rcx, [rbp%+d]\n", temp_offset(in->src2));
            fprintf(out, "    test    rcx, rcx\n");
            fprintf(out, "    setne   cl\n");
            fprintf(out, "    movzx   rcx, cl\n");
            fprintf(out, "    or      rax, rcx\n");
            fprintf(out, "    mov     [rbp%+d], rax\n", temp_offset(in->dst));
            break;

        /* ---- IR_STORE: named_var = t_src1 ---- */
        case IR_STORE: {
            int slot = local_slot(&lm, in->name);
            int off  = local_offset(slot);
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    mov     [rbp%+d], rax   ; store '%s'\n",
                    off, in->name);
            break;
        }

        /* ---- IR_LOAD: t_dst = named_var ---- */
        case IR_LOAD: {
            int slot = local_slot(&lm, in->name);
            int off  = local_offset(slot);
            fprintf(out, "    mov     rax, [rbp%+d]   ; load '%s'\n",
                    off, in->name);
            fprintf(out, "    mov     [rbp%+d], rax\n", temp_offset(in->dst));
            break;
        }

        /* ---- IR_LABEL ---- */
        case IR_LABEL:
            fprintf(out, ".L_%s_%s:\n", f->name, in->name);
            break;

        /* ---- IR_JUMP ---- */
        case IR_JUMP:
            fprintf(out, "    jmp     .L_%s_%s\n", f->name, in->name);
            break;

        /* ---- IR_JUMPZ ---- */
        case IR_JUMPZ:
            fprintf(out, "    mov     rax, [rbp%+d]\n", temp_offset(in->src1));
            fprintf(out, "    test    rax, rax\n");
            fprintf(out, "    jz      .L_%s_%s\n", f->name, in->name);
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
            fprintf(out, "    ; Call %s (%d args)\n", in->name, pb.count);
            for (int i = 0; i < pb.count; i++) {
                fprintf(out, "    mov     %s, [rbp%+d]   ; arg %d\n",
                        ARG_REGS[i], temp_offset(pb.temps[i]), i);
            }
            fprintf(out, "    call    %s\n", in->name);
            if (in->dst != IR_NO_TEMP) {
                fprintf(out, "    mov     [rbp%+d], rax   ; t%d = return value\n",
                        temp_offset(in->dst), in->dst);
            }
            pb.count = 0;
            break;
        }

        /* ---- IR_RETURN ---- */
        case IR_RETURN:
            if (in->src1 != IR_NO_TEMP) {
                fprintf(out, "    mov     rax, [rbp%+d]   ; return value\n",
                        temp_offset(in->src1));
            } else {
                fprintf(out, "    xor     eax, eax   ; void return\n");
            }
            fprintf(out, "    ; Epilogue\n");
            fprintf(out, "    mov     rsp, rbp\n");
            fprintf(out, "    pop     rbp\n");
            fprintf(out, "    ret\n");
            break;

        /* ---- IR_PRINT (Module 15): call print_int(src1) ---- */
        case IR_PRINT:
            fprintf(out, "    mov     rdi, [rbp%+d]   ; print argument\n",
                    temp_offset(in->src1));
            fprintf(out, "    call    print_int\n");
            break;

        default:
            fprintf(stderr, "codegen: unknown IROp %d\n", (int)in->op);
            break;
        }
    }

    /* Safety net: implicit return 0 if body did not end with IR_RETURN. */
    fprintf(out, "    ; (implicit return 0)\n");
    fprintf(out, "    xor     eax, eax\n");
    fprintf(out, "    mov     rsp, rbp\n");
    fprintf(out, "    pop     rbp\n");
    fprintf(out, "    ret\n");
}

/* ------------------------------------------------------------------ */
/* codegen — public entry point                                          */
/* ------------------------------------------------------------------ */

void codegen(const IRProg *prog, FILE *out) {
    /* File header — NASM syntax */
    fprintf(out, "; Generated by my_compiler Module 15\n");
    fprintf(out, "; Assemble: nasm -f elf64 <this_file> -o prog.o\n");
    fprintf(out, "; Link:     nasm -f elf64 runtime.asm -o runtime.o\n");
    fprintf(out, ";           gcc -no-pie prog.o runtime.o -o program\n");
    fprintf(out, "\n");

    /* Declare external runtime symbols */
    fprintf(out, "extern print_int\n");
    fprintf(out, "extern print_newline\n");
    fprintf(out, "\n");
    fprintf(out, "section .text\n");

    for (int i = 0; i < prog->n_funcs; i++) {
        emit_function(&prog->funcs[i], out);
    }
}
