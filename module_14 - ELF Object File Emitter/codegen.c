/* codegen.c — x86-64 code generator for my_compiler
 * Module 14: ELF Object File Emitter
 *
 * Strategy: every IR temp and named local lives on the stack frame.
 * We use the RegMap from regalloc to find [rbp - 8*slot] for each value.
 * All computation flows through rax (and rcx/rdx as scratch).
 */
#include "codegen.h"
#include "regalloc.h"
#include "x86enc.h"
#include "ir.h"
#include "elf_emit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum machine-code bytes per function */
#define FUNC_BUF (1 << 17)

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

/* Frame displacement (negative) for a slot number (1-based). */
static int32_t disp(int slot) {
    return (int32_t)(-8 * slot);
}

/* Displacement for an IR temporary. */
static int32_t temp_disp(const RegMap *rm, int t) {
    return disp(regmap_temp_slot(rm, t));
}

/* Displacement for a named local variable. */
static int32_t named_disp(const RegMap *rm, const char *name) {
    int s = regmap_named_slot(rm, name);
    if (s == 0) {
        fprintf(stderr, "codegen: unknown variable '%s'\n", name);
        exit(1);
    }
    return disp(s);
}

/* Load temp t into rax. */
static void load_temp(const RegMap *rm, uint8_t *buf, size_t *pos, int t) {
    enc_load_rax(buf, pos, temp_disp(rm, t));
}

/* Store rax into temp t. */
static void store_temp(const RegMap *rm, uint8_t *buf, size_t *pos, int t) {
    enc_store_rax(buf, pos, temp_disp(rm, t));
}

/* ------------------------------------------------------------------ */
/* Argument passing (System V AMD64): rdi,rsi,rdx,rcx,r8,r9           */
/* ------------------------------------------------------------------ */

typedef void (*LoadArgFn)(uint8_t *, size_t *, int32_t);

static LoadArgFn arg_loaders[6] = {
    enc_mov_rdi_mem,
    enc_mov_rsi_mem,
    enc_mov_rdx_mem,
    enc_mov_rcx_mem,
    enc_mov_r8_mem,
    enc_mov_r9_mem,
};

typedef void (*StoreArgFn)(uint8_t *, size_t *, int32_t);

static StoreArgFn arg_storers[6] = {
    enc_store_rdi,
    enc_store_rsi,
    enc_store_rdx,
    enc_store_rcx,
    enc_store_r8,
    enc_store_r9,
};

/* ------------------------------------------------------------------ */
/* Label / jump patching                                                */
/* ------------------------------------------------------------------ */

#define MAX_LABELS 256

/* label_pos[L] = byte offset of label L in the function buffer, or
 * (size_t)-1 if not yet seen. */
static size_t label_pos[MAX_LABELS];

/* Pending forward-jump patches */
typedef struct { size_t rel32_pos; int label; } Patch;
static Patch patches[1024];
static int   n_patches;

static void reset_labels(void) {
    for (int i = 0; i < MAX_LABELS; i++) label_pos[i] = (size_t)-1;
    n_patches = 0;
}

static void define_label(int L, size_t pos) {
    if (L < 0 || L >= MAX_LABELS) return;
    label_pos[L] = pos;
}

/* Emit a jmp rel32 to label L (may be forward). */
static void emit_jmp_label(uint8_t *buf, size_t *pos, int L) {
    size_t rel32 = enc_jmp_rel32(buf, pos);
    if (L >= 0 && label_pos[L] != (size_t)-1) {
        enc_patch_rel32(buf, rel32, label_pos[L]);
    } else {
        patches[n_patches].rel32_pos = rel32;
        patches[n_patches].label     = L;
        n_patches++;
    }
}

