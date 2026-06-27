/* x86enc.c — x86-64 machine-code encoder implementation
 * Module 14: ELF Object File Emitter
 *
 * All encodings target the 64-bit System V ABI (Linux/WSL).
 * We use REX.W (0x48) for 64-bit operand size throughout.
 * Memory operands are always [rbp + disp32] (ModRM 10 + SIB/base rbp).
 */
#include "x86enc.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Tiny helpers                                                         */
/* ------------------------------------------------------------------ */

static void emit8(uint8_t *buf, size_t *pos, uint8_t b) {
    buf[(*pos)++] = b;
}

static void emit32(uint8_t *buf, size_t *pos, int32_t v) {
    uint32_t u = (uint32_t)v;
    buf[(*pos)++] = (uint8_t)(u);
    buf[(*pos)++] = (uint8_t)(u >> 8);
    buf[(*pos)++] = (uint8_t)(u >> 16);
    buf[(*pos)++] = (uint8_t)(u >> 24);
}

static void emit64(uint8_t *buf, size_t *pos, int64_t v) {
    uint64_t u = (uint64_t)v;
    for (int i = 0; i < 8; i++) { buf[(*pos)++] = (uint8_t)(u & 0xff); u >>= 8; }
}

/* ModRM byte for [rbp + disp32]: mod=10, reg=<reg3>, rm=101 */
static uint8_t modrm_rbp_disp32(uint8_t reg3) {
    return (uint8_t)(0x80 | ((reg3 & 7) << 3) | 5);
}

/* ------------------------------------------------------------------ */
/* Prologue / epilogue                                                  */
/* ------------------------------------------------------------------ */

void enc_push_rbp(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0x55); /* push rbp */
}

void enc_mov_rbp_rsp(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x89); emit8(buf, pos, 0xE5);
    /* REX.W  MOV r/m64, r64   ModRM: mod=11 reg=rsp(4) rm=rbp(5) => 0xE5 */
}

void enc_sub_rsp_imm32(uint8_t *buf, size_t *pos, int32_t imm) {
    /* REX.W 81 /5 id   SUB r/m64, imm32 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x81); emit8(buf, pos, 0xEC);
    emit32(buf, pos, imm);
}

void enc_mov_rsp_rbp(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x89); emit8(buf, pos, 0xEC);
    /* REX.W  MOV r/m64, r64  ModRM: mod=11 reg=rbp(5) rm=rsp(4) => 0xEC */
}

void enc_pop_rbp(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0x5D); /* pop rbp */
}

void enc_ret(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0xC3);
}

/* ------------------------------------------------------------------ */
/* Load / store                                                         */
/* ------------------------------------------------------------------ */

void enc_mov_rax_imm64(uint8_t *buf, size_t *pos, int64_t imm) {
    /* REX.W B8+rd  MOV rax, imm64 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0xB8);
    emit64(buf, pos, imm);
}

void enc_store_rax(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W 89 /r  MOV r/m64, rax — store rax to [rbp+disp] */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x89);
    emit8(buf, pos, modrm_rbp_disp32(0)); /* reg=rax(0) */
    emit32(buf, pos, disp);
}

void enc_load_rax(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W 8B /r  MOV rax, r/m64 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x8B);
    emit8(buf, pos, modrm_rbp_disp32(0));
    emit32(buf, pos, disp);
}

/* ------------------------------------------------------------------ */
/* Arithmetic                                                           */
/* ------------------------------------------------------------------ */

void enc_add_rax_mem(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W 03 /r  ADD rax, r/m64 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x03);
    emit8(buf, pos, modrm_rbp_disp32(0));
    emit32(buf, pos, disp);
}

void enc_sub_rax_mem(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W 2B /r  SUB rax, r/m64 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x2B);
    emit8(buf, pos, modrm_rbp_disp32(0));
    emit32(buf, pos, disp);
}

