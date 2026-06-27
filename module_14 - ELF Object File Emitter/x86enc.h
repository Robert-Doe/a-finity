/* x86enc.h — x86-64 machine-code encoder for my_compiler
 * Module 14: ELF Object File Emitter
 *
 * Instead of emitting NASM text we emit raw bytes directly into a
 * caller-supplied byte buffer.  Every function takes (buf, pos) where
 * buf is a uint8_t array and *pos is the current write offset.
 *
 * Conventions used by the System V AMD64 ABI (Linux/WSL):
 *   Arguments : rdi, rsi, rdx, rcx, r8, r9
 *   Return    : rax
 *   Scratch   : rax, rcx, rdx, rdi, rsi, r8, r9, r10, r11
 *   Preserved : rbx, rbp, r12-r15
 *   Stack     : 16-byte aligned before a call instruction
 */
#ifndef MY_COMPILER_X86ENC_H
#define MY_COMPILER_X86ENC_H

#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Prologue / epilogue                                                  */
/* ------------------------------------------------------------------ */

/* push rbp */
void enc_push_rbp(uint8_t *buf, size_t *pos);

/* mov rbp, rsp */
void enc_mov_rbp_rsp(uint8_t *buf, size_t *pos);

/* sub rsp, imm32  — reserves frame space (always emits 32-bit form) */
void enc_sub_rsp_imm32(uint8_t *buf, size_t *pos, int32_t imm);

/* mov rsp, rbp */
void enc_mov_rsp_rbp(uint8_t *buf, size_t *pos);

/* pop rbp */
void enc_pop_rbp(uint8_t *buf, size_t *pos);

/* ret */
void enc_ret(uint8_t *buf, size_t *pos);

/* ------------------------------------------------------------------ */
/* Load / store                                                         */
/* ------------------------------------------------------------------ */

/* mov rax, imm64 */
void enc_mov_rax_imm64(uint8_t *buf, size_t *pos, int64_t imm);

/* mov qword [rbp + disp], rax   (disp is negative for locals) */
void enc_store_rax(uint8_t *buf, size_t *pos, int32_t disp);

/* mov rax, qword [rbp + disp] */
void enc_load_rax(uint8_t *buf, size_t *pos, int32_t disp);

/* ------------------------------------------------------------------ */
/* Arithmetic  (all use rax as destination)                            */
/* ------------------------------------------------------------------ */

/* add rax, qword [rbp + disp] */
void enc_add_rax_mem(uint8_t *buf, size_t *pos, int32_t disp);

/* sub rax, qword [rbp + disp]  — rax = rax - mem */
void enc_sub_rax_mem(uint8_t *buf, size_t *pos, int32_t disp);

/* imul rax, qword [rbp + disp] */
void enc_imul_rax_mem(uint8_t *buf, size_t *pos, int32_t disp);

/* neg rax */
void enc_neg_rax(uint8_t *buf, size_t *pos);

/* Division: idiv.
 * Idiom: load dividend into rax already done by caller.
 *   cqo                  — sign-extend rax into rdx:rax
 *   idiv qword [rbp+disp]  — rax = quotient, rdx = remainder
 * After the call, rax holds quotient; rdx holds remainder. */
void enc_cqo(uint8_t *buf, size_t *pos);
void enc_idiv_mem(uint8_t *buf, size_t *pos, int32_t disp);

/* mov rdx, rax  — copy remainder result to rax for MOD */
void enc_mov_rax_rdx(uint8_t *buf, size_t *pos);

/* ------------------------------------------------------------------ */
/* Comparison                                                           */
/* ------------------------------------------------------------------ */

/* cmp rax, qword [rbp + disp] */
void enc_cmp_rax_mem(uint8_t *buf, size_t *pos, int32_t disp);

/* test rax, rax */
void enc_test_rax_rax(uint8_t *buf, size_t *pos);

/* setl/setg/setle/setge/sete/setne al  then  movzx rax, al */
void enc_setl_rax(uint8_t *buf, size_t *pos);
void enc_setg_rax(uint8_t *buf, size_t *pos);
void enc_setle_rax(uint8_t *buf, size_t *pos);
void enc_setge_rax(uint8_t *buf, size_t *pos);
void enc_sete_rax(uint8_t *buf, size_t *pos);
void enc_setne_rax(uint8_t *buf, size_t *pos);

/* ------------------------------------------------------------------ */
/* Control flow                                                         */
/* ------------------------------------------------------------------ */

/* jmp rel32  — emits a 5-byte near jump.
 * Returns the offset of the rel32 field so the caller can patch it. */
size_t enc_jmp_rel32(uint8_t *buf, size_t *pos);

/* jz rel32  — emits a 6-byte near conditional jump.
 * Returns the offset of the rel32 field. */
size_t enc_jz_rel32(uint8_t *buf, size_t *pos);

/* call rel32 — emits a 5-byte near call.
 * Returns the offset of the rel32 field (for relocation). */
size_t enc_call_rel32(uint8_t *buf, size_t *pos);

/* Patch a previously emitted rel32 field so the jump/call targets
 * the instruction at 'target_pos'.  'rel32_pos' is the offset of the
 * 4-byte field; 'instr_end' is the offset just after the instruction
 * (i.e. rel32_pos + 4). */
void enc_patch_rel32(uint8_t *buf, size_t rel32_pos, size_t target_pos);

/* ------------------------------------------------------------------ */
/* Argument registers (for function calls)                             */
/* ------------------------------------------------------------------ */

/* mov rdi/rsi/rdx/rcx/r8/r9, qword [rbp + disp] */
void enc_mov_rdi_mem(uint8_t *buf, size_t *pos, int32_t disp);
void enc_mov_rsi_mem(uint8_t *buf, size_t *pos, int32_t disp);
void enc_mov_rdx_mem(uint8_t *buf, size_t *pos, int32_t disp);
void enc_mov_rcx_mem(uint8_t *buf, size_t *pos, int32_t disp);
void enc_mov_r8_mem(uint8_t *buf, size_t *pos, int32_t disp);
void enc_mov_r9_mem(uint8_t *buf, size_t *pos, int32_t disp);

/* mov qword [rbp + disp], rdi  — spill incoming argument to frame */
void enc_store_rdi(uint8_t *buf, size_t *pos, int32_t disp);
void enc_store_rsi(uint8_t *buf, size_t *pos, int32_t disp);
void enc_store_rdx(uint8_t *buf, size_t *pos, int32_t disp);
void enc_store_rcx(uint8_t *buf, size_t *pos, int32_t disp);
void enc_store_r8(uint8_t *buf, size_t *pos, int32_t disp);
void enc_store_r9(uint8_t *buf, size_t *pos, int32_t disp);

/* ------------------------------------------------------------------ */
/* Miscellaneous                                                        */
/* ------------------------------------------------------------------ */

/* sub rax, qword [rbp+disp]  implemented as:
 *   mov rcx, [rbp+disp]
 *   sub rax, rcx
 * (avoids needing a separate sub_rax_mem with reversed operands) */
void enc_sub_rax_mem_via_rcx(uint8_t *buf, size_t *pos, int32_t disp);

#endif /* MY_COMPILER_X86ENC_H */