/* Emit a jz rel32 to label L (conditional jump if rax==0). */
static void emit_jz_label(uint8_t *buf, size_t *pos, int L) {
    size_t rel32 = enc_jz_rel32(buf, pos);
    if (L >= 0 && label_pos[L] != (size_t)-1) {
        enc_patch_rel32(buf, rel32, label_pos[L]);
    } else {
        patches[n_patches].rel32_pos = rel32;
        patches[n_patches].label     = L;
        n_patches++;
    }
}

static void apply_patches(uint8_t *buf) {
    for (int i = 0; i < n_patches; i++) {
        int L = patches[i].label;
        if (L >= 0 && L < MAX_LABELS && label_pos[L] != (size_t)-1) {
            enc_patch_rel32(buf, patches[i].rel32_pos, label_pos[L]);
        } else {
            fprintf(stderr, "codegen: unresolved label %d\n", L);
        }
    }
}

/* ------------------------------------------------------------------ */
/* PARAM staging: collect params then emit calls                        */
/* ------------------------------------------------------------------ */

#define MAX_PARAMS 6

static int    staged_params[MAX_PARAMS];
static int    n_staged;

/* ------------------------------------------------------------------ */
/* Code generation for one function                                     */
/* ------------------------------------------------------------------ */

static void codegen_func(const IRFunc *f, ElfEmitter *e) {
    RegMap rm;
    regmap_build(&rm, f);

    uint8_t buf[FUNC_BUF];
    size_t  pos = 0;

    reset_labels();
    n_staged = 0;

    /* ---- Prologue ---- */
    enc_push_rbp(buf, &pos);
    enc_mov_rbp_rsp(buf, &pos);

    /* Frame size: rm.n_slots * 8, rounded up to 16 */
    int frame_bytes = rm.n_slots * 8;
    if (frame_bytes % 16 != 0) frame_bytes += 16 - (frame_bytes % 16);
    if (frame_bytes > 0)
        enc_sub_rsp_imm32(buf, &pos, (int32_t)frame_bytes);

    /* ---- Spill incoming arguments (named params) ---- */
    /* The IRFunc stores incoming params as named locals in order.
     * n_named tells us how many there are, and they're slots 1..n_named. */
    for (int i = 0; i < rm.n_named && i < 6; i++) {
        int32_t d = disp(rm.named_slot[i]);
        arg_storers[i](buf, &pos, d);
    }

    /* ---- Translate IR instructions ---- */
    for (const IRInstr *in = f->head; in; in = in->next) {
        switch (in->op) {

        case IR_ICONST:
            enc_mov_rax_imm64(buf, &pos, in->ival);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_COPY:
            load_temp(&rm, buf, &pos, in->src1);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_NEG:
            load_temp(&rm, buf, &pos, in->src1);
            enc_neg_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_ADD:
            load_temp(&rm, buf, &pos, in->src1);
            enc_add_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_SUB:
            load_temp(&rm, buf, &pos, in->src1);
            enc_sub_rax_mem_via_rcx(buf, &pos, temp_disp(&rm, in->src2));
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_MUL:
            load_temp(&rm, buf, &pos, in->src1);
            enc_imul_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_DIV:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cqo(buf, &pos);
            enc_idiv_mem(buf, &pos, temp_disp(&rm, in->src2));
            /* rax = quotient */
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_MOD:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cqo(buf, &pos);
            enc_idiv_mem(buf, &pos, temp_disp(&rm, in->src2));
            /* rdx = remainder → move to rax */
            enc_mov_rax_rdx(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_LT:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cmp_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            enc_setl_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_GT:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cmp_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            enc_setg_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_LEQ:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cmp_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            enc_setle_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_GEQ:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cmp_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            enc_setge_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_EQ:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cmp_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            enc_sete_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_NEQ:
            load_temp(&rm, buf, &pos, in->src1);
            enc_cmp_rax_mem(buf, &pos, temp_disp(&rm, in->src2));
            enc_setne_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_AND:
            /* Boolean AND: result = (src1 != 0) & (src2 != 0) */
            load_temp(&rm, buf, &pos, in->src1);
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);  /* tmp = src1 != 0 */
            load_temp(&rm, buf, &pos, in->src2);
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            /* rax = src2 != 0; combine with dst */
            enc_add_rax_mem(buf, &pos, temp_disp(&rm, in->dst));
            /* rax now 0,1,2; we want rax == 2 → true */
            /* Simple: store and compare */
            store_temp(&rm, buf, &pos, in->dst);
            /* Reload and check == 2 via: cmp rax,2; sete; but no enc_cmp_rax_imm.
             * Workaround: subtract 1 twice and use test */
            /* Actually just use: result = (src1 != 0) * (src2 != 0) via imul */
            /* Redo cleanly: */
            load_temp(&rm, buf, &pos, in->src1);
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            load_temp(&rm, buf, &pos, in->src2);
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            enc_imul_rax_mem(buf, &pos, temp_disp(&rm, in->dst));
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_OR:
            /* Boolean OR: result = (src1 + src2) != 0 */
            load_temp(&rm, buf, &pos, in->src1);
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            load_temp(&rm, buf, &pos, in->src2);
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            enc_add_rax_mem(buf, &pos, temp_disp(&rm, in->dst));
            enc_test_rax_rax(buf, &pos);
            enc_setne_rax(buf, &pos);
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_LABEL:
            define_label((int)in->ival, pos);
            break;

        case IR_JUMP:
            emit_jmp_label(buf, &pos, (int)in->ival);
            break;

        case IR_JUMPZ:
            /* Jump to label in->ival if temp src1 == 0 */
            load_temp(&rm, buf, &pos, in->src1);
            enc_test_rax_rax(buf, &pos);
            emit_jz_label(buf, &pos, (int)in->ival);
            break;

        case IR_STORE:
            /* IR_STORE: named variable (name) = src1 */
            load_temp(&rm, buf, &pos, in->src1);
            enc_store_rax(buf, &pos, named_disp(&rm, in->name));
            break;

        case IR_LOAD:
            /* IR_LOAD: dst = named variable (name) */
            enc_load_rax(buf, &pos, named_disp(&rm, in->name));
            store_temp(&rm, buf, &pos, in->dst);
            break;

        case IR_PARAM:
            /* Stage the parameter temp for the upcoming call */
            if (n_staged < MAX_PARAMS) {
                staged_params[n_staged++] = in->src1;
            }
            break;

        case IR_CALL: {
            /* Load staged params into argument registers */
            for (int i = 0; i < n_staged && i < 6; i++) {
                arg_loaders[i](buf, &pos, temp_disp(&rm, staged_params[i]));
            }
            n_staged = 0;
            /* Emit call rel32 (linker will patch for cross-function calls) */
            enc_call_rel32(buf, &pos);
            /* The rel32 is left as 0 for now; within-module calls in the
             * same .text section will be resolved by the linker since we
             * are writing a relocatable ET_REL object. */
            /* Store return value */
            if (in->dst != IR_NO_TEMP)
                store_temp(&rm, buf, &pos, in->dst);
            break;
        }

        case IR_RETURN:
            if (in->src1 != IR_NO_TEMP)
                load_temp(&rm, buf, &pos, in->src1);
            else
                enc_mov_rax_imm64(buf, &pos, 0);
            enc_mov_rsp_rbp(buf, &pos);
            enc_pop_rbp(buf, &pos);
            enc_ret(buf, &pos);
            break;

        default:
            fprintf(stderr, "codegen: unhandled IR op %d\n", in->op);
            break;
        }
    }

    /* Patch all forward jumps */
    apply_patches(buf);

    /* Add to ELF */
    elf_add_func(e, f->name, buf, pos);
}

/* ------------------------------------------------------------------ */
/* Public entry point                                                   */
/* ------------------------------------------------------------------ */

void codegen_program(const IRProg *prog, ElfEmitter *e) {
    for (int i = 0; i < prog->n_funcs; i++) {
        codegen_func(&prog->funcs[i], e);
    }
}