void enc_sub_rax_mem_via_rcx(uint8_t *buf, size_t *pos, int32_t disp) {
    /* mov rcx, [rbp+disp] */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x8B);
    emit8(buf, pos, modrm_rbp_disp32(1)); /* reg=rcx(1) */
    emit32(buf, pos, disp);
    /* sub rax, rcx  REX.W 29 /r  ModRM: mod=11 reg=rcx(1) rm=rax(0) => 0xC8 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x2B); emit8(buf, pos, 0xC1);
}

void enc_imul_rax_mem(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W 0F AF /r  IMUL rax, r/m64 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x0F); emit8(buf, pos, 0xAF);
    emit8(buf, pos, modrm_rbp_disp32(0));
    emit32(buf, pos, disp);
}

void enc_neg_rax(uint8_t *buf, size_t *pos) {
    /* REX.W F7 /3  NEG rax  ModRM: mod=11 reg=3 rm=rax(0) => 0xD8 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0xF7); emit8(buf, pos, 0xD8);
}

void enc_cqo(uint8_t *buf, size_t *pos) {
    /* REX.W 99  CQO */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x99);
}

void enc_idiv_mem(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W F7 /7  IDIV r/m64  ModRM: mod=10 reg=7 rm=rbp => 0xBD */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0xF7);
    emit8(buf, pos, modrm_rbp_disp32(7));
    emit32(buf, pos, disp);
}

void enc_mov_rax_rdx(uint8_t *buf, size_t *pos) {
    /* REX.W 89 D0  MOV rax, rdx  (ModRM mod=11 reg=rdx(2) rm=rax(0) => 0xD0) */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x89); emit8(buf, pos, 0xD0);
}

/* ------------------------------------------------------------------ */
/* Comparison                                                           */
/* ------------------------------------------------------------------ */

void enc_cmp_rax_mem(uint8_t *buf, size_t *pos, int32_t disp) {
    /* REX.W 3B /r  CMP rax, r/m64 */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x3B);
    emit8(buf, pos, modrm_rbp_disp32(0));
    emit32(buf, pos, disp);
}

void enc_test_rax_rax(uint8_t *buf, size_t *pos) {
    /* REX.W 85 C0  TEST rax, rax */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x85); emit8(buf, pos, 0xC0);
}

/* Helper: setcc al  then  movzx rax, al */
static void enc_setcc_rax(uint8_t *buf, size_t *pos, uint8_t setcc_op2) {
    /* 0F <op2> C0   setcc al (ModRM mod=11 reg=0 rm=0 => 0xC0) */
    emit8(buf, pos, 0x0F); emit8(buf, pos, setcc_op2); emit8(buf, pos, 0xC0);
    /* REX.W 0F B6 C0  MOVZX rax, al */
    emit8(buf, pos, 0x48); emit8(buf, pos, 0x0F); emit8(buf, pos, 0xB6);
    emit8(buf, pos, 0xC0);
}

void enc_setl_rax(uint8_t *buf, size_t *pos)  { enc_setcc_rax(buf, pos, 0x9C); }
void enc_setg_rax(uint8_t *buf, size_t *pos)  { enc_setcc_rax(buf, pos, 0x9F); }
void enc_setle_rax(uint8_t *buf, size_t *pos) { enc_setcc_rax(buf, pos, 0x9E); }
void enc_setge_rax(uint8_t *buf, size_t *pos) { enc_setcc_rax(buf, pos, 0x9D); }
void enc_sete_rax(uint8_t *buf, size_t *pos)  { enc_setcc_rax(buf, pos, 0x94); }
void enc_setne_rax(uint8_t *buf, size_t *pos) { enc_setcc_rax(buf, pos, 0x95); }

/* ------------------------------------------------------------------ */
/* Control flow                                                         */
/* ------------------------------------------------------------------ */

size_t enc_jmp_rel32(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0xE9);
    size_t patch = *pos;
    emit32(buf, pos, 0);
    return patch;
}

size_t enc_jz_rel32(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0x0F); emit8(buf, pos, 0x84);
    size_t patch = *pos;
    emit32(buf, pos, 0);
    return patch;
}

