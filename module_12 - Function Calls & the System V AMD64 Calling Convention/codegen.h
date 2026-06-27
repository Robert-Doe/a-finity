/* codegen.h — x86-64 assembly code generator for my_compiler
 * Module 12: Function Calls and the System V AMD64 Calling Convention
 * Prerequisites: ir.h
 *
 * This module generates GNU-syntax x86-64 assembly that is ABI-compliant
 * with the System V AMD64 calling convention used on Linux (and macOS with
 * minor differences).
 *
 * Key ABI rules implemented here:
 *   - First 6 integer arguments: rdi, rsi, rdx, rcx, r8, r9
 *   - Return value: rax
 *   - Caller-saved (volatile): rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
 *   - Callee-saved (non-volatile): rbx, rbp, r12, r13, r14, r15
 *   - Stack must be 16-byte aligned immediately BEFORE the call instruction
 *     (the call itself pushes an 8-byte return address, making rsp 8-byte
 *     aligned at function entry — hence the prologue aligns it again).
 */
#ifndef MY_COMPILER_CODEGEN_H
#define MY_COMPILER_CODEGEN_H

#include "ir.h"
#include <stdio.h>

/* Generate a complete x86-64 assembly file for prog into the file
 * pointed to by out.  The output uses GNU assembler (gas) syntax and
 * is suitable for assembling with gcc or as.
 *
 * Each function in prog is emitted as a separate .globl label with a
 * standard prologue/epilogue.  All temporaries are spilled to the stack
 * (no register allocation in this module — that comes later). */
void codegen(const IRProg *prog, FILE *out);

#endif /* MY_COMPILER_CODEGEN_H */
