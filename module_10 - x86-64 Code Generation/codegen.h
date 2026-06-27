/* codegen.h — x86-64 NASM code generator for my_compiler
 * Module 10: Code Generation
 * Prerequisites: ir.h
 *
 * Strategy: naive stack allocation.
 *   - Each IR temporary t_i maps to [rbp - (i+1)*8].
 *   - Named local variables maintain a separate name->slot table;
 *     each slot is [rbp - (MAX_TEMPS + slot + 1)*8].
 *   - Follows the System V AMD64 ABI for function calls:
 *     first 6 integer args in rdi, rsi, rdx, rcx, r8, r9.
 *   - rax, rcx, rdx are used as scratch registers.
 */
#ifndef MY_COMPILER_CODEGEN_H
#define MY_COMPILER_CODEGEN_H

#include "ir.h"
#include <stdio.h>

/* Emit NASM assembly for prog to the file stream out.
 * Output includes a 'section .note.GNU-stack' for Linux compatibility. */
void codegen(const IRProg *prog, FILE *out);

#endif /* MY_COMPILER_CODEGEN_H */
