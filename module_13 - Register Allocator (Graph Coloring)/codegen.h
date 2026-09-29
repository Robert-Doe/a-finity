/* codegen.h — x86-64 NASM code generation with register allocation
 * Module 13: Register Allocation
 * Prerequisites: ir.h, regalloc.h
 *
 * This module generates NASM-syntax x86-64 assembly from the IR,
 * using the RegMap produced by regalloc() to assign temporaries to
 * real registers instead of always spilling to the stack.
 *
 * ABI notes (System V AMD64):
 *   Arguments 1-6 passed in: rdi, rsi, rdx, rcx, r8, r9
 *   Return value:            rax
 *   Caller-saved:            rax, rcx, rdx, rdi, rsi, r8, r9, r10, r11
 *   Callee-saved:            rbx, r12, r13, r14, r15, rbp, rsp
 *
 * Stack frame layout (after prologue):
 *   [rbp]       = saved rbp
 *   [rbp-8]     = local var slot 1  (or first spill if no locals)
 *    ...
 *   [rbp - 8*n_locals]        = last local variable
 *   [rbp - 8*(n_locals+1)]    = first spilled temporary
 *    ...
 */
#ifndef BOB_CODEGEN_H
#define BOB_CODEGEN_H

#include <stdio.h>
#include "ir.h"
#include "regalloc.h"

/* Generate NASM x86-64 assembly for the whole program to fp.
 * Performs register allocation per-function before emitting code. */
void codegen(const IRProg *prog, FILE *fp);

#endif /* BOB_CODEGEN_H */