size_t enc_call_rel32(uint8_t *buf, size_t *pos) {
    emit8(buf, pos, 0xE8);
    size_t patch = *pos;
    emit32(buf, pos, 0);
    return patch;
}

void enc_patch_rel32(uint8_t *buf, size_t rel32_pos, size_t target_pos) {
    /* rel32 = target - (rel32_pos + 4) */
    int32_t rel = (int32_t)((ptrdiff_t)target_pos - (ptrdiff_t)(rel32_pos + 4));
    uint32_t u = (uint32_t)rel;
    buf[rel32_pos + 0] = (uint8_t)(u);
    buf[rel32_pos + 1] = (uint8_t)(u >> 8);
    buf[rel32_pos + 2] = (uint8_t)(u >> 16);
    buf[rel32_pos + 3] = (uint8_t)(u >> 24);
}

/* ------------------------------------------------------------------ */
/* Argument registers                                                   */
/* ------------------------------------------------------------------ */

/* Helper: MOV reg64, [rbp+disp]
 * REX = 0x48 for rdi/rsi/rdx/rcx, 0x4C for r8/r9 */
static void enc_mov_reg_mem(uint8_t *buf, size_t *pos,
                             uint8_t rex, uint8_t reg3, int32_t disp) {
    emit8(buf, pos, rex); emit8(buf, pos, 0x8B);
    emit8(buf, pos, modrm_rbp_disp32(reg3));
    emit32(buf, pos, disp);
}

void enc_mov_rdi_mem(uint8_t *buf, size_t *pos, int32_t disp) { enc_mov_reg_mem(buf, pos, 0x48, 7, disp); }
void enc_mov_rsi_mem(uint8_t *buf, size_t *pos, int32_t disp) { enc_mov_reg_mem(buf, pos, 0x48, 6, disp); }
void enc_mov_rdx_mem(uint8_t *buf, size_t *pos, int32_t disp) { enc_mov_reg_mem(buf, pos, 0x48, 2, disp); }
void enc_mov_rcx_mem(uint8_t *buf, size_t *pos, int32_t disp) { enc_mov_reg_mem(buf, pos, 0x48, 1, disp); }
void enc_mov_r8_mem(uint8_t *buf, size_t *pos, int32_t disp)  { enc_mov_reg_mem(buf, pos, 0x4C, 0, disp); }
void enc_mov_r9_mem(uint8_t *buf, size_t *pos, int32_t disp)  { enc_mov_reg_mem(buf, pos, 0x4C, 1, disp); }

/* Helper: MOV [rbp+disp], reg64 */
static void enc_store_reg_mem(uint8_t *buf, size_t *pos,
                               uint8_t rex, uint8_t reg3, int32_t disp) {
    emit8(buf, pos, rex); emit8(buf, pos, 0x89);
    emit8(buf, pos, modrm_rbp_disp32(reg3));
    emit32(buf, pos, disp);
}

void enc_store_rdi(uint8_t *buf, size_t *pos, int32_t disp) { enc_store_reg_mem(buf, pos, 0x48, 7, disp); }
void enc_store_rsi(uint8_t *buf, size_t *pos, int32_t disp) { enc_store_reg_mem(buf, pos, 0x48, 6, disp); }
void enc_store_rdx(uint8_t *buf, size_t *pos, int32_t disp) { enc_store_reg_mem(buf, pos, 0x48, 2, disp); }
void enc_store_rcx(uint8_t *buf, size_t *pos, int32_t disp) { enc_store_reg_mem(buf, pos, 0x48, 1, disp); }
void enc_store_r8(uint8_t *buf, size_t *pos, int32_t disp)  { enc_store_reg_mem(buf, pos, 0x4C, 0, disp); }
void enc_store_r9(uint8_t *buf, size_t *pos, int32_t disp)  { enc_store_reg_mem(buf, pos, 0x4C, 1, disp); }
